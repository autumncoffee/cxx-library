#pragma once

#include <sys/types.h>
#include <string>

namespace NAC {
    namespace NHTTP {
        void URLUnescape(size_t& size, char* data);

        static inline void URLUnescape(std::string& data) {
            size_t size = data.size();

            URLUnescape(size, data.data());

            data.resize(size);
        }
    }
}
