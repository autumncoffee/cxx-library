#include "model.hpp"
#include <wiredtiger.h>
#include <stdio.h>

namespace NAC {
    void TWiredTigerModelBase::Load(void* session, size_t size, const void* data) {
        auto&& modelData = GetACModelData();
        modelData.clear();
        WT_PACK_STREAM* stream;

        int result = wiredtiger_unpack_start(
            (WT_SESSION*)session,
            ACWTModelGetFullFormat().data(),
            data,
            size,
            &stream
        );

        if (result != 0) {
            dprintf(
                2,
                "Failed to unpack: %s\n",
                wiredtiger_strerror(result)
            );

            return;
        }

        for (const auto& it : GetACModelFields()) {
            modelData.emplace_back(((const TWiredTigerFieldBase*)it.get())->Load(stream));
        }

        {
            size_t dummy;
            wiredtiger_pack_close(stream, &dummy);
        }
    }

    TBlob TWiredTigerModelBase::Dump(void* session) const {
        size_t size = 0;

        for (const auto& it : GetACModelFields()) {
            if (auto* ptr = ACModelGet(it->Index)) {
                size += ((const TWiredTigerFieldBase*)it.get())->DumpedSize(session, ptr);
            }
        }

        if (size == 0) {
            return TBlob();
        }

        TBlob out;
        out.Reserve(size);

        WT_PACK_STREAM* stream;

        int result = wiredtiger_pack_start(
            (WT_SESSION*)session,
            ACWTModelGetFullFormat().data(),
            (void*)out.Data(),
            size,
            &stream
        );

        if (result != 0) {
            dprintf(
                2,
                "Failed to pack: %s\n",
                wiredtiger_strerror(result)
            );

            return TBlob();
        }

        for (const auto& it : GetACModelFields()) {
            if (auto* ptr = ACModelGet(it->Index)) {
                ((const TWiredTigerFieldBase*)it.get())->Dump(stream, ptr);
            }
        }

        wiredtiger_pack_close(stream, &size);
        out.Shrink(size);

        return out;
    }
}
