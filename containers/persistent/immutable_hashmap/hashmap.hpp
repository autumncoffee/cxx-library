#pragma once

#include <ac-common/file.hpp>
#include <ac-common/str.hpp>
#include <string.h>

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

        static const uint32_t DefaultSeed = 42;

    public:
        TPersistentImmutableHashMap() = delete;

        // Write mode
        TPersistentImmutableHashMap(
            const std::string& path,
            uint64_t bucketCount,
            uint32_t seed
        );

        // Read mode
        TPersistentImmutableHashMap(
            const std::string& path,
            uint32_t seed
        );

        ~TPersistentImmutableHashMap();

        void Add(const TBlob& key, const TBlob& value);

        void Add(const char* key, const TBlob& value) {
            Add(TBlob(strlen(key), key), value);
        }

        void Add(const std::string& key, const TBlob& value) {
            Add(TBlob(key.size(), key.data()), value);
        }

        void Add(uint64_t key, const TBlob& value);
        void Add(uint32_t key, const TBlob& value);
        void Add(uint16_t key, const TBlob& value);

        void Add(uint8_t key, const TBlob& value) {
            Add(TBlob(sizeof(key), (char*)&key), value);
        }

        void Add(size_t key, const TBlob& value) {
            Add((uint64_t)key, value);
        }

        bool Close();

        TBlob Get(const TBlob& key) const;

        TBlob Get(const char* key) const {
            return Get(TBlob(strlen(key), key));
        }

        TBlob Get(const std::string& key) const {
            return Get(TBlob(key.size(), key.data()));
        }

        TBlob Get(uint64_t key) const;
        TBlob Get(uint32_t key) const;
        TBlob Get(uint16_t key) const;

        TBlob Get(uint8_t key) const {
            return Get(TBlob(sizeof(key), (char*)&key));
        }

        TBlob Get(size_t key) const {
            return Get((uint64_t)key);
        }

        explicit operator bool() const {
            return (bool)File();
        }

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
        size_t DataPos = 1;
    };
}
