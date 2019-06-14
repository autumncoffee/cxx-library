#include "hashmap.hpp"
#include <ac-common/utils/htonll.hpp>
#include <ac-library/contrib/murmur/MurmurHash3.h>
#include <absl/numeric/int128.h>
#include <stdio.h>
#include <arpa/inet.h>
// #include <iostream>

#define ADD_INT_KEY(type) void TPersistentImmutableHashMap::Add(type key, const TBlob& value) { \
    auto tmp = DumpInt(key); \
    Add(TBlob(sizeof(tmp), (char*)&tmp), value); \
}

#define GET_INT_KEY(type) TBlob TPersistentImmutableHashMap::Get(type key) const { \
    auto tmp = DumpInt(key); \
    return Get(TBlob(sizeof(tmp), (char*)&tmp)); \
}

namespace {
    template<typename T>
    static T DumpInt(T val) {
        static_assert(
            (sizeof(T) == sizeof(uint64_t))
            || (sizeof(T) == sizeof(uint32_t))
            || (sizeof(T) == sizeof(uint16_t))
        );

        T out;

        if constexpr (sizeof(T) == sizeof(uint64_t)) {
            out = htons(val);

        } else if constexpr (sizeof(T) == sizeof(uint32_t)) {
            out = htonl(val);

        } else if constexpr (sizeof(T) == sizeof(uint16_t)) {
            out = htonll(val);
        }

        return out;
    }
}

namespace NAC {
    size_t TPersistentImmutableHashMap::TValue::Dump(TFile& out) const {
        uint64_t tmp(htonll(Key.Size()));
        out.Append(sizeof(tmp), (char*)&tmp);
        out.Append(Key.Size(), Key.Data());

        tmp = htonll(Value.Size());
        out.Append(sizeof(tmp), (char*)&tmp);
        out.Append(Value.Size(), Value.Data());

        tmp = htonll(Prev);
        out.Append(sizeof(tmp), (char*)&tmp);

        return (sizeof(uint64_t) * 3) + Key.Size() + Value.Size();
    }

    TPersistentImmutableHashMap::TValue TPersistentImmutableHashMap::TValue::Restore(char* ptr) {
        TPersistentImmutableHashMap::TValue out;
        uint64_t tmp;
        size_t offset = 0;

        memcpy(&tmp, ptr + offset, sizeof(tmp));
        offset += sizeof(tmp);
        tmp = ntohll(tmp);

        out.Key.Wrap(tmp, ptr + offset);
        offset += tmp;

        memcpy(&tmp, ptr + offset, sizeof(tmp));
        offset += sizeof(tmp);
        tmp = ntohll(tmp);

        out.Value.Wrap(tmp, ptr + offset);
        offset += tmp;

        memcpy(&tmp, ptr + offset, sizeof(tmp));
        offset += sizeof(tmp);
        out.Prev = ntohll(tmp);

        return out;
    }

    TPersistentImmutableHashMap::TPersistentImmutableHashMap(
        const std::string& path,
        uint64_t bucketCount,
        uint32_t seed
    )
        : BucketCount(bucketCount)
        , Seed(seed)
        , Path(path)
    {
        new (File_) TFile(Path + ".new", TFile::ACCESS_CREATE);

        if (!*this) {
            return;
        }

        auto& file = File();

        file.Resize(sizeof(uint64_t) * (1 + BucketCount));
        file.Stat();
        file.Map();
        file.SeekToEnd();
        // file.MSync();
        // std::cerr << "Created: " << file.Size() << std::endl;

        if (!*this) {
            return;
        }

        bucketCount = htonll(bucketCount);
        memcpy(file.Data(), &bucketCount, sizeof(bucketCount));
    }

    TPersistentImmutableHashMap::TPersistentImmutableHashMap(
        const std::string& path,
        uint32_t seed
    )
        : Seed(seed)
        , Path(path)
    {
        new (File_) TFile(Path, TFile::ACCESS_RDONLY);

        if (!*this) {
            return;
        }

        uint64_t tmp;
        memcpy(&tmp, File().Data(), sizeof(tmp));
        BucketCount = htonll(tmp);
    }

    TPersistentImmutableHashMap::~TPersistentImmutableHashMap() {
        File().~TFile();
    }

    uint64_t TPersistentImmutableHashMap::Bucket(const TBlob& key) const {
        absl::uint128 hash;
        static_assert(sizeof(hash) == (sizeof(uint64_t) * 2));

        MurmurHash3_x64_128(key.Data(), key.Size(), Seed, &hash);

        return (uint64_t)(hash % BucketCount);
        // std::cerr << "Bucket(" << (std::string)key << "): " << rv << std::endl;
        // return rv;
    }

    void TPersistentImmutableHashMap::Add(const TBlob& key, const TBlob& value) {
        if (!*this) {
            return;
        }

        auto* ptr = KeyPtr(key);
        size_t size = 0;

        {
            TValue tmp;
            memcpy(&tmp.Prev, ptr, sizeof(tmp));
            tmp.Prev = ntohll(tmp.Prev);
            tmp.Key.Wrap(key.Size(), key.Data());
            tmp.Value.Wrap(value.Size(), value.Data());

            size = tmp.Dump(File());
        }

        {
            uint64_t tmp(htonll(DataPos));
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
        if (!*this) {
            return TBlob();
        }

        const auto* ptr = KeyPtr(key);
        const size_t headerSize((sizeof(uint64_t) * (1 + BucketCount)));

        uint64_t tmp;
        memcpy(&tmp, ptr, sizeof(tmp));
        // std::cerr << ntohll(tmp) << std::endl;
        tmp = headerSize + ntohll(tmp);

        if (tmp > headerSize) {
            --tmp;

        } else {
            return TBlob();
        }

        // std::cerr << tmp << ", " << File().Size() << std::endl;

        while (true) {
            auto rv = TValue::Restore(File().Data() + tmp);

            if (
                (rv.Key.Size() == key.Size())
                && (memcmp(rv.Key.Data(), key.Data(), key.Size()) == 0)
            ) {
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
}
