#include "iterator.hpp"
#include <wiredtiger.h>
#include <stdio.h>
#include <utility>

namespace NAC {
    class TWiredTigerIteratorImpl {
    public:
        TWiredTigerIteratorImpl() = delete;
        TWiredTigerIteratorImpl(const TWiredTigerIteratorImpl&) = delete;
        TWiredTigerIteratorImpl(TWiredTigerIteratorImpl&&) = delete;

        TWiredTigerIteratorImpl(const std::shared_ptr<void>& data, int direction, TBlob&& key)
            : Cursor(data, (WT_CURSOR*)data.get())
            , Direction(direction)
            , Key(std::move(key))
        {
        }

        bool Next(TWiredTigerModelBase& out) {
            int result;

            if (First) {
                First = false;

                if (Direction == 0) {
                    result = Cursor->search(Cursor.get());

                } else {
                    int exact;
                    result = Cursor->search_near(Cursor.get(), &exact);

                    if (result == 0) {
                        if ((Direction < 0) && (exact > 0)) {
                            result = Cursor->prev(Cursor.get());

                        } else if ((Direction > 0) && (exact < 0)) {
                            result = Cursor->next(Cursor.get());
                        }
                    }
                }

            } else {
                if (Direction < 0) {
                    result = Cursor->prev(Cursor.get());

                } else {
                    result = Cursor->next(Cursor.get());
                }
            }

            if (result == 0) {
                WT_ITEM value;
                result = Cursor->get_value(Cursor.get(), &value);

                if (result == 0) {
                    out.Load((void*)Cursor->session, value.size, value.data);

                    return true;

                } else {
                    dprintf(
                        2,
                        "Failed to unpack record: %s\n",
                        wiredtiger_strerror(result)
                    );
                }

            } else if (result != WT_NOTFOUND) {
                dprintf(
                    2,
                    "Failed to get next record: %s\n",
                    wiredtiger_strerror(result)
                );
            }

            return false;
        }

    private:
        std::shared_ptr<WT_CURSOR> Cursor;
        int Direction;
        TBlob Key;
        bool First = true;
    };

    TWiredTigerIterator::TWiredTigerIterator(const std::shared_ptr<void>& data, int direction, TBlob&& key)
        : Impl(new TWiredTigerIteratorImpl(data, direction, std::move(key)))
    {
    }

    TWiredTigerIterator::~TWiredTigerIterator() {
        delete Impl;
    }

    bool TWiredTigerIterator::Next(TWiredTigerModelBase& out) {
        return Impl->Next(out);
    }
}
