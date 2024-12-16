#include "rbtree.hpp"
#include <ac-common/utils/htonll.hpp>
#include <cstdint>
// #include <iostream>
// #include <unordered_set>

#define TRACE(x) //std::cerr << x << std::endl;
#define EXT_TRACE(x) TRACE(x)

#define NODE_INT_ACCESSOR(name, idx) \
inline uint64_t name() const { \
    return Data[idx]; \
} \
\
inline void name(uint64_t val) { \
    Data[idx] = val; \
}

#define ADD_INT_KEY(type) \
void TRBTreeBase::Insert(type key, const TBlob& value) { \
    auto tmp = hton((uint64_t)key); \
    Insert(TBlob(sizeof(tmp), (char*)&tmp), value); \
} \
\
void TRBTreeBase::Insert(type key) { \
    auto tmp = hton((uint64_t)key); \
    Insert(TBlob(sizeof(tmp), (char*)&tmp)); \
}

#define GET_INT_KEY(type) TBlob TRBTreeBase::Get(type key) const { \
    auto tmp = hton(key); \
    return Get(TBlob(sizeof(tmp), (char*)&tmp)); \
}

namespace {
    using namespace NAC;

    class TRBTreeNode {
    private:
        static const uint64_t Flag = 0x1000000000000000u;

    private:
        char* Ptr = nullptr;
        uint64_t Data[4] = {0, 0, 0, 0};
        bool IsRed_ = true;

    public:
        inline TRBTreeNode() = default;

        inline TRBTreeNode(char* ptr) {
            Load(ptr);
        }

        inline TRBTreeNode(const char* ptr) {
            Load(ptr);
        }

        static inline size_t Size() {
            return sizeof(Data);
        }

        inline void Load(char* ptr) {
            Ptr = ptr;
            memcpy(Data, Ptr, sizeof(Data));

            // const auto x = Data[0];

            Data[0] = ntoh(Data[0]);
            Data[1] = ntoh(Data[1]);
            Data[2] = ntoh(Data[2]);
            Data[3] = ntoh(Data[3]);

            if (Data[0] & Flag) {
                Data[0] ^= Flag;
                IsRed_ = true;

            } else {
                IsRed_ = false;
            }

            EXT_TRACE("Load(" << (uint64_t)ptr << "): " << Data[0] << "(" << x << ")" << ", " << (uint64_t)Data[1] << ", " << (uint64_t)Data[2] << ", " << (uint64_t)Data[3] << ", red=" << IsRed_);
        }

        inline void Load(const char* ptr) {
            Load((char*)ptr);
        }

        inline void Dump(char* ptr = nullptr) {
            if (!Ptr && ptr) {
                Ptr = ptr;
            }

            uint64_t tmp[4];
            tmp[0] = Data[0];

            if (IsRed_) {
                tmp[0] |= Flag;
            }

            tmp[0] = hton(tmp[0]);
            tmp[1] = hton(Data[1]);
            tmp[2] = hton(Data[2]);
            tmp[3] = hton(Data[3]);

            memcpy((ptr ? ptr : Ptr), tmp, sizeof(tmp));

            EXT_TRACE("Save(" << (uint64_t)(ptr ? ptr : Ptr) << "): " << Data[0] << "(" << tmp[0] << ")" << ", " << (uint64_t)Data[1] << ", " << (uint64_t)Data[2] << ", " << (uint64_t)Data[3] << ", red=" << IsRed_);
        }

        inline void Dump(const char* ptr) {
            Dump((char*)ptr);
        }

        inline bool IsRed() const {
            return IsRed_;
        }

        inline void PaintBlack() {
            IsRed_ = false;
        }

        inline void PaintRed() {
            IsRed_ = true;
        }

        NODE_INT_ACCESSOR(Parent, 0);
        NODE_INT_ACCESSOR(Left, 1);
        NODE_INT_ACCESSOR(Right, 2);
        NODE_INT_ACCESSOR(ValueOffset, 3);

        inline TBlob Key() const {
            uint64_t tmp;
            memcpy(&tmp, Ptr + Size(), sizeof(tmp));

            return TBlob(ntoh(tmp), Ptr + Size() + sizeof(tmp));
        }
    };

