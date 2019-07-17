#pragma once

#include <string>
#include "headers.hpp"
#include <ac-library/httplike/parser/parser.hpp>
#include <memory>
#include <utility>
#include <ac-library/http/abstract_message.hpp>

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
        class TBodyPart : public TAbstractMessage {
        public:
            explicit TBodyPart(std::shared_ptr<NHTTPLikeParser::TParsedData> data);

            const THeaders& Headers() const override {
                return Data->Headers;
            }

            size_t ContentLength() const override {
                return Data->BodySize;
            }

            const char* Content() const override {
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
