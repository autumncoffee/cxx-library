#pragma once

#include <ac-common/file.hpp>
#include <ac-common/str.hpp>
#include <string>

namespace NAC {
    class TPersistentBinaryHeap {
    public:
        class TIterator {
        public:
            TIterator() = default;
            TIterator(const TPersistentBinaryHeap* heap, size_t pos, size_t prefixSize);

            void ToMin();
            bool Next(TBlob& value);

            explicit operator bool() const {
                return (bool)Heap && (bool)(*Heap);
            }

        private:
            const TPersistentBinaryHeap* Heap = nullptr;
            TBlob Prefix;
            size_t Pivot = 0;
            size_t Pos = 0;
            int Direction = -1;
        };

    public:
        TPersistentBinaryHeap(const std::string& path);
        TPersistentBinaryHeap(const std::string& path, uint64_t recordSize);

        ~TPersistentBinaryHeap();

        void Insert(const void* item);

        void Insert(const TBlob& item) {
            Insert(item.Data());
        }

        bool Close();

        TIterator GetAll(const TBlob& prefix = TBlob()) const;

        TBlob Get(const TBlob& prefix) const {
            TBlob out;
            GetAll(prefix).Next(out);

            return out;
        }

        uint64_t RecordSize() const {
            return RecordSize_;
        }

        size_t Size() const {
            if (RecordSize_ == 0) {
                return 0;
            }

            return (File().Size() - sizeof(RecordSize_)) / RecordSize_;
        }

        const char* operator[](size_t index) const {
            if (index >= Size()) {
                return nullptr;
            }

            return File().Data() + sizeof(RecordSize_) + (RecordSize_ * index);
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

    private:
        std::string Path;
        uint64_t RecordSize_ = 0;
        alignas(TFile) char File_[sizeof(TFile)];
    };
}
