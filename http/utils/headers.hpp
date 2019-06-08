#pragma once

#include <ac-library/httplike/parser/parser.hpp>
#include <string>
#include <unordered_map>

namespace NAC {
    namespace NHTTP {
        using THeaders = NHTTPLikeParser::THeaders;
        using THeaderParams = std::unordered_map<std::string, std::string>;
    }

    namespace NHTTPUtils {
        void ParseHeaderValue(
            const std::string& headerValue,
            std::string& dest,
            NHTTP::THeaderParams& paramsDest
        );

        static inline void ParseHeader(
            const NHTTP::THeaders& headers,
            const std::string& headersKey,
            std::string& dest,
            NHTTP::THeaderParams& paramsDest
        ) {
            const auto& headerValue = headers.find(headersKey);

            if (headerValue == headers.end()) {
                return;
            }

            return ParseHeaderValue(headerValue->second.front(), dest, paramsDest);
        }
    }
}
