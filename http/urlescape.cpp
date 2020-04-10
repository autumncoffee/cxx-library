#include "urlescape.hpp"
#include <iostream>
#include <ac-common/utils/string.hpp>

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

        TBlob URLEscape(const size_t size, const char* data) {
            static const bool ShouldEscape[256] = {true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, true, true, false, true, true, false, true, true, false, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, true, false, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true};
            static const char CharTable[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
            static const char Plus('+');

            TBlob out;
            out.Reserve(size);

            for (size_t i = 0; i < size; ++i) {
                if (data[i] == ' ') {
                    out.Append(1, &Plus);

                } else if (ShouldEscape[data[i]]) {
                    out.Append(1, "%");
                    out.Append(1, &CharTable[data[i] >> 4]);
                    out.Append(1, &CharTable[data[i] & 0xf]);

                } else {
                    out.Append(1, &data[i]);
                }
            }

            return out;
        }

        TQueryParams ParseQueryParams(const size_t size, const char* data) {
            TQueryParams out;
            bool inKey(true);
            std::string key;
            size_t offset(0);

            for (size_t i = 0; i < size; ++i) {
                const char chr(data[i]);
                const bool isLast(i == (size - 1));

                if (inKey) {
                    if ((chr == '=') || (chr == '&') || isLast || (chr == '#')) {
                        key.assign(data + offset, i + ((isLast && (chr != '=') && (chr != '&') && (chr != '#')) ? 1 : 0) - offset);
                        NStringUtils::ToLower(key);
                        URLUnescape(key);

                        offset = i + 1;

                        if ((chr == '=') && !isLast) {
                            inKey = false;

                        } else if (key.size() > 0) {
                            out[key].emplace_back();
                        }
                    }

                } else {
                    if ((chr == '&') || isLast || (chr == '#')) {
                        const size_t size(i + ((isLast && (chr != '&') && (chr != '#')) ? 1 : 0) - offset);

                        if (size > 0) {
                            std::string value(data + offset, size);
                            URLUnescape(value);

                            out[key].emplace_back(std::move(value));

                        } else if (key.size() > 0) {
                            out[key].emplace_back();
                        }

                        offset = i + 1;
                        inKey = true;
                    }
                }

                if (chr == '#') {
                    break;
                }
            }

            return out;
        }
    }
}
