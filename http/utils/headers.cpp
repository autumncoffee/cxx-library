#include "headers.hpp"
#include <algorithm>
#include <ctype.h>
#include <ac-common/utils/string.hpp>

namespace NAC {
    namespace NHTTPUtils {
        void ParseHeaderValue(
            const std::string& contentType,
            std::string& contentTypeDest,
            NHTTP::THeaderParams& contentTypeParamsDest
        ) {
            size_t i = 0;
            size_t offset = 0;
            bool inKey = true;
            bool haveKey = false;
            bool inQueryParams = false;
            std::string key;

            for (; i < contentType.size(); ++i) {
                const char chr = contentType[i];
                const bool isLast = (i == (contentType.size() - 1));

                if (!inQueryParams && (isLast || (chr == ';'))) {
                    NStringUtils::Strip(
                        (i + (isLast ? 1 : 0) - offset),
                        contentType.data() + offset,
                        contentTypeDest
                    );

                    std::transform(contentTypeDest.begin(), contentTypeDest.end(), contentTypeDest.begin(), tolower);

                    inQueryParams = true;
                    offset = i + 1;
                    continue;
                }

                if (!inQueryParams) {
                    continue;
                }

                if ((chr == ' ') && inKey && (!haveKey)) {
                    offset = i + 1;
                }

                if (inKey) {
                    haveKey = true;
                }

                if (chr == '=') {
                    NStringUtils::Strip(
                        i - offset,
                        contentType.data() + offset,
                        key
                    );

                    std::transform(key.begin(), key.end(), key.begin(), tolower);

                    offset = i + 1;
                    inKey = false;

                } else if ((chr == ';') || isLast) {
                    std::string value;

                    NStringUtils::Strip(
                        i + (isLast ? 1 : 0) - offset,
                        contentType.data() + offset,
                        value
                    );

                    contentTypeParamsDest.emplace(key, std::move(value));
                    offset = i + 1;
                    inKey = true;
                    haveKey = false;
                }
            }
        }
    }
}
