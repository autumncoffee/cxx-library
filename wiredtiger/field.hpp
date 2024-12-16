#pragma once

#include <ac-library/models/field.hpp>
#include <cstdint>
#include <string>
#include <functional>

namespace NAC {
    class TWiredTigerFieldBase : public TModelFieldBase {
    public:
        virtual std::string Format() const = 0;
        virtual std::function<void*()> Load(void*) const = 0;
        virtual size_t DumpedSize(void*, const void*) const = 0;
        virtual void Dump(void*, const void*) const = 0;
    };

    template<typename T>
    class TWiredTigerField : public TWiredTigerFieldBase {
    public:
        using TValue = T;

    public:
        TValue Value;
    };

    class TWiredTigerUIntField : public TWiredTigerField<uint64_t> {
    public:
        std::string Format() const override;
        std::function<void*()> Load(void*) const override;
        size_t DumpedSize(void*, const void*) const override;
        void Dump(void*, const void*) const override;
    };

    class TWiredTigerAutoincrementField : public TWiredTigerUIntField {
    public:
        std::string Format() const override;
    };

    class TWiredTigerStringField : public TWiredTigerField<std::string> {
    public:
        std::string Format() const override;
        std::function<void*()> Load(void*) const override;
        size_t DumpedSize(void*, const void*) const override;
        void Dump(void*, const void*) const override;
    };
}
