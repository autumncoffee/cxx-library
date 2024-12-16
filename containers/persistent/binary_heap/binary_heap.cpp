#include "binary_heap.hpp"
#include <ac-common/utils/htonll.hpp>
#include <cstdint>
#include <stdio.h>
#include <string.h>
#include <iterator>
#include <algorithm>

namespace NAC {
    TPersistentBinaryHeap::TPersistentBinaryHeap(const std::string& path)
        : Path(path)
    {
        new (File_) TFile(path, TFile::ACCESS_RDONLY);

        if (!*this) {
            return;
        }

        memcpy(&RecordSize_, File().Data(), sizeof(RecordSize_));
        RecordSize_ = ntoh(RecordSize_);
    }

    TPersistentBinaryHeap::TPersistentBinaryHeap(
        const std::string& path,
        uint64_t recordSize
    )
        : Path(path)
        , RecordSize_(recordSize)
    {
        new (File_) TFile(path + ".new", TFile::ACCESS_CREATE);

        if (!*this) {
            return;
        }

        recordSize = hton(recordSize);
        File().Append(sizeof(recordSize), (char*)&recordSize);
    }

    TPersistentBinaryHeap::~TPersistentBinaryHeap() {
        File().~TFile();
    }

    void TPersistentBinaryHeap::Insert(const void* item) {
        File().Append(RecordSize_, (const char*)item);
    }

    TPersistentBinaryHeap::TIterator TPersistentBinaryHeap::GetAll(const TBlob& prefix) const {
        if (!*this) {
            return TIterator();
        }

        const size_t size(Size());

        if (size == 0) {
            return TIterator();
        }

        if (size == 1) {
            const char* ptr = (*this)[0];

            if (ptr && (prefix.Cmp(prefix.Size(), ptr) == 0)) {
                return TIterator(this, 0, prefix.Size());
            }

            return TIterator();
        }

        size_t left = 0;
        size_t right = size;

        while (right >= left) {
            const size_t pivot = (right - ((double)(right - left) / 2.0));

            if (pivot >= size) {
                return TIterator();
            }

            const char* ptr = (*this)[pivot];
            const int rv = 0 - prefix.Cmp(prefix.Size(), ptr);

            if (rv == 0) {
                return TIterator(this, pivot, prefix.Size());

            } else if (rv < 0) {
                left = pivot + 1;

            } else {
                if (pivot == 0) {
                    return TIterator();
                }

                right = pivot - 1;
            }
        }

        return TIterator();
    }

    TPersistentBinaryHeap::TIterator::TIterator(const TPersistentBinaryHeap* heap, size_t pos, size_t prefixSize)
        : Heap(heap)
        , Prefix(prefixSize, (*heap)[pos], /* own = */false)
        , Pivot(pos)
        , Pos(pos)
    {
    }

    bool TPersistentBinaryHeap::TIterator::Next(TBlob& value) {
        if (!*this) {
            return false;
        }

        const char* ptr = (*Heap)[Pos];

        if (!ptr) {
            Heap = nullptr;
            return false;
        }

        if (Prefix.Cmp(Prefix.Size(), ptr) == 0) {
            value.Wrap(Heap->RecordSize(), ptr, /* own = */false);

            if ((Pos == 0) && (Direction < 0)) {
                Direction *= -1;
                Pos = Pivot + Direction;

            } else {
                Pos += Direction;
            }

            return true;

        } else if (Direction < 0) {
            Direction *= -1;
            Pos = Pivot + Direction;

            return Next(value);

        } else {
            Heap = nullptr;
        }

        return false;
    }

    void TPersistentBinaryHeap::TIterator::ToMin() {
        if (!*this) {
            return;
        }

        if (Prefix.Size() == 0) {
            Pos = 0;

        } else {
            if (Direction > 0) {
                Direction *= -1;

                if (Pos > Pivot) {
                    Pos = Pivot;
                }
            }

            while (true) {
                const char* ptr = (*Heap)[Pos];

                if (!ptr) {
                    Heap = nullptr;
                    return;
                }

                if (Prefix.Cmp(Prefix.Size(), ptr) == 0) {
                    if (Pos == 0) {
                        break;
                    }

                    Pos += Direction;

                } else {
                    Pos -= Direction;
                    break;
                }
            }
        }

        if (Direction < 0) {
            Direction *= -1;
        }
    }

    namespace NPrivate {
        class TSortIterator {
        public:
            class TItem {
            public:
                TItem(uint64_t recordSize, char* ptr) noexcept
                    : RecordSize(recordSize)
                    , Ptr(ptr)
                {
                }

                TItem(const TItem& right) = delete;

                TItem(TItem&& right) noexcept
                    : RecordSize(right.RecordSize)
                    , Kludge(true)
                {
                    /*
                        TItem tmp(std::move(a));
                        a = std::move(b);
                        b = std::move(tmp);

                        So here we save original value from "a"
                        so it won't be overridden on first move-assignment
                        and can later be transferred to "b".

                        This should be called rarely.
                    */
                    Ptr = (char*)malloc(RecordSize);
                    memcpy(Ptr, right.Ptr, RecordSize);
                }

                ~TItem() {
                    if (Kludge) {
                        free(Ptr);
                    }
                }

                TItem& operator=(const TItem& right) = delete;

