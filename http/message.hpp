#pragma once

#include <ac-library/httplike/parser/parser.hpp>
#include <string>
#include <string.h>
#include <algorithm>
#include <vector>
#include <ac-library/http/utils/multipart.hpp>

namespace NAC {
    namespace NHTTP {
        class TMessageBase : public TWithBodyParts {
        public:
            TMessageBase(std::shared_ptr<NHTTPLikeParser::TParsedData> data);

            virtual ~TMessageBase() {
            }

            const std::string& ContentType() const {
                return ContentType_;
            }

            const THeaderParams& ContentTypeParams() const {
                return ContentTypeParams_;
            }

            const std::string FirstLine() const {
                return std::string(Data->FirstLine, Data->FirstLineSize);
            }

            const THeaders& Headers() const {
                return Data->Headers;
            }

            size_t ContentLength() const {
                return Data->BodySize;
            }

            const char* Content() const {
                return Data->Body;
            }

            template<typename TArg>
            const std::vector<std::string>* HeaderValues(const TArg& name) const {
                const auto& headers = Headers();
                const auto& header = headers.find(name);

                if ((header == headers.end()) || header->second.empty()) {
                    return nullptr;
                }

                return &header->second;
            }

            template<typename TArg>
            const std::string& HeaderValue(const TArg& name) const {
                static const std::string empty;
                const auto* values = HeaderValues(name);

                return (values ? values->front() : empty);
            }

            template<typename TArg>
            std::string HeaderValueLower(const TArg& name) const {
                std::string value(HeaderValue(name));
                std::transform(value.begin(), value.end(), value.begin(), tolower);

                return value;
            }

        private:
            void ParseParts();

        private:
            std::shared_ptr<NHTTPLikeParser::TParsedData> Data;
            std::string ContentType_;
            THeaderParams ContentTypeParams_;
        };
    }
}
