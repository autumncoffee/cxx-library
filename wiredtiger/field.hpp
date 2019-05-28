#pragma once

#include <string>
#include <functional>

namespace NAC {
    class TWiredTigerFieldBase {
    public:
        std::string Name;
        size_t Index;

    public:
        virtual std::string Format() const;
        virtual std::function<void*()> Load(void*) const;
        virtual size_t DumpedSize(void*, const void*) const;
        virtual void Dump(void*, const void*) const;
    };

    template<typename T>
    class TWiredTigerField : public TWiredTigerFieldBase {
    public:
        using TValue = T;

    public:
        TValue Value;
    };

    using TWiredTigerUIntField = TWiredTigerField<uint64_t>;
    using TWiredTigerStringField = TWiredTigerField<std::string>;
}
