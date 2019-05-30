#pragma once

#include <memory>

namespace NAC {
    class TWiredTigerTransactionImpl;

    class TWiredTigerTransaction {
    public:
        TWiredTigerTransaction() = delete;
        TWiredTigerTransaction(const TWiredTigerTransaction&);
        TWiredTigerTransaction(TWiredTigerTransaction&&);

        TWiredTigerTransaction(void*, size_t&, const char*);

        ~TWiredTigerTransaction();

        bool Commit(const char* options = nullptr);

    private:
        TWiredTigerTransactionImpl* Impl;
        bool Committed = false;
    };
}
