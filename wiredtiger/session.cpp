#include "session.hpp"
#include <wiredtiger.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <utility>

namespace NAC {
    class TWiredTigerSessionImpl {
    public:
        TWiredTigerSessionImpl() = delete;
        TWiredTigerSessionImpl(const TWiredTigerSessionImpl&) = delete;
        TWiredTigerSessionImpl(TWiredTigerSessionImpl&&) = delete;

        TWiredTigerSessionImpl(WT_SESSION* session) {
            Session = std::shared_ptr<WT_SESSION>(session, [](WT_SESSION* session) {
                int result = session->close(session, nullptr);

                if (result != 0) {
                    dprintf(
                        2,
                        "Failed to close session: %s\n",
                        wiredtiger_strerror(result)
                    );

                    abort();
                }
            });
        }

        void CreateTable(
            const std::string& dbName,
            const std::string& keyFormat,
            const std::string& valueFormat,
            const std::string& fieldNames
        ) {
            int result = Session->create(
                Session.get(),
                ("table:" + dbName).c_str(),
                ("key_format=" + keyFormat + ",value_format=" + valueFormat + ",columns=(" + fieldNames + ")").c_str()
            );

            if (result != 0) {
                dprintf(
                    2,
                    "Failed to create table: %s\n",
                    wiredtiger_strerror(result)
                );

                abort();
            }
        }

        void CreateIndex(
            const std::string& tableName,
            const std::string& indexName,
            const std::string& keyFieldNames
        ) {
            int result = Session->create(
                Session.get(),
                ("index:" + tableName + ":" + indexName).c_str(),
                ("columns=(" + keyFieldNames + ")").c_str()
            );

            if (result != 0) {
                dprintf(
                    2,
                    "Failed to create index: %s\n",
                    wiredtiger_strerror(result)
                );

                abort();
            }
        }

    private:
        std::shared_ptr<WT_CURSOR> Cursor(
            const std::string& dbName,
            const char* const options = nullptr
        ) {
            WT_CURSOR* cursor;
            int result = Session->open_cursor(
                Session.get(),
                dbName.data(),
                nullptr,
                options,
                &cursor
            );

            if (result != 0) {
                dprintf(
                    2,
                    "Error opening cursor: %s\n",
                    wiredtiger_strerror(result)
                );

                abort();
            }

            return std::shared_ptr<WT_CURSOR>(cursor, [sess = Session](WT_CURSOR* cursor) {
                cursor->close(cursor);
            });
        }

        bool Insert(
            const std::string& dbName,
            const TWiredTigerModelBase& key_,
            const TWiredTigerModelBase& value_,
            const char* options
        ) {
            auto cursor = Cursor(("table:" + dbName), options);
            auto keyBlob = key_.Dump((void*)Session.get());
            auto valueBlob = value_.Dump((void*)Session.get());

            WT_ITEM key {
                .data = keyBlob.Data(),
                .size = keyBlob.Size(),
            };

            WT_ITEM value {
                .data = valueBlob.Data(),
                .size = valueBlob.Size(),
            };

            cursor->set_key(cursor.get(), &key);
            cursor->set_value(cursor.get(), &value);

            return (cursor->insert(cursor.get()) == 0);
        }

    public:
        bool Insert(
            const std::string& dbName,
            const TWiredTigerModelBase& key,
            const TWiredTigerModelBase& value
        ) {
            return Insert(dbName, key, value, "raw,overwrite=false");
        }

        bool Set(
            const std::string& dbName,
            const TWiredTigerModelBase& key,
            const TWiredTigerModelBase& value
        ) {
            return Insert(dbName, key, value, "raw,overwrite=true");
        }

        bool Append(
            const std::string& dbName,
            TWiredTigerModelBase& key_,
            const TWiredTigerModelBase& value_
        ) {
            auto cursor = Cursor(("table:" + dbName), "raw,overwrite=false,append=true");
            auto valueBlob = value_.Dump((void*)Session.get());

            WT_ITEM value {
                .data = valueBlob.Data(),
                .size = valueBlob.Size(),
            };

            cursor->set_value(cursor.get(), &value);

            int result = cursor->insert(cursor.get());

            if (result == 0) {
                WT_ITEM key;
                result = cursor->get_key(cursor.get(), &key);

                if (result == 0) {
                    key_.Load((void*)Session.get(), key.size, key.data);

                    return true;
                }

            } else {
                dprintf(
                    2,
                    "Error appending record: %s\n",
                    wiredtiger_strerror(result)
                );
            }

            return false;
        }

