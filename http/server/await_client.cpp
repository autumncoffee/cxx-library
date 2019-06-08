#include "await_client.hpp"
#include <ac-common/utils/string.hpp>

namespace NAC {
    namespace NHTTP {
        TIncomingResponse::TIncomingResponse(std::shared_ptr<NHTTPLikeParser::TParsedData> data)
            : TMessageBase(data)
        {
            std::string tmp;
            tmp.reserve(3);
            bool seenProtocol(false);

            for (size_t i = 0; i < data->FirstLineSize; ++i) {
                const char chr(data->FirstLine[i]);

                if (chr == ' ') {
                    if (seenProtocol) {
                        if (!tmp.empty()) {
                            break;
                        }

                    } else {
                        seenProtocol = true;
                    }

                } else if (seenProtocol) {
                    tmp += chr;
                }
            }

            NStringUtils::FromString(tmp, StatusCode_);
        }
    }
}
