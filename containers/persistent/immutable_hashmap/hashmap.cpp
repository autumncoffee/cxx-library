#include "hashmap.hpp"
#include <ac-common/utils/htonll.hpp>
#include <ac-library/contrib/murmur/MurmurHash3.h>
#include <absl/numeric/int128.h>
#include <stdio.h>
// #include <iostream>

#define ADD_INT_KEY(type) void TPersistentImmutableHashMap::Insert(type key, const TBlob& value) { \
    auto tmp = hton(key); \
    Insert(TBlob(sizeof(tmp), (char*)&tmp), value); \
}

#define GET_INT_KEY(type) TBlob TPersistentImmutableHashMap::Get(type key) const { \
    auto tmp = hton(key); \
    return Get(TBlob(sizeof(tmp), (char*)&tmp)); \
}

#define NEXT_INT_KEY(type) bool TPersistentImmutableHashMap::TIterator::Next(type& key, TBlob& value) { \
    TBlob tmp; \
\
    if (Next(tmp, value)) { \
        memcpy(&key, tmp.Data(), sizeof(key)); \
        key = ntoh(key); \
\
        return true; \
    } \
\
    return false; \
}

namespace {
    using namespace NAC;

    struct TValue {
        TBlob Key;
        TBlob Value;
        uint64_t Prev = 0;

        inline size_t Dump(TFile& out) const {
            uint64_t tmp(hton(Key.Size()));
            out.Append(sizeof(tmp), (char*)&tmp);
            out.Append(Key.Size(), Key.Data());

            tmp = hton(Value.Size());
            out.Append(sizeof(tmp), (char*)&tmp);
            out.Append(Value.Size(), Value.Data());

            tmp = hton(Prev);
            out.Append(sizeof(tmp), (char*)&tmp);

            return (sizeof(uint64_t) * 3) + Key.Size() + Value.Size();
        }

        static inline TValue Restore(char* ptr) {
            TValue out;
            uint64_t tmp;
            size_t offset = 0;

            memcpy(&tmp, ptr + offset, sizeof(tmp));
            offset += sizeof(tmp);
            tmp = ntoh(tmp);

            out.Key.Wrap(tmp, ptr + offset);
            offset += tmp;

            memcpy(&tmp, ptr + offset, sizeof(tmp));
            offset += sizeof(tmp);
            tmp = ntoh(tmp);

            out.Value.Wrap(tmp, ptr + offset);
            offset += tmp;

            memcpy(&tmp, ptr + offset, sizeof(tmp));
            offset += sizeof(tmp);
            out.Prev = ntoh(tmp);

            return out;
        }
    };
}

namespace NAC {
    TPersistentImmutableHashMap::TPersistentImmutableHashMap(
        const std::string& path,
        uint64_t bucketCount,
        uint32_t seed
    )
        : BucketCount_(bucketCount)
        , Seed(seed)
        , Path(path)
    {
        new (File_) TFile(Path + ".new", TFile::ACCESS_CREATE);

        if (!*this) {
            return;
        }

        auto& file = File();

        file.Resize(sizeof(uint64_t) * (1 + BucketCount_));
        file.SeekToEnd();
        // file.MSync();
        // std::cerr << "Created: " << file.Size() << std::endl;

        if (!*this) {
            return;
        }

        bucketCount = hton(bucketCount);
        memcpy(file.Data(), &bucketCount, sizeof(bucketCount));
    }

    TPersistentImmutableHashMap::TPersistentImmutableHashMap(
        const std::string& path,
        uint32_t seed,
        bool rw
    )
        : Seed(seed)
        , Path(path)
    {
        new (File_) TFile(Path, (rw ? TFile::ACCESS_RDWR : TFile::ACCESS_RDONLY));

        if (!*this) {
            return;
        }

        memcpy(&BucketCount_, File().Data(), sizeof(BucketCount_));
        BucketCount_ = ntoh(BucketCount_);
    }

    TPersistentImmutableHashMap::~TPersistentImmutableHashMap() {
        File().~TFile();
    }

    uint64_t TPersistentImmutableHashMap::Bucket(const TBlob& key) const {
        absl::uint128 hash;
        static_assert(sizeof(hash) == (sizeof(uint64_t) * 2));

        MurmurHash3_x64_128(key.Data(), key.Size(), Seed, &hash);

        return (uint64_t)(hash % BucketCount_);
        // std::cerr << "Bucket(" << (std::string)key << "): " << rv << std::endl;
        // return rv;
    }

