#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <ac-common/spin_lock.hpp>
#include <memory>
#include <ac-common/memdisk.hpp>

namespace NAC {
    namespace NHTTPLikeParser {
        using THeaders = std::unordered_map<std::string, std::vector<std::string>>;

        struct TParsedData {
            const size_t FirstLineSize = 0;
            char* FirstLine = nullptr;
            THeaders Headers;
            const size_t BodySize = 0;
            char* Body = nullptr;
            TMemDisk Request;

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
            TParser(
                size_t memMax = 10 * 1024 * 1024,
                const std::string& diskMask = "/tmp/achttplike.XXXXXXXXXX"
            );

            void Add(const size_t dataSize, char* data);
            void Message(const size_t dataSize, char* data, bool copy = false);
            void MessageNoFirstLine(const size_t dataSize, char* data, bool copy = false);
            std::vector<std::shared_ptr<TParsedData>> ExtractData();

        private:
            std::shared_ptr<TParserState> State;
        };
    }
}
