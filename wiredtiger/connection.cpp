#include "connection.hpp"
#include <wiredtiger.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

namespace NAC {
    class TWiredTigerImpl {
    public:
        TWiredTigerImpl() = delete;
        TWiredTigerImpl(const TWiredTigerImpl&) = delete;
        TWiredTigerImpl(TWiredTigerImpl&&) = delete;

        TWiredTigerImpl(const std::string& path, const char* options) {
            WT_CONNECTION* db;
            int result = wiredtiger_open(
                path.c_str(),
                nullptr,
                (options ? options : "create,cache_size=5GB,eviction=(threads_max=20,threads_min=5),eviction_dirty_target=30,eviction_target=40,eviction_trigger=60,session_max=10000,log=(enabled=true,file_max=100MB)"),
                &db
            );

            if (result != 0) {
                dprintf(2, "Error opening database: %s\n", wiredtiger_strerror(result));
                abort();
            }

            Db = std::shared_ptr<WT_CONNECTION>(db, [](WT_CONNECTION* db) {
                int result = db->close(db, nullptr);

                if (result != 0) {
                    dprintf(
                        2,
                        "Failed to close database: %s\n",
                        wiredtiger_strerror(result)
                    );

                    abort();
                }
            });
        }

        TWiredTigerSession Open() const {
            WT_SESSION* session;
            int result = Db->open_session(
                Db.get(),
                nullptr,
                nullptr,
                &session
            );

            if (result != 0) {
                dprintf(
                    2,
                    "Failed to open session: %s\n",
                    wiredtiger_strerror(result)
                );

                abort();
            }

            return TWiredTigerSession(session);
        }

    private:
        std::shared_ptr<WT_CONNECTION> Db;
    };

    TWiredTiger::TWiredTiger(const std::string& path, const char* options)
        : Impl(new TWiredTigerImpl(path, options))
    {
    }

    TWiredTiger::~TWiredTiger() {
        delete Impl;
    }

    TWiredTigerSession TWiredTiger::Open() const {
        return Impl->Open();
    }
}
