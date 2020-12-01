#pragma once

#include <string>

namespace NAC {
    class TModelFieldBase {
    public:
        std::string Name;
        size_t Index;

        virtual ~TModelFieldBase() {
        }
    };

    template<typename T>
    class TModelField : public TModelFieldBase {
    public:
        using TValue = T;

    public:
        TValue Value;
    };
}
