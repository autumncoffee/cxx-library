#pragma once

#include "field.hpp"
#include <memory>
#include <vector>
#include <utility>
#include <functional>
#include <string>
#include <unordered_map>
#include <ac-common/str.hpp>

namespace {
    template<typename TKey, typename TValue>
    static const std::string& FieldNames() {
        thread_local static std::string out;

        if (out.empty()) {
            out = TKey::__ACModelGetFieldNameListStatic();

            if (!out.empty()) {
                out += ",";
            }

            out += TValue::__ACModelGetFieldNameListStatic();
        }

        return out;
    }
}

namespace NAC {
    template<typename TKey_, typename TValue_>
    class TWiredTigerModelDescr {
    public:
        using TKey = TKey_;
        using TValue = TValue_;

        static std::string DBName;

        static const std::string& FieldNames() {
            return ::FieldNames<TKey, TValue>();
        }
    };

    template<typename TWiredTigerModelDescr, typename TKey_>
    class TWiredTigerIndexDescr {
    public:
        using TModel = TWiredTigerModelDescr;
        using TKey = TKey_;

        static std::string DBName;
    };
}

using __TACModelFieldMap = std::unordered_map<std::string, size_t>;
using __TACModelFields = std::vector<std::unique_ptr<NAC::TWiredTigerFieldBase>>;
using __TACModelData = std::vector<std::function<void*()>>;

#define AC_MODEL_BEGIN(cls) \
class cls : public TWiredTigerModelBase { \
private: \
    using TSelf = cls; \
\
private: \
    static __TACModelFieldMap __ACModelFieldMap; \
    static __TACModelFields __ACModelFields; \
    __TACModelData __ACModelData; \
\
protected: \
    const __TACModelFieldMap& __GetACModelFieldMap() const override { \
        return __ACModelFieldMap; \
    } \
\
    const __TACModelFields& __GetACModelFields() const override { \
        return __ACModelFields; \
    } \
\
    __TACModelData& __GetACModelData() override { \
        return __ACModelData; \
    } \
\
    const __TACModelData& __GetACModelData() const override { \
        return __ACModelData; \
    } \
\
    void Init() { \
        __ACModelData.reserve(__ACModelFields.size()); \
\
        for (const auto& it : __ACModelFields) { \
            __ACModelData.emplace_back(__TACModelData::value_type()); \
        } \
    } \
\
public: \
    cls() { \
        Init(); \
    } \
\
    static const __TACModelFields& __GetACModelFieldsStatic() { \
        return __ACModelFields; \
    }

#define AC_MODEL_FIELD2(type, name, dbName) \
private: \
    static size_t __ACModelGet ## name ## Index() { \
        std::unique_ptr<TWiredTigerFieldBase> ptr(new type); \
        ptr->Name = #dbName; \
        size_t index = ptr->Index = __ACModelFields.size(); \
\
        __ACModelFields.emplace_back(std::move(ptr)); \
        __ACModelFieldMap.emplace(#name, index); \
\
        return index; \
    } \
\
    static const size_t __ACModelFieldIndex ## name; \
\
public: \
    const type::TValue& Get ## name() const { \
        if (auto* ptr = (const type::TValue*)__ACModelGet(__ACModelFieldIndex ## name)) { \
            return *ptr; \
\
        } else { \
            static type::TValue dummy; \
            return dummy; \
        } \
    } \
\
    TSelf& Set ## name(const type::TValue& val) { \
        __ACModelSet(__ACModelFieldIndex ## name, [val]() { \
            return (void*)&val; \
        }); \
\
        return *this; \
    }

#define AC_MODEL_FIELD(type, name) AC_MODEL_FIELD2(type, name, name)

#define AC_MODEL_FIELD_IMPL(cls, name) \
    const size_t cls::__ACModelFieldIndex ## name = cls::__ACModelGet ## name ## Index();

#define AC_MODEL_END() \
private: \
    static std::string __ACModelBuildFullFormat() { \
        std::string out; \
\
        for (const auto& it : __ACModelFields) { \
            out += it->Format(); \
        } \
\
        return out; \
    } \
\
    static const std::string __ACModelFullFormat; \
\
    static std::string __ACModelBuildFieldNameList() { \
        std::string out; \
\
        for (const auto& it : __ACModelFields) { \
            out += it->Name + ","; \
        } \
\
        if (out.size() > 0) { \
            out.resize(out.size() - 1); \
        } \
\
        return out; \
    } \
\
    static const std::string __ACModelFieldNameList; \
\
protected: \
    const std::string& __ACModelGetFullFormat() const override { \
        return __ACModelFullFormat; \
    } \
\
public: \
    static const std::string& __ACModelGetFullFormatStatic() { \
        return __ACModelFullFormat; \
    } \
\
    static const std::string& __ACModelGetFieldNameListStatic() { \
        return __ACModelFieldNameList; \
    } \
};

#define AC_MODEL_IMPL_START(cls) \
    __TACModelFieldMap cls::__ACModelFieldMap; \
    __TACModelFields cls::__ACModelFields;

#define AC_MODEL_IMPL_END(cls) \
    const std::string cls::__ACModelFullFormat = cls::__ACModelBuildFullFormat(); \
    const std::string cls::__ACModelFieldNameList = cls::__ACModelBuildFieldNameList();

namespace NAC {
    class TWiredTigerModelBase {
    public:
        void Load(void*, size_t, const void*);
        TBlob Dump(void*) const;

    protected:
        virtual const __TACModelFieldMap& __GetACModelFieldMap() const = 0;
        virtual const __TACModelFields& __GetACModelFields() const = 0;
        virtual const std::string& __ACModelGetFullFormat() const = 0;
        virtual __TACModelData& __GetACModelData() = 0;
        virtual const __TACModelData& __GetACModelData() const = 0;

        void* __ACModelGet(size_t i) const {
            if (auto&& cb = __GetACModelData().at(i)) {
                return cb();

            } else {
                return nullptr;
            }
        }

        void __ACModelSet(size_t i, std::function<void*()>&& item) {
            __GetACModelData()[i] = std::move(item);
        }
    };
}
