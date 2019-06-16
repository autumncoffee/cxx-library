#pragma once

#include <utility>
#include <ac-common/str.hpp>

namespace NAC {
    class TRBTreeBase {
    public:
        virtual ~TRBTreeBase() {
        }

        void FindRoot();

        void Insert(const TBlob& key);
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

        void Insert(size_t key, const TBlob& value) {
            Insert((uint64_t)key, value);
        }

        void Insert(const char* key) {
            Insert(TBlob(strlen(key), key));
        }

        void Insert(const std::string& key) {
            Insert(TBlob(key.size(), key.data()));
        }

        void Insert(uint64_t key);
        void Insert(uint32_t key);
        void Insert(uint16_t key);

        void Insert(uint8_t key) {
            Insert(TBlob(sizeof(key), (char*)&key));
        }

        void Insert(size_t key) {
            Insert((uint64_t)key);
        }

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

        template<typename T>
        TBlob operator[](T&& key) const {
            return Get(std::forward<T>(key));
        }

    private:
        virtual void Reserve(size_t size) = 0;
        virtual char* Data() const = 0;
        virtual size_t Size() const = 0;

    private:
        size_t RootOffset = 0;
    };

    template<typename TBackend, typename TOps>
    class TRBTree : public TRBTreeBase {
    public:
        template<typename... TArgs>
        TRBTree(TArgs&&... args)
            : Backend(std::forward<TArgs>(args)...)
        {
        }

    private:
        void Reserve(size_t size) override {
            TOps::Reserve(Backend, size);
        }

        char* Data() const override {
            return TOps::Data(Backend);
        }

        size_t Size() const override {
            return TOps::Size(Backend);
        }

    private:
        TBackend Backend;
    };
}
