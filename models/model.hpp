#pragma once

#include "field.hpp"
#include <memory>
#include <vector>
#include <utility>
#include <functional>
#include <string>
#include <unordered_map>

namespace NAC {
    class TModelDescrBase {
    public:
        virtual ~TModelDescrBase() {
        }
    };

    template<typename TKey_, typename TValue_>
    class TModelDescr : public TModelDescrBase {
    public:
        using TKey = TKey_;
        using TValue = TValue_;

        static std::string DBName;
    };

    class TModelIndexDescrBase {
    public:
        virtual ~TModelIndexDescrBase() {
        }
    };

    template<typename TModelDescr, typename TKey_>
    class TModelIndexDescr : public TModelIndexDescrBase {
    public:
        using TModel = TModelDescr;
        using TKey = TKey_;

        static std::string DBName;
    };

    using TModelFieldMap = std::unordered_map<std::string, size_t>;
    using TModelFields = std::vector<std::unique_ptr<NAC::TModelFieldBase>>;
    using TModelData = std::vector<std::function<void*()>>;
}

#define AC_MODEL_BEGIN(cls, parent) \
class cls : public parent { \
    static_assert(std::is_base_of_v<NAC::TModelBase, parent>); \
\
private: \
    using TSelf = cls; \
\
private: \
    static NAC::TModelFieldMap ACModelFieldMap_; \
    static NAC::TModelFields ACModelFields_; \
    NAC::TModelData ACModelData_; \
\
protected: \
    const NAC::TModelFieldMap& GetACModelFieldMap() const override { \
        return ACModelFieldMap_; \
    } \
\
    const NAC::TModelFields& GetACModelFields() const override { \
        return ACModelFields_; \
    } \
\
    NAC::TModelData& GetACModelData() override { \
        return ACModelData_; \
    } \
\
    const NAC::TModelData& GetACModelData() const override { \
        return ACModelData_; \
    } \
\
    void ACModelInit() { \
        ACModelData_.reserve(ACModelFields_.size()); \
\
        for (const auto& it : ACModelFields_) { \
            ACModelData_.emplace_back(NAC::TModelData::value_type()); \
        } \
    } \
\
public: \
    cls() { \
        ACModelInit(); \
    } \
\
    static const NAC::TModelFields& GetACModelFieldsStatic() { \
        return ACModelFields_; \
    }

#define AC_MODEL_FIELD2(type, name, dbName) \
private: \
    static size_t ACModelInitField ## name () { \
        std::unique_ptr<NAC::TModelFieldBase> ptr(new type); \
        ptr->Name = #dbName; \
        size_t index = ptr->Index = ACModelFields_.size(); \
\
        ACModelFields_.emplace_back(std::move(ptr)); \
        ACModelFieldMap_.emplace(#name, index); \
\
        return index; \
    } \
\
    static const size_t ACModelFieldIndex ## name ## _; \
\
public: \
    const type::TValue& Get ## name() const { \
        if (auto* ptr = (const type::TValue*)ACModelGet(ACModelFieldIndex ## name ## _)) { \
            return *ptr; \
\
        } else { \
            static type::TValue dummy; \
            return dummy; \
        } \
    } \
\
    TSelf& Set ## name(const type::TValue& val) { \
        ACModelSet(ACModelFieldIndex ## name ## _, [val]() { \
            return (void*)&val; \
        }); \
\
        return *this; \
    }

#define AC_MODEL_FIELD(type, name) AC_MODEL_FIELD2(type, name, name)

#define AC_MODEL_FIELD_IMPL(cls, name) \
    const size_t cls::ACModelFieldIndex ## name ## _ = cls::ACModelInitField ## name ();

#define AC_MODEL_END() \
};

#define AC_MODEL_IMPL_START(cls) \
    NAC::TModelFieldMap cls::ACModelFieldMap_; \
    NAC::TModelFields cls::ACModelFields_;

#define AC_MODEL_IMPL_END(cls)

namespace NAC {
    class TModelBase {
    public:
        virtual ~TModelBase() {
        }

    protected:
        virtual const NAC::TModelFieldMap& GetACModelFieldMap() const = 0;
        virtual const NAC::TModelFields& GetACModelFields() const = 0;
        virtual NAC::TModelData& GetACModelData() = 0;
        virtual const NAC::TModelData& GetACModelData() const = 0;

        void* ACModelGet(size_t i) const {
            if (auto&& cb = GetACModelData().at(i)) {
                return cb();

            } else {
                return nullptr;
            }
        }

        void ACModelSet(size_t i, std::function<void*()>&& item) {
            GetACModelData()[i] = std::move(item);
        }
    };
}
