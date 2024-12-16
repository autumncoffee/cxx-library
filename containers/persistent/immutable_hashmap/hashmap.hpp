#pragma once

#include <ac-common/file.hpp>
#include <ac-common/str.hpp>
#include <cstdint>
#include <utility>

namespace NAC {
    class TPersistentImmutableHashMap {
    public:
        static const uint32_t DefaultSeed = 42;

        class TIterator {
        public:
            TIterator() = default;
            TIterator(char* ptr, uint64_t bucketCount);

            bool Next(TBlob& key, TBlob& value);

            bool Next(uint64_t& key, TBlob& value);
            bool Next(uint32_t& key, TBlob& value);
            bool Next(uint16_t& key, TBlob& value);

            bool Next(uint8_t& key, TBlob& value) {
                TBlob tmp;

                if (Next(tmp, value)) {
                    memcpy(&key, tmp.Data(), sizeof(key));

                    return true;
                }

                return false;
            }

            explicit operator bool() const {
                return (bool)Ptr;
            }

        private:
            const char* BucketPtr(uint64_t bucket) const {
                return Ptr + (sizeof(uint64_t) * (1 + bucket));
            }

        private:
            char* Ptr = nullptr;
            uint64_t BucketCount_ = 0;
            uint64_t CurrentBucket = 0;
            uint64_t Offset = 0;
        };

    public:
        TPersistentImmutableHashMap() = delete;

        // Create mode
        TPersistentImmutableHashMap(
            const std::string& path,
            uint64_t bucketCount,
            uint32_t seed
        );

        // Read/Append mode
        TPersistentImmutableHashMap(
            const std::string& path,
            uint32_t seed,
            bool rw = false
        );

        ~TPersistentImmutableHashMap();

        uint64_t BucketCount() const {
            return BucketCount_;
        }

        void Insert(const TBlob& key, const TBlob& value);

        void Insert(const char* key, const TBlob& value) {
            Insert(TBlob(strlen(key), key), value);
        }

        void Insert(const std::string& key, const TBlob& value) {
            Insert(TBlob(key.size(), key.data()), value);
        }

        void Insert(uint64_t key, const TBlob& value);
        void Insert(uint32_t key, const TBlob& value);
        void Insert(uint16_t key, const TBlob& value);

        void Insert(uint8_t key, const TBlob& value) {
            Insert(TBlob(sizeof(key), (char*)&key), value);
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

        template<typename T>
        TBlob operator[](T&& key) const {
            return Get(std::forward<T>(key));
        }

        explicit operator bool() const {
            return (bool)File();
        }

        TIterator GetAll() const;

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
        uint64_t BucketCount_;
        uint32_t Seed;
        std::string Path;
        alignas(TFile) char File_[sizeof(TFile)];
        size_t DataPos = 1;
    };
}
