#pragma once

#include <sys/types.h>
#include <string>
#include <ac-common/str.hpp>
#include <unordered_map>
#include <vector>

namespace NAC {
    namespace NHTTP {
        using TQueryParams = std::unordered_map<std::string, std::vector<std::string>>;

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

        TQueryParams ParseQueryParams(const size_t size, const char* data);

        static inline TQueryParams ParseQueryParams(const std::string& data) {
            return ParseQueryParams(data.size(), data.data());
        }

        static inline TQueryParams ParseQueryParams(const TBlob& data) {
            return ParseQueryParams(data.Size(), data.Data());
        }

        class TQueryParamsBuilder {
        public:
            operator std::string() const {
                return (std::string)Data;
            }

            TQueryParamsBuilder& Reserve(const size_t size) {
                Data.Reserve(size);

                return *this;
            }

            TQueryParamsBuilder& Add(const size_t keySize, const char* key) {
                if (Data.Size() == 0) {
                    Data.Append(1, "?");

                } else {
                    Data.Append(1, "&");
                }

                auto&& key_ = URLEscape(keySize, key);
                Data.Append(key_.Size(), key_.Data());

                return *this;
            }

            TQueryParamsBuilder& Add(const size_t keySize, const char* key, const size_t valueSize, const char* value) {
                Add(keySize, key);

                Data.Append(1, "=");

                auto&& value_ = URLEscape(valueSize, value);
                Data.Append(value_.Size(), value_.Data());

                return *this;
            }

            TQueryParamsBuilder& Add(const std::string& key) {
                return Add(key.size(), key.data());
            }

            TQueryParamsBuilder& Add(const std::string& key, const std::string& value) {
                return Add(key.size(), key.data(), value.size(), value.data());
            }

            TQueryParamsBuilder& Add(const TBlob& key) {
                return Add(key.Size(), key.Data());
            }

            TQueryParamsBuilder& Add(const TBlob& key, const TBlob& value) {
                return Add(key.Size(), key.Data(), value.Size(), value.Data());
            }

        private:
            TBlob Data;
        };
    }
}
