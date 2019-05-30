#pragma once

#include <memory>
#include "model.hpp"
#include <ac-common/str.hpp>

namespace NAC {
    class TWiredTigerIteratorImpl;

    class TWiredTigerIterator {
    public:
        TWiredTigerIterator() = delete;
        TWiredTigerIterator(const std::shared_ptr<void>&, int, TBlob&&);

        TWiredTigerIterator(const TWiredTigerIterator&) = delete;
        TWiredTigerIterator(TWiredTigerIterator&& right) {
            Impl = right.Impl;
            right.Impl = nullptr;
        }

        ~TWiredTigerIterator();

        bool Next(TWiredTigerModelBase& value);
        bool Next(TWiredTigerModelBase& key, TWiredTigerModelBase& value);

    private:
        TWiredTigerIteratorImpl* Impl;
    };
}
