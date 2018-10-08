#include "urlescape.hpp"
#include <iostream>

namespace {
    static const bool Hex[256] = {
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
        false, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false
    };

    #define TO_CHAR_IMPL { \
        c *= 16; \
        c += (data[0] >= 'A' ? ((data[0] & 0xdf) - 'A') + 10 : (data[0] - '0')); \
        ++data; \
    }

    static inline char ToChar(const char* data) {
        unsigned char c = 0;

        TO_CHAR_IMPL;
        TO_CHAR_IMPL;

        return c;
    }
}

namespace NAC {
    namespace NHTTP {
        void URLUnescape(size_t& size, char* data) {
            size_t offset = 0;

            for (size_t i = 0; i < size; ++i) {
                switch (data[i]) {
                    case '%': {
                        if ((i + 2) >= size) {
                            data[offset++] = '%';

                        } else {
                            if ((Hex[data[i + 1]]) && (Hex[data[i + 2]])) {
                                data[offset++] = ToChar(data + i + 1);
                                i += 2;

                            } else {
                                data[offset++] = '%';
                            }
                        }

                        break;
                    }

                    case '+':
                        data[offset++] = ' ';
                        break;

                    default:
                        data[offset++] = data[i];
                }
            }

            size = offset;
        }
    }
}
