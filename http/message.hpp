#pragma once

#include <ac-library/httplike/parser/parser.hpp>
#include <string>
#include <vector>
#include <ac-library/http/utils/multipart.hpp>
#include <ac-common/utils/string.hpp>
#include "abstract_message.hpp"

namespace NAC {
    namespace NHTTP {
        class TMessageBase : public TWithBodyParts, public TAbstractMessage {
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

            const THeaders& Headers() const override {
                return Data->Headers;
            }

            size_t ContentLength() const override {
                return Data->BodySize;
            }

            const char* Content() const override {
                return Data->Body;
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
