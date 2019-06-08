#include "multipart.hpp"
#include <random>
#include <string.h>
#include <algorithm>

namespace NAC {
    namespace NHTTPUtils {
        std::string Boundary() {
            static const char* Chars("-_1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
            static const size_t CharsSize(strlen(Chars));
            static const size_t OutSize(40);

            std::string out;
            out.reserve(OutSize);

            thread_local static std::random_device rd;
            thread_local static std::mt19937 g(rd());
            thread_local static std::uniform_int_distribution<size_t> dis(0, (CharsSize - 1));

            for (size_t i = 0; i < OutSize; ++i) {
                out += Chars[dis(g)];
            }

            return out;
        }

        std::vector<NHTTP::TBodyPart> ParseParts(
            const std::string& boundary_,
            size_t size,
            const char* pos
        ) {
            std::string boundary;
            boundary.reserve(boundary_.size() + 2);
            boundary += "--";
            boundary += boundary_;

            const char* end = pos + size;
            const char* nextPos;
            bool isFirst = true;

            NHTTPLikeParser::TParser parser;

            while ((pos = std::search(pos, end, boundary.begin(), boundary.end())) != end) {
                if (isFirst) {
                    boundary = std::string("\r\n") + boundary;
                }

                pos += boundary.size();

                if (isFirst) {
                    isFirst = false;
                    // pos is ok: first boundary occurrence does not have leading \r\n, but does have trailing \r\n

                } else {
                    pos += 2; // further occurrences of boundary have both leading and trailing \r\n
                }

                if (pos >= end) {
                    break;
                }

                nextPos = std::search(pos, end, boundary.begin(), boundary.end());

                if (nextPos == end) {
                    break;
                }

                parser.MessageNoFirstLine((uintptr_t)nextPos - (uintptr_t)pos, (char*)pos);
                pos = nextPos;
            }

            const auto& parts = parser.ExtractData();
            std::vector<NHTTP::TBodyPart> out;
            out.reserve(parts.size());

            for (const auto& part : parts) {
                out.emplace_back(part);
            }

            return out;
        }
    }

    namespace NHTTP {
        TBodyPart::TBodyPart(std::shared_ptr<NHTTPLikeParser::TParsedData> data)
            : Data(data)
        {
            NHTTPUtils::ParseHeader(Headers(), "content-type", ContentType_, ContentTypeParams_);
            NHTTPUtils::ParseHeader(Headers(), "content-disposition", ContentDisposition_, ContentDispositionParams_);
        }
    }
}