    static inline TRBTreeNode Root(const char* const ptr, size_t& offset) {
        TRBTreeNode root(ptr + offset);

        while (root.Parent() > 0) {
            TRACE("root(" << offset << ")");
            offset = root.Parent() - 1;
            root.Load(ptr + offset);
        }

        return root;
    }

    static inline bool InsertImpl(
        char* const ptr,
        size_t& rootOffset,
        TRBTreeNode& root,
        const size_t dataOffset,
        TRBTreeNode& node,
        const TBlob& key
    ) {
        // static std::unordered_set<size_t> validOffsets;
        // std::unordered_set<size_t> seen;
        //
        // validOffsets.insert(0);
        // validOffsets.insert(dataOffset);

        while (true) {
            const int rv = key.Cmp(root.Key());

            // if (seen.count(rootOffset) > 0) {
            //     throw std::runtime_error(std::to_string(rootOffset));
            // }
            //
            // seen.insert(rootOffset);

            if (rv < 0) {
                if (root.Left() > 0) {
                    rootOffset = root.Left() - 1;
                    root.Load(ptr + rootOffset);
                    TRACE("going left to " << rootOffset << "...");
                    // if (validOffsets.count(rootOffset) == 0) {
                    //     throw std::runtime_error("Unknown offset: " + std::to_string(rootOffset));
                    // }
                    // if (root.Parent() == 0) {
                    //     throw std::runtime_error("WTF");
                    // }
                    continue;

                } else {
                    root.Left(1 + dataOffset);
                    TRACE("new left: " << dataOffset);
                }

            } else if (rv > 0) {
                if (root.Right() > 0) {
                    // const auto x = rootOffset;
                    rootOffset = root.Right() - 1;
                    root.Load(ptr + rootOffset);
                    TRACE("going right to " << rootOffset << "...");
                    // if (validOffsets.count(rootOffset) == 0) {
                    //     throw std::runtime_error("Unknown offset: " + std::to_string(rootOffset));
                    // }
                    // if (root.Parent() == 0) {
                    //     throw std::runtime_error("WTF");
                    // }
                    continue;

                } else {
                    root.Right(1 + dataOffset);
                    TRACE("new right: " << dataOffset);
                }

            } else {
                TRACE("overwrite");
                root.ValueOffset(node.ValueOffset());
                root.Dump();
                return false;
            }

            node.Parent(1 + rootOffset);
            node.Dump(ptr + dataOffset);

            return true;
        }
    }

    class TRotateLeftOps {
    public:
        static inline size_t Right(const TRBTreeNode& node) {
            return node.Right();
        }

        static inline void Right(TRBTreeNode& node, size_t value) {
            node.Right(value);
        }

        static inline size_t Left(const TRBTreeNode& node) {
            return node.Left();
        }

        static inline void Left(TRBTreeNode& node, size_t value) {
            node.Left(value);
        }
    };

    class TRotateRightOps {
    public:
        static inline size_t Right(const TRBTreeNode& node) {
            return node.Left();
        }

        static inline void Right(TRBTreeNode& node, size_t value) {
            node.Left(value);
        }

        static inline size_t Left(const TRBTreeNode& node) {
            return node.Right();
        }

        static inline void Left(TRBTreeNode& node, size_t value) {
            node.Right(value);
        }
    };

    template<typename TOps>
    static inline void Rotate(
        char* const ptr,
        TRBTreeNode& root,
        const size_t dataOffset,
        TRBTreeNode& node
    ) {
        const size_t rightOffset(TOps::Right(node));

        if (rightOffset == 0) {
            return;
        }

        TRBTreeNode right(ptr + rightOffset - 1);

        TOps::Right(node, TOps::Left(right));
        TOps::Left(right, 1 + dataOffset);

        {
            const size_t newRightOffset(TOps::Right(node));

            if (newRightOffset > 0) {
                TRBTreeNode newRight(ptr + newRightOffset - 1);
                newRight.Parent(1 + dataOffset);
                newRight.Dump();
            }
        }

        if (node.Parent() > 0) {
            if ((dataOffset + 1) == root.Left()) {
                root.Left(rightOffset);

            } else {
                root.Right(rightOffset);
            }

            root.Dump();
        }

        right.Parent(node.Parent());
        right.Dump();

        node.Parent(rightOffset);
        node.Dump();
    }
}

