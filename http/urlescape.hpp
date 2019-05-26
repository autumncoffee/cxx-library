#pragma once

#include <sys/types.h>
#include <string>
#include <ac-common/str.hpp>

namespace NAC {
    namespace NHTTP {
        void URLUnescape(size_t& size, char* data);
        TBlob URLEscape(const size_t size, const char* data);

        static inline void URLUnescape(std::string& data) {
            size_t size = data.size();

            URLUnescape(size, data.data());

            data.resize(size);
        }

        static inline TBlob URLEscape(const std::string& data) {
            return URLEscape(data.size(), data.data());
        }
    }
}