    void TPersistentImmutableHashMap::Insert(const TBlob& key, const TBlob& value) {
        if (!*this || (BucketCount_ == 0)) {
            return;
        }

        auto* ptr = KeyPtr(key);
        size_t size = 0;

        {
            TValue tmp;
            memcpy(&tmp.Prev, ptr, sizeof(tmp));
            tmp.Prev = ntoh(tmp.Prev);
            tmp.Key.Wrap(key.Size(), key.Data());
            tmp.Value.Wrap(value.Size(), value.Data());

            size = tmp.Dump(File());
        }

        {
            uint64_t tmp(hton(DataPos));
            memcpy(ptr, &tmp, sizeof(tmp));
        }

        // std::cerr << "add: " << DataPos << ", " << size << std::endl;
        DataPos += size;
    }

    bool TPersistentImmutableHashMap::Close() {
        if (!*this) {
            return false;
        }

        // File().MSync();

        if (rename(File().Path().c_str(), Path.c_str()) == 0) {
            File().~TFile();
            new (File_) TFile(Path, TFile::ACCESS_RDONLY);
            return (bool)*this;

        } else {
            perror("rename");
            return false;
        }
    }

    TBlob TPersistentImmutableHashMap::Get(const TBlob& key) const {
        if (!*this || (BucketCount_ == 0)) {
            return TBlob();
        }

        const auto* ptr = KeyPtr(key);
        const size_t headerSize((sizeof(uint64_t) * (1 + BucketCount_)));

        if (File().Size() < (headerSize + DataPos - 1)) {
            TFile info(File().Path(), TFile::ACCESS_INFO);

            if (!info) {
                return TBlob();
            }

            if (File().Size() < info.Size()) {
                ((TFile*)File_)->Resize(info.Size());

                if (!*this) {
                    return TBlob();
                }
            }
        }

        uint64_t tmp;
        memcpy(&tmp, ptr, sizeof(tmp));
        // std::cerr << ntoh(tmp) << std::endl;
        tmp = headerSize + ntoh(tmp);

        if (tmp > headerSize) {
            --tmp;

        } else {
            return TBlob();
        }

        // std::cerr << tmp << ", " << File().Size() << std::endl;

        while (true) {
            auto rv = TValue::Restore(File().Data() + tmp);

            if (rv.Key.Cmp(key) == 0) {
                return TBlob(rv.Value.Size(), rv.Value.Data());

            } else if (rv.Prev > 0) {
                tmp = headerSize + rv.Prev - 1;

            } else {
                return TBlob();
            }
        }
    }

    ADD_INT_KEY(uint64_t);
    ADD_INT_KEY(uint32_t);
    ADD_INT_KEY(uint16_t);

    GET_INT_KEY(uint64_t);
    GET_INT_KEY(uint32_t);
    GET_INT_KEY(uint16_t);

    NEXT_INT_KEY(uint64_t);
    NEXT_INT_KEY(uint32_t);
    NEXT_INT_KEY(uint16_t);

    TPersistentImmutableHashMap::TIterator::TIterator(char* ptr, uint64_t bucketCount)
        : Ptr(ptr)
        , BucketCount_(bucketCount)
    {
    }

    bool TPersistentImmutableHashMap::TIterator::Next(TBlob& key, TBlob& value) {
        if (!Ptr) {
            return false;
        }

        if (Offset == 0) {
            while (true) {
                // std::cerr << "1: " << CurrentBucket << std::endl;
                memcpy(&Offset, BucketPtr(CurrentBucket), sizeof(Offset));
                Offset = ntoh(Offset);

                if (Offset > 0) {
                    break;

                } else if ((CurrentBucket + 1) == BucketCount_) {
                    Ptr = nullptr;
                    return false;

                } else {
                    ++CurrentBucket;
                }
            }
        }

        // std::cerr << "2: " << Offset << std::endl;

        const size_t headerSize((sizeof(uint64_t) * (1 + BucketCount_)));
        auto rv = TValue::Restore(Ptr + headerSize + Offset - 1);
        key.Wrap(rv.Key.Size(), rv.Key.Data());
        value.Wrap(rv.Value.Size(), rv.Value.Data());

        Offset = rv.Prev;

        if (Offset == 0) {
            if ((CurrentBucket + 1) == BucketCount_) {
                Ptr = nullptr;

            } else {
                ++CurrentBucket;
            }
        }

        // std::cerr << "3: " << CurrentBucket << ", " << Offset << std::endl;

        return true;
    }

    TPersistentImmutableHashMap::TIterator TPersistentImmutableHashMap::GetAll() const {
        if (!*this || (BucketCount_ == 0)) {
            return TIterator();
        }

        return TIterator(File().Data(), BucketCount_);
    }
}