                TItem& operator=(TItem&& right) noexcept {
                    memcpy(Ptr, right.Ptr, RecordSize);

                    return *this;
                }

                bool operator<(const TItem& right) const noexcept {
                    return memcmp(Ptr, right.Ptr, RecordSize) < 0;
                }

                bool operator>(const TItem& right) const noexcept {
                    return memcmp(Ptr, right.Ptr, RecordSize) > 0;
                }

                bool operator==(const TItem& right) const noexcept {
                    return memcmp(Ptr, right.Ptr, RecordSize) == 0;
                }

                bool operator!=(const TItem& right) const noexcept {
                    return memcmp(Ptr, right.Ptr, RecordSize) != 0;
                }

            public:
                uint64_t RecordSize = 0;
                char* Ptr = nullptr;

            private:
                bool Kludge = false;
            };

            using TDifference = int64_t;

            using difference_type = TDifference;
            using value_type = TItem;
            using pointer = TItem*;
            using reference = TItem&;
            using iterator_category = std::random_access_iterator_tag;

        public:
            TSortIterator(uint64_t recordSize, char* ptr) noexcept
                : Item(recordSize, ptr)
            {
            }

            TSortIterator(const TSortIterator& right) noexcept
                : Item(right.Item.RecordSize, (char*)right.Item.Ptr)
            {
            }

            TSortIterator& operator=(const TSortIterator& right) noexcept {
                Item.Ptr = (char*)right.Item.Ptr;
                Item.RecordSize = right.Item.RecordSize;

                return *this;
            }

            TSortIterator(TSortIterator&& right) noexcept
                : Item(right.Item.RecordSize, right.Item.Ptr)
            {
            }

            TSortIterator& operator=(TSortIterator&& right) noexcept {
                Item.Ptr = right.Item.Ptr;
                Item.RecordSize = right.Item.RecordSize;

                return *this;
            }

            TItem operator[](size_t i) const noexcept {
                return TItem(Item.RecordSize, Item.Ptr + (Item.RecordSize * i));
            }

            TSortIterator operator+(TDifference n) const noexcept {
                return TSortIterator(Item.RecordSize, Item.Ptr + (Item.RecordSize * n));
            }

            TSortIterator& operator+=(TDifference n) noexcept {
                Item.Ptr += (Item.RecordSize * n);
                return *this;
            }

            TSortIterator operator-(TDifference n) const noexcept {
                return TSortIterator(Item.RecordSize, Item.Ptr - (Item.RecordSize * n));
            }

            TSortIterator& operator-=(TDifference n) noexcept {
                Item.Ptr -= (Item.RecordSize * n);
                return *this;
            }

            TDifference operator-(const TSortIterator& right) const noexcept {
                return (Item.Ptr - right.Item.Ptr) / Item.RecordSize;
            }

            TSortIterator& operator++() noexcept {
                Item.Ptr += Item.RecordSize;
                return *this;
            }

            TSortIterator operator++(int) const noexcept {
                return TSortIterator(Item.RecordSize, Item.Ptr + Item.RecordSize);
            }

            TSortIterator& operator--() noexcept {
                Item.Ptr -= Item.RecordSize;
                return *this;
            }

            TSortIterator operator--(int) const noexcept {
                return TSortIterator(Item.RecordSize, Item.Ptr - Item.RecordSize);
            }

            TItem& operator*() noexcept {
                return Item;
            }

            TItem* operator->() noexcept {
                return &Item;
            }

            bool operator<(const TSortIterator& right) const noexcept {
                return Item.Ptr < right.Item.Ptr;
            }

            bool operator<=(const TSortIterator& right) const noexcept {
                return Item.Ptr <= right.Item.Ptr;
            }

            bool operator>(const TSortIterator& right) const noexcept {
                return Item.Ptr > right.Item.Ptr;
            }

            bool operator>=(const TSortIterator& right) const noexcept {
                return Item.Ptr >= right.Item.Ptr;
            }

            bool operator==(const TSortIterator& right) const noexcept {
                return Item.Ptr == right.Item.Ptr;
            }

            bool operator!=(const TSortIterator& right) const noexcept {
                return Item.Ptr != right.Item.Ptr;
            }

        private:
            TItem Item;
        };

        static inline void SwapCharPtr(char* a, char* b, size_t size) noexcept {
            for (size_t i = 0; i < size; ++i) {
                a[i] ^= b[i];
                b[i] = (a[i] ^ b[i]);
                a[i] ^= b[i];
            }
        }

        void swap(TSortIterator::TItem& a, TSortIterator::TItem& b) noexcept {
            SwapCharPtr(a.Ptr, b.Ptr, a.RecordSize);
        }
    }

    bool TPersistentBinaryHeap::Close() {
        if (!*this) {
            return false;
        }

        // File().MSync();

        File().Stat();
        File().Map();

        if (!File()) {
            return false;
        }

        if (Size() > 1) {
            using namespace NPrivate;

            auto begin = TSortIterator(RecordSize_, File().Data() + sizeof(RecordSize_));
            auto end = TSortIterator(RecordSize_, File().Data() + File().Size());

            std::sort(begin, end);
        }

        if (rename(File().Path().c_str(), Path.c_str()) == 0) {
            File().~TFile();
            new (File_) TFile(Path, TFile::ACCESS_RDONLY);
            return (bool)*this;

        } else {
            perror("rename");
            return false;
        }
    }
}
