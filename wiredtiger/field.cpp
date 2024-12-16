#include "field.hpp"
#include <wiredtiger.h>
#include <cstdint>
#include <stdio.h>

namespace NAC {
    std::string TWiredTigerUIntField::Format() const {
        return std::string("Q");
    }

    std::string TWiredTigerAutoincrementField::Format() const {
        return std::string("r");
    }

    std::string TWiredTigerStringField::Format() const {
        return std::string("S");
    }

    std::function<void*()> TWiredTigerUIntField::Load(void* stream) const {
        uint64_t val;
        int result = wiredtiger_unpack_uint((WT_PACK_STREAM*)stream, &val);

        if (result == 0) {
            return [val](){
                return (void*)&val;
            };
        }

        dprintf(2, "Failed to unpack uint: %s\n", wiredtiger_strerror(result));

        return std::function<void*()>();
    }

    std::function<void*()> TWiredTigerStringField::Load(void* stream) const {
        const char* data;
        int result = wiredtiger_unpack_str((WT_PACK_STREAM*)stream, &data);

        if (result == 0) {
            return [val = std::string(data)](){
                return (void*)&val;
            };
        }

        dprintf(2, "Failed to unpack string: %s\n", wiredtiger_strerror(result));

        return std::function<void*()>();
    }

    size_t TWiredTigerUIntField::DumpedSize(void* session, const void* value) const {
        size_t out = 0;
        wiredtiger_struct_size((WT_SESSION*)session, &out, "Q", *(const uint64_t*)value);
        return out;
    }

    size_t TWiredTigerStringField::DumpedSize(void* session, const void* value) const {
        size_t out = 0;
        wiredtiger_struct_size((WT_SESSION*)session, &out, "S", ((const std::string*)value)->data());
        return out;
    }

    void TWiredTigerUIntField::Dump(void* stream, const void* value) const {
        wiredtiger_pack_uint((WT_PACK_STREAM*)stream, *(const uint64_t*)value);
    }

    void TWiredTigerStringField::Dump(void* stream, const void* value) const {
        wiredtiger_pack_str((WT_PACK_STREAM*)stream, ((const std::string*)value)->data());
    }
}