namespace NAC {
    void TRBTreeBase::Insert(const TBlob& key) {
        static const TBlob empty;

        Insert(key, empty);
    }

    void TRBTreeBase::Insert(const TBlob& key, const TBlob& value) {
        if (key.Size() == 0) {
            return;
        }

        TRACE("---===>>> insert <<<===---");

        size_t dataSize(TRBTreeNode::Size() + sizeof(uint64_t) + key.Size());

        if (value.Size() > 0) {
            dataSize += sizeof(uint64_t) + value.Size();
        }

        Reserve(dataSize);

        size_t dataOffset(Size() - dataSize);
        auto* const ptr = Data();
        auto* dataStart = ptr + dataOffset;

        TRACE("dataOffset=" << dataOffset);

        uint64_t tmp(hton(key.Size()));
        memcpy(dataStart + TRBTreeNode::Size(), &tmp, sizeof(tmp));
        memcpy(dataStart + TRBTreeNode::Size() + sizeof(tmp), key.Data(), key.Size());

        if (value.Size() > 0) {
            tmp = hton(value.Size());
            memcpy(dataStart + TRBTreeNode::Size() + sizeof(tmp) + key.Size(), &tmp, sizeof(tmp));
            memcpy(dataStart + TRBTreeNode::Size() + (sizeof(tmp) * 2) + key.Size(), value.Data(), value.Size());
        }

        TRBTreeNode node;

        if (value.Size() > 0) {
            node.ValueOffset(1 + dataOffset + TRBTreeNode::Size() + sizeof(tmp) + key.Size());
        }

        if (dataOffset == 0) {
            node.PaintBlack();
            node.Dump(dataStart);
            TRACE("insert first");
            return;
        }

        auto root = Root(ptr, RootOffset);
        size_t rootOffset(RootOffset);

        if (!InsertImpl(ptr, rootOffset, root, dataOffset, node, key)) {
            TRACE("update existing value");
            return;
        }

        if (!root.IsRed() || (root.Parent() == 0)) {
            root.Dump();
            TRACE("insert case 1/2");
            return;
        }

        TRBTreeNode parentRoot;

        while (true) {
            size_t parentRootOffset(root.Parent() - 1);
            parentRoot.Load(ptr + parentRootOffset);
            tmp = 0;

            if (parentRoot.Left() == node.Parent()) {
                tmp = parentRoot.Right();

            } else {
                tmp = parentRoot.Left();
            }

            if (tmp > 0) {
                TRBTreeNode uncle(ptr + tmp - 1);

                if (uncle.IsRed()) {
                    TRACE("insert case 3 [1]");
                    root.PaintBlack();
                    root.Dump();

                    uncle.PaintBlack();
                    uncle.Dump();

                    if (parentRoot.Parent() == 0) {
                        if (parentRoot.IsRed()) {
                            parentRoot.PaintBlack();
                            parentRoot.Dump();
                        }

                        TRACE("insert case 3 [2]");
                        break;
                    }

                    if (!parentRoot.IsRed()) {
                        parentRoot.PaintRed();
                        parentRoot.Dump();
                    }

                    rootOffset = parentRoot.Parent() - 1;
                    root.Load(ptr + rootOffset);

                    if (!root.IsRed() || (root.Parent() == 0)) {
                        TRACE("insert case 3 [3]");
                        break;
                    }

                    node = parentRoot;
                    dataOffset = parentRootOffset;
                    TRACE("insert case 3 [4]");

                    continue;
                }
            }

            TRACE("insert case 4 [1]");
            bool reloadRoots(false);

            if (
                ((dataOffset + 1) == root.Right())
                && (node.Parent() == parentRoot.Left())
            ) {
                TRACE("insert case 4 - rotate left [1]");
                Rotate<TRotateLeftOps>(ptr, parentRoot, rootOffset, root);
                node.Load(ptr + dataOffset);

                if (node.Left() > 0) {
                    dataOffset = node.Left() - 1;
                    node.Load(ptr + dataOffset);
                    reloadRoots = true;

                } else {
                    break;
                }

            } else if (
                ((dataOffset + 1) == root.Left())
                && (node.Parent() == parentRoot.Right())
            ) {
                TRACE("insert case 4 - rotate right [1]");
                Rotate<TRotateRightOps>(ptr, parentRoot, rootOffset, root);
                node.Load(ptr + dataOffset);

                if (node.Right() > 0) {
                    dataOffset = node.Right() - 1;
                    node.Load(ptr + dataOffset);
                    reloadRoots = true;

                } else {
                    break;
                }
            }

            if (reloadRoots) {
                if (node.Parent() > 0) {
                    rootOffset = node.Parent() - 1;
                    root.Load(ptr + rootOffset);

                    if (root.Parent() > 0) {
                        parentRootOffset = root.Parent() - 1;
                        parentRoot.Load(ptr + parentRootOffset);
                    }
                }

            } else {
                root.Dump();
            }

            if ((node.Parent() == 0) || (root.Parent() == 0)) {
                TRACE("insert case 4 [2]");
                break;
            }

            if (parentRoot.Parent() > 0) {
                node.Load(ptr + parentRoot.Parent() - 1);
            }

            if ((dataOffset + 1) == root.Left()) {
                TRACE("insert case 4 - rotate right [2]");
                Rotate<TRotateRightOps>(ptr, node, parentRootOffset, parentRoot);

            } else {
                TRACE("insert case 4 - rotate left [2]");
                Rotate<TRotateLeftOps>(ptr, node, parentRootOffset, parentRoot);
            }

            if (root.IsRed()) {
                root.Load(ptr + rootOffset);
                root.PaintBlack();
                root.Dump();
            }

            if (!parentRoot.IsRed()) {
                parentRoot.Load(ptr + parentRootOffset);
                parentRoot.PaintRed();
                parentRoot.Dump();
            }

            TRACE("insert case 4 [3]");

            break;
        }

        TRACE("look for new root");

        Root(ptr, RootOffset); // find new root
    }

