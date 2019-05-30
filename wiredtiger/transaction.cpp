#include "transaction.hpp"
#include <wiredtiger.h>
#include <stdlib.h>
#include <stdio.h>

namespace NAC {
    class TWiredTigerTransactionImpl {
    public:
        TWiredTigerTransactionImpl() = delete;
        TWiredTigerTransactionImpl(const TWiredTigerTransactionImpl&) = delete;
        TWiredTigerTransactionImpl(TWiredTigerTransactionImpl&&) = delete;

        TWiredTigerTransactionImpl(
            WT_SESSION* session,
            size_t& activeCounter,
            const char* options
        )
            : Session(session)
            , Active(activeCounter)
        {
            int result = Session->begin_transaction(Session, (options ? options : "isolation=snapshot"));

            if (result != 0) {
                dprintf(
                    2,
                    "Failed to begin transaction: %s\n",
                    wiredtiger_strerror(result)
                );

                abort();
            }
        }

        bool Commit(const char* options) {
            if (Active == 0) {
                dprintf(2, "[WiredTiger] Double commit attempted\n");
                return true;
            }

            --Active;

            if (Active == 0) {
                Committed = true;
                int result;

                if (Failed > 0) {
                    result = Session->rollback_transaction(Session, nullptr);

                } else {
                    result = Session->commit_transaction(Session, options);
                }

                if (result == 0) {
                    return (Failed == 0);

                } else {
                    dprintf(
                        2,
                        "Failed to %s transaction: %s\n",
                        ((Failed > 0) ? "rollback" : "commit"),
                        wiredtiger_strerror(result)
                    );

                    if (Failed > 0) {
                        abort();
                    }

                    return false;
                }
            }

            return (Failed == 0);
        }

        ~TWiredTigerTransactionImpl() {
            if (Committed) {
                return;
            }

            if (Active > 0) {
                dprintf(2, "[WiredTiger] Something went terribly wrong\n");
                abort();
            }

            int result = Session->rollback_transaction(Session, nullptr);

            if (result != 0) {
                dprintf(
                    2,
                    "Failed to rollback transaction: %s\n",
                    wiredtiger_strerror(result)
                );

                abort();
            }
        }

        void Ref() {
            ++Active;
        }

        bool UnRef(bool committed) {
            if (!committed) {
                --Active;
                ++Failed;
            }

            return (Active == 0);
        }

    private:
        WT_SESSION* Session;
        size_t& Active;
        size_t Failed = 0;
        bool Committed = false;
    };

    TWiredTigerTransaction::TWiredTigerTransaction(
        void* session,
        size_t& activeCounter,
        const char* options
    )
        : Impl(new TWiredTigerTransactionImpl((WT_SESSION*)session, activeCounter, options))
    {
        // Impl->Ref(); // this is not needed here, see TWiredTigerSessionImpl::Begin
    }

    TWiredTigerTransaction::TWiredTigerTransaction(const TWiredTigerTransaction& right)
        : Impl(right.Impl)
    {
        Impl->Ref();
    }

    TWiredTigerTransaction::TWiredTigerTransaction(TWiredTigerTransaction&& right)
        : Impl(right.Impl)
        , Committed(right.Committed)
    {
        right.Impl = nullptr;
    }

    bool TWiredTigerTransaction::Commit(const char* options) {
        if (Committed) {
            dprintf(2, "[WiredTiger] Double commit attempted\n");
            return true;
        }

        Committed = true;
        return Impl->Commit(options);
    }

    TWiredTigerTransaction::~TWiredTigerTransaction() {
        if (Impl && Impl->UnRef(Committed)) {
            delete Impl;
            Impl = nullptr;
        }
    }
}
