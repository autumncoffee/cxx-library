#pragma once

#include "session.hpp"
#include <string>

namespace NAC {
    class TWiredTigerImpl;

    class TWiredTiger {
    public:
        TWiredTiger() = delete;
        TWiredTiger(const std::string& path, const char* options = nullptr);

        TWiredTiger(const TWiredTiger&) = delete;
        TWiredTiger(TWiredTiger&& right) {
            Impl = right.Impl;
            right.Impl = nullptr;
        }

        ~TWiredTiger();

        TWiredTigerSession Open() const;

    private:
        TWiredTigerImpl* Impl;
    };
}
