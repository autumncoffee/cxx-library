#pragma once

#include <ac-common/file.hpp>
#include <ac-common/str.hpp>

namespace NAC {
    class TPersistentImmutableHashMap {
    public:
        struct TValue {
            TBlob Key;
            TBlob Value;
            uint64_t Prev = 0;

            size_t Dump(TFile&) const;

            static TValue Restore(char*);
        };

    public:
        TPersistentImmutableHashMap() = delete;

        TPersistentImmutableHashMap(
            const std::string& path,
            uint64_t bucketCount,
            uint32_t seed = 42
        );

        void Add(const TBlob& key, const TBlob& value);
        void Close();

        TBlob Get(const TBlob& key) const;

    private:
        TFile& File() {
            return *(TFile*)File_;
        }

        const TFile& File() const {
            return *(const TFile*)File_;
        }

        uint64_t Bucket(const TBlob& key) const;

        char* KeyPtr(const TBlob& key) {
            return File().Data() + (sizeof(uint64_t) * (1 + Bucket(key)));
        }

        const char* KeyPtr(const TBlob& key) const {
            return File().Data() + (sizeof(uint64_t) * (1 + Bucket(key)));
        }

    private:
        uint64_t BucketCount;
        uint32_t Seed;
        std::string Path;
        char File_[sizeof(TFile)];
        size_t DataPos = 0;
    };
}