        bool Get(
            const std::string& dbName,
            const TWiredTigerModelBase& key_,
            TWiredTigerModelBase& out
        ) {
            auto cursor = Cursor(("table:" + dbName), "raw");
            auto keyBlob = key_.Dump((void*)Session.get());
            WT_ITEM key {
                .data = keyBlob.Data(),
                .size = keyBlob.Size(),
            };

            cursor->set_key(cursor.get(), &key);

            int result = cursor->search(cursor.get());

            if (result == 0) {
                WT_ITEM value;
                result = cursor->get_value(cursor.get(), &value);

                if (result == 0) {
                    out.Load((void*)Session.get(), value.size, value.data);

                    return true;
                }

            } else if (result != WT_NOTFOUND) {
                dprintf(
                    2,
                    "Failed to get key: %s\n",
                    wiredtiger_strerror(result)
                );
            }

            return false;
        }

        TWiredTigerIterator Search(
            const std::string& tableName,
            const std::string& indexName,
            const std::string& valueFieldNames,
            const TWiredTigerModelBase& key_,
            int direction
        ) {
            auto cursor = Cursor(("index:" + tableName + ":" + indexName + "(" + valueFieldNames + ")"), "raw");
            auto keyBlob = key_.Dump((void*)Session.get());
            WT_ITEM key {
                .data = keyBlob.Data(),
                .size = keyBlob.Size(),
            };

            cursor->set_key(cursor.get(), &key);

            return TWiredTigerIterator(std::shared_ptr<void>(cursor, (void*)cursor.get()), direction, std::move(keyBlob));
        }

        bool Remove(
            const std::string& dbName,
            const TWiredTigerModelBase& key_
        ) {
            auto cursor = Cursor(("table:" + dbName), "raw,overwrite=true");
            auto keyBlob = key_.Dump((void*)Session.get());
            WT_ITEM key {
                .data = keyBlob.Data(),
                .size = keyBlob.Size(),
            };

            cursor->set_key(cursor.get(), &key);

            return (cursor->remove(cursor.get()) == 0);
        }

    private:
        std::shared_ptr<WT_SESSION> Session;
    };

    TWiredTigerSession::TWiredTigerSession(void* ptr)
        : Impl(new TWiredTigerSessionImpl((WT_SESSION*)ptr))
    {
    }

    TWiredTigerSession::~TWiredTigerSession() {
        delete Impl;
    }

    void TWiredTigerSession::CreateTable(
        const std::string& dbName,
        const std::string& keyFormat,
        const std::string& valueFormat,
        const std::string& fieldNames
    ) {
        Impl->CreateTable(dbName, keyFormat, valueFormat, fieldNames);
    }

    void TWiredTigerSession::CreateIndex(
        const std::string& tableName,
        const std::string& indexName,
        const std::string& keyFieldNames
    ) {
        Impl->CreateIndex(tableName, indexName, keyFieldNames);
    }

    bool TWiredTigerSession::Insert(
        const std::string& dbName,
        const TWiredTigerModelBase& key,
        const TWiredTigerModelBase& data
    ) {
        return Impl->Insert(dbName, key, data);
    }

    bool TWiredTigerSession::Set(
        const std::string& dbName,
        const TWiredTigerModelBase& key,
        const TWiredTigerModelBase& data
    ) {
        return Impl->Set(dbName, key, data);
    }

    bool TWiredTigerSession::Append(
        const std::string& dbName,
        TWiredTigerModelBase& key,
        const TWiredTigerModelBase& data
    ) {
        return Impl->Append(dbName, key, data);
    }

    bool TWiredTigerSession::Get(
        const std::string& dbName,
        const TWiredTigerModelBase& key,
        TWiredTigerModelBase& out
    ) {
        return Impl->Get(dbName, key, out);
    }

    TWiredTigerIterator TWiredTigerSession::Search(
        const std::string& tableName,
        const std::string& indexName,
        const std::string& valueFieldNames,
        const TWiredTigerModelBase& key,
        int direction
    ) {
        return Impl->Search(tableName, indexName, valueFieldNames, key, direction);
    }

    bool TWiredTigerSession::Remove(
        const std::string& dbName,
        const TWiredTigerModelBase& key
    ) {
        return Impl->Remove(dbName, key);
    }
}