    TBlob TRBTreeBase::Get(const TBlob& key) const {
        if (Size() == 0) {
            return TBlob();
        }

        const auto* const ptr = Data();
        TRBTreeNode root(ptr + RootOffset);

        while (true) {
            const int rv = key.Cmp(root.Key());

            if (rv < 0) {
                if (root.Left() > 0) {
                    root.Load(ptr + root.Left() - 1);
                    TRACE("going left...");
                    continue;

                } else {
                    TRACE("less, but have nothing");
                    return TBlob();
                }

            } else if (rv > 0) {
                if (root.Right() > 0) {
                    root.Load(ptr + root.Right() - 1);
                    TRACE("going right...");
                    continue;

                } else {
                    TRACE("greater, but have nothing");
                    return TBlob();
                }

            } else if (root.ValueOffset() > 0) {
                TRACE("have value!");
                uint64_t tmp;
                memcpy(&tmp, ptr + root.ValueOffset() - 1, sizeof(tmp));

                return TBlob(ntoh(tmp), ptr + root.ValueOffset() - 1 + sizeof(tmp));

            } else {
                TRACE("found, but have no value");
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

    void TRBTreeBase::FindRoot() {
        Root(Data(), RootOffset);
    }

    TRBTreeBase::TIterator::TIterator(const char* ptr, size_t offset, const TBlob& prefix)
        : Ptr(ptr)
        , RootOffset(offset)
        , Prefix(prefix.Size(), prefix.Data())
    {
        CurrentOffset = Descend(RootOffset, RootOffset);
        TRACE("TIterator done");
    }

    bool TRBTreeBase::TIterator::Next(TBlob& key, TBlob& value) {
        if (!Ptr) {
            return false;
        }

        TRBTreeNode node(Ptr + CurrentOffset);
        key = node.Key();

        if (node.ValueOffset() > 0) {
            uint64_t tmp;
            memcpy(&tmp, Ptr + node.ValueOffset() - 1, sizeof(tmp));

            value.Wrap(ntoh(tmp), Ptr + node.ValueOffset() - 1 + sizeof(tmp));

        } else {
            value.Reset();
        }

        if ((RootOffset == CurrentOffset) || (node.Parent() == 0)) {
            Ptr = nullptr;
            return true;
        }

        size_t rootOffset(node.Parent() - 1);
        TRBTreeNode root(Ptr + rootOffset);

        while (RootOffset != CurrentOffset) {
            if (((CurrentOffset + 1) == root.Left()) && (root.Right() > 0)) {
                CurrentOffset = Descend(root.Right() - 1, rootOffset);

            } else {
                CurrentOffset = rootOffset;
            }

            if (!Prefix || (CurrentOffset != rootOffset)) {
                break;
            }

            TRACE("next: StartsWith(" << (std::string)root.Key() << ", " << (std::string)Prefix << ")");

            if (root.Key().StartsWith(Prefix)) {
                break;

            } else if (root.Parent() == 0) {
                Ptr = nullptr;
                break;

            } else {
                rootOffset = root.Parent() - 1;
                root.Load(Ptr + rootOffset);
            }
        }

        return true;
    }

    size_t TRBTreeBase::TIterator::Descend(size_t offset, size_t rv) const {
        size_t tmp(offset);
        TRBTreeNode root(Ptr + tmp);

        if (Prefix) {
            size_t lastOkRight(0);
            bool isLeft(false);

            while (true) {
                const int cmp(root.Key().PrefixCmp(Prefix));

                TRACE("descend: cmp(" << (std::string)root.Key() << ", " << (std::string)Prefix << ") == " << cmp << ", offset=" << tmp);

                if (cmp < 0) {
                    if (root.Right() > 0) {
                        tmp = root.Right() - 1;

                    } else if (isLeft && (lastOkRight > 0)) {
                        tmp = lastOkRight - 1;
                        isLeft = false;

                    } else {
                        break;
                    }

                } else if (cmp > 0) {
                    if (root.Left() > 0) {
                        tmp = root.Left() - 1;

                    } else if (isLeft && (lastOkRight > 0)) {
                        tmp = lastOkRight - 1;
                        isLeft = false;

                    } else {
                        break;
                    }

                } else {
                    rv = tmp;

                    if (root.Left() > 0) {
                        tmp = root.Left() - 1;
                        isLeft = true;
                        lastOkRight = root.Right();

                    } else if (root.Right() > 0) {
                        tmp = root.Right() - 1;
                        isLeft = false;

                    } else {
                        break;
                    }
                }

                root.Load(Ptr + tmp);
            }

        } else {
            while (true) {
                rv = tmp;

                if (root.Left() > 0) {
                    tmp = root.Left() - 1;

                } else if (root.Right() > 0) {
                    tmp = root.Right() - 1;

                } else {
                    break;
                }

                root.Load(Ptr + tmp);
            }
        }

        TRACE("descend returns " << rv);

        return rv;
    }

    TRBTreeBase::TIterator TRBTreeBase::GetAll() const {
        if (Size() == 0) {
            return TIterator();
        }

        return TIterator(Data(), RootOffset);
    }

    TRBTreeBase::TIterator TRBTreeBase::GetAll(const TBlob& prefix) const {
        if (Size() == 0) {
            return TIterator();
        }

        const auto* const ptr = Data();
        size_t currentOffset(RootOffset);
        TRBTreeNode root(ptr + currentOffset);

        while (true) {
            auto key = root.Key();
            const int rv = key.PrefixCmp(prefix);

            if (rv < 0) {
                if (root.Right() > 0) {
                    currentOffset = root.Right() - 1;
                    root.Load(ptr + currentOffset);
                    TRACE("going right...");
                    continue;

                } else {
                    TRACE("greater, but have nothing");
                    return TIterator();
                }

            } else if (rv > 0) {
                if (root.Left() > 0) {
                    currentOffset = root.Left() - 1;
                    root.Load(ptr + currentOffset);
                    TRACE("going left...");
                    continue;

                } else {
                    TRACE("less, but have nothing");
                    return TIterator();
                }

            } else {
                TRACE("found");
                return TIterator(ptr, currentOffset, TBlob(prefix.Size(), key.Data()));
            }
        }
    }
}
