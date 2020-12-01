#pragma once

#include <ac-library/models/model.hpp>
#include "field.hpp"
#include <utility>
#include <vector>
#include <string>
#include <ac-common/str.hpp>

namespace {
    template<typename TKey, typename TValue>
    static const std::string& FieldNames() {
        thread_local static std::string out;

        if (out.empty()) {
            out = TKey::ACWTModelGetFieldNameListStatic();

            if (!out.empty()) {
                out += ",";
            }

            out += TValue::ACWTModelGetFieldNameListStatic();
        }

        return out;
    }
}

namespace NAC {
    template<typename TKey, typename TValue>
    class TWiredTigerModelDescr : public TModelDescr<TKey, TValue> {
    public:
        static const std::string& FieldNames() {
            return ::FieldNames<TKey, TValue>();
        }
    };

    template<typename TWiredTigerModelDescr, typename TKey_>
    class TWiredTigerIndexDescr : public TModelIndexDescr<TWiredTigerModelDescr, TKey_> {
    };

    using TWTModelData = std::vector<std::function<void*()>>;
}

#define AC_WT_MODEL_BEGIN(cls) \
    AC_MODEL_BEGIN(cls, TWiredTigerModelBase) \
\
private: \
    NAC::TWTModelData ACWTModelData_; \
\
private: \
    NAC::TWTModelData& GetACWTModelData() override { \
        return ACWTModelData_; \
    } \
\
    const NAC::TWTModelData& GetACWTModelData() const override { \
        return ACWTModelData_; \
    } \
\
protected: \
    void ACWTModelInit() { \
        ACWTModelData_.reserve(GetACModelFieldsStatic().size()); \
\
        for (const auto& it : GetACModelFieldsStatic()) { \
            ACWTModelData_.emplace_back(NAC::TWTModelData::value_type()); \
        } \
    } \
\
public: \
    cls() { \
        ACWTModelInit(); \
    }

#define AC_WT_MODEL_FIELD2(type, name, dbName) AC_MODEL_FIELD2(type, name, dbName)

#define AC_WT_MODEL_FIELD(type, name) AC_MODEL_FIELD(type, name)

#define AC_WT_MODEL_FIELD_IMPL(cls, name) AC_MODEL_FIELD_IMPL(cls, name)

#define AC_WT_MODEL_END() \
private: \
    static std::string ACWTModelBuildFullFormat() { \
        std::string out; \
\
        for (const auto& it : GetACModelFieldsStatic()) { \
            out += ((const TWiredTigerFieldBase*)it.get())->Format(); \
        } \
\
        return out; \
    } \
\
    static const std::string ACWTModelFullFormat_; \
\
    static std::string ACWTModelBuildFieldNameList() { \
        std::string out; \
\
        for (const auto& it : GetACModelFieldsStatic()) { \
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
    static const std::string ACWTModelFieldNameList_; \
\
protected: \
    const std::string& ACWTModelGetFullFormat() const override { \
        return ACWTModelFullFormat_; \
    } \
\
public: \
    static const std::string& ACWTModelGetFullFormatStatic() { \
        return ACWTModelFullFormat_; \
    } \
\
    static const std::string& ACWTModelGetFieldNameListStatic() { \
        return ACWTModelFieldNameList_; \
    } \
\
    AC_MODEL_END()

#define AC_WT_MODEL_IMPL_START(cls) AC_MODEL_IMPL_START(cls)

#define AC_WT_MODEL_IMPL_END(cls) \
    const std::string cls::ACWTModelFullFormat_ = cls::ACWTModelBuildFullFormat(); \
    const std::string cls::ACWTModelFieldNameList_ = cls::ACWTModelBuildFieldNameList(); \
\
    AC_MODEL_IMPL_END(cls)

namespace NAC {
    class TWiredTigerModelBase : public TModelBase {
    public:
        void Load(void*, size_t, const void*);
        TBlob Dump(void*) const;

    protected:
        virtual const std::string& ACWTModelGetFullFormat() const = 0;
        virtual NAC::TWTModelData& GetACWTModelData() = 0;
        virtual const NAC::TWTModelData& GetACWTModelData() const = 0;

        void* ACModelGet(size_t i) const override {
            if (auto&& cb = GetACWTModelData().at(i)) {
                return cb();

            } else {
                return nullptr;
            }
        }

        void ACModelSet(size_t i, std::function<void*()>&& item) override {
            GetACWTModelData()[i] = std::move(item);
        }
    };
}
