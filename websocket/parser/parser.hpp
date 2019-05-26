#pragma once

#include <ac-common/str.hpp>
#include <memory>
#include <vector>
#include <ac-common/utils/string.hpp>

namespace NAC {
    namespace NWebSocketParser {
        struct TFrame {
            bool IsFin = false;
            bool RSV1 = false;
            bool RSV2 = false;
            bool RSV3 = false;
            bool IsMasked = false;
            unsigned char Opcode = 0;
            char Mask[4] = {0, 0, 0, 0};
            TBlob Payload;

            TBlob Header() const;
            TBlob MaskedPayload() const;

            explicit operator TBlob() const;
        };

        class TParserState;

        class TParser {
        public:
            TParser();

            void Add(const size_t dataSize, char* data);
            std::vector<std::shared_ptr<TFrame>> ExtractData();

        private:
            std::shared_ptr<TParserState> State;
        };
    }
}
