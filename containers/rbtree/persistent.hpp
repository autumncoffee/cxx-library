#pragma once

#include "rbtree.hpp"
#include <ac-common/file.hpp>
#include <stdexcept>

namespace NAC {
    class TPersistentRBTreeOps;
    using TPersistentRBTree = TRBTree<TFile, TPersistentRBTreeOps>;

    class TPersistentRBTreeOps {
    public:
        static inline void Reserve(TFile& backend, size_t size) {
            backend.Resize(backend.Size() + size);

            if (!backend) {
                throw std::runtime_error("File backend failed");
            }
        }

        static inline char* Data(const TFile& backend) {
            return backend.Data();
        }

        static inline size_t Size(const TFile& backend) {
            return backend.Size();
        }
    };
}
