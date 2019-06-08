#pragma once

#include <string>
#include "headers.hpp"
#include <ac-library/httplike/parser/parser.hpp>
#include <memory>
#include <utility>

namespace NAC {
    namespace NHTTP {
        class TBodyPart;
        class TWithBodyParts;
    }

    namespace NHTTPUtils {
        std::string Boundary();

        std::vector<NHTTP::TBodyPart> ParseParts(
            const std::string& boundary,
            size_t size,
            const char* data
        );
    }

    namespace NHTTP {
        class TBodyPart {
        public:
            explicit TBodyPart(std::shared_ptr<NHTTPLikeParser::TParsedData> data);

            const THeaders& Headers() const {
                return Data->Headers;
            }

            size_t ContentLength() const {
                return Data->BodySize;
            }

            const char* Content() const {
                return Data->Body;
            }

            const std::string& ContentType() const {
                return ContentType_;
            }

            const THeaderParams& ContentTypeParams() const {
                return ContentTypeParams_;
            }

            const std::string& ContentDisposition() const {
                return ContentDisposition_;
            }

            const THeaderParams& ContentDispositionParams() const {
                return ContentDispositionParams_;
            }

        private:
            std::shared_ptr<NHTTPLikeParser::TParsedData> Data;
            std::string ContentType_;
            THeaderParams ContentTypeParams_;
            std::string ContentDisposition_;
            THeaderParams ContentDispositionParams_;
        };

        class TWithBodyParts {
        public:
            virtual ~TWithBodyParts() {
            }

            const std::vector<TBodyPart>& Parts() const {
                return Parts_;
            }

        protected:
            template<typename... TArgs>
            void ParseParts(TArgs&&... args) {
                Parts_ = NHTTPUtils::ParseParts(std::forward<TArgs>(args)...);
            }

        private:
            std::vector<TBodyPart> Parts_;
        };
    }
}
