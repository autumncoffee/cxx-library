#pragma once

#include "rbtree.hpp"

namespace NAC {
    class TInMemRBTreeOps;
    using TInMemRBTree = TRBTree<TBlob, TInMemRBTreeOps>;

    class TInMemRBTreeOps {
    public:
        static inline void Reserve(TBlob& backend, size_t size) {
            backend.Reserve(backend.Capacity() + size);
        }

        static inline char* Data(const TBlob& backend) {
            return backend.Data();
        }

        static inline size_t Size(const TBlob& backend) {
            return backend.Capacity();
        }
    };
}
