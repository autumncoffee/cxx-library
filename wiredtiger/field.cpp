#include "field.hpp"
#include <wiredtiger.h>
#include <stdio.h>

namespace NAC {
    std::string TWiredTigerFieldBase::Format() const {
        if (const auto* ptr = dynamic_cast<const TWiredTigerStringField*>(this)) {
            return std::string("S");
        }

        if (const auto* ptr = dynamic_cast<const TWiredTigerUIntField*>(this)) {
            return std::string("Q");
        }

        return std::string();
    }

    std::function<void*()> TWiredTigerFieldBase::Load(void* stream) const {
        if (const auto* ptr = dynamic_cast<const TWiredTigerStringField*>(this)) {
            const char* data;
            int result = wiredtiger_unpack_str((WT_PACK_STREAM*)stream, &data);

            if (result == 0) {
                return [val = std::string(data)](){
                    return (void*)&val;
                };

            } else {
                dprintf(
                    2,
                    "Failed to unpack string: %s\n",
                    wiredtiger_strerror(result)
                );
            }
        }

        if (const auto* ptr = dynamic_cast<const TWiredTigerUIntField*>(this)) {
            uint64_t val;
            int result = wiredtiger_unpack_uint((WT_PACK_STREAM*)stream, &val);

            if (result == 0) {
                return [val](){
                    return (void*)&val;
                };

            } else {
                dprintf(
                    2,
                    "Failed to unpack uint: %s\n",
                    wiredtiger_strerror(result)
                );
            }
        }

        return std::function<void*()>();
    }

    size_t TWiredTigerFieldBase::DumpedSize(void* session, const void* value) const {
        size_t out = 0;

        if (const auto* ptr = dynamic_cast<const TWiredTigerStringField*>(this)) {
            wiredtiger_struct_size((WT_SESSION*)session, &out, "S", ((const std::string*)value)->data());
        }

        if (const auto* ptr = dynamic_cast<const TWiredTigerUIntField*>(this)) {
            wiredtiger_struct_size((WT_SESSION*)session, &out, "Q", *(const uint64_t*)value);
        }

        return out;
    }

    void TWiredTigerFieldBase::Dump(void* stream, const void* value) const {
        if (const auto* ptr = dynamic_cast<const TWiredTigerStringField*>(this)) {
            wiredtiger_pack_str((WT_PACK_STREAM*)stream, ((const std::string*)value)->data());
        }

        if (const auto* ptr = dynamic_cast<const TWiredTigerUIntField*>(this)) {
            wiredtiger_pack_uint((WT_PACK_STREAM*)stream, *(const uint64_t*)value);
        }
    }
}
