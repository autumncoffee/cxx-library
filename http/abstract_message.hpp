#pragma once

#include <string>
#include <vector>
#include <ac-library/http/utils/headers.hpp>
#include <ac-common/utils/string.hpp>

namespace NAC {
    namespace NHTTP {
        class TAbstractMessage {
        public:
            virtual ~TAbstractMessage() {
            }

            virtual const THeaders& Headers() const = 0;
            virtual size_t ContentLength() const = 0;
            virtual const char* Content() const = 0;

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
                return NStringUtils::ToLower(HeaderValue(name));
            }
        };
    }
}
