#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <spin_lock.hpp>
#include <memory>
#include <string.h>
#include <str.hpp>

namespace NAC {
    namespace NHTTPLikeParser {
        using THeaders = std::unordered_map<std::string, std::vector<std::string>>;

        struct TParsedData {
            const size_t FirstLineSize = 0;
            char* FirstLine = nullptr;
            THeaders Headers;
            const size_t BodySize = 0;
            char* Body = nullptr;
            TBlob Request;
            size_t WriteOffset = 0;

            template<typename... TArgs>
            TParsedData& Append(TArgs&&... args) {
                Request.Append(std::forward<TArgs&&>(args)...);
                return *this;
            }

            template<typename... TArgs>
            TParsedData& operator<<(TArgs&&... args) {
                Request.Append(std::forward<TArgs&&>(args)...);
                return *this;
            }

            ~TParsedData();
        };

        struct TParserState;

        class TParser {
        public:
            TParser();

            void Add(const size_t dataSize, char* data);
            void Message(const size_t dataSize, char* data, bool copy = false);
            void MessageNoFirstLine(const size_t dataSize, char* data, bool copy = false);
            std::vector<std::shared_ptr<TParsedData>> ExtractData();

        private:
            std::shared_ptr<TParserState> State;
        };
    }
}
