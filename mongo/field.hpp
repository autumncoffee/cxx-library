#pragma once

#include <ac-library/models/field.hpp>
#include <cstdint>
#include <string>
#include <functional>

namespace NAC {
    class TMongoFieldBase : public TModelFieldBase {
    public:
        virtual std::function<void*()> Load(const void*) const = 0;
    };

    template<typename T>
    class TMongoField : public TMongoFieldBase {
    public:
        using TValue = T;

    public:
        TValue Value;
    };

    class TMongoInt64Field : public TMongoField<int64_t> {
    public:
        std::function<void*()> Load(const void*) const override;
    };

    class TMongoStringField : public TMongoField<std::string> {
    public:
        std::function<void*()> Load(const void*) const override;
    };

    class TMongoBoolField : public TMongoField<bool> {
    public:
        std::function<void*()> Load(const void*) const override;
    };

    class TMongoOIDField : public TMongoStringField {
    public:
        std::function<void*()> Load(const void*) const override;
    };
}
