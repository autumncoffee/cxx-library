#pragma once

#include <httplike/parser/parser.hpp>
#include "response.hpp"

namespace NAC {
    namespace NHTTP {
        using THeaders = NHTTPLikeParser::THeaders;
        using TQueryParams = std::unordered_map<std::string, std::vector<std::string>>;
        using TCookies = std::unordered_map<std::string, std::string>;
        using TContentTypeParams = std::unordered_map<std::string, std::string>;
        using TContentDispositionParams = TContentTypeParams;

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

            const TContentTypeParams& ContentTypeParams() const {
                return ContentTypeParams_;
            }

            const std::string& ContentDisposition() const {
                return ContentDisposition_;
            }

            const TContentDispositionParams& ContentDispositionParams() const {
                return ContentDispositionParams_;
            }

        private:
            std::shared_ptr<NHTTPLikeParser::TParsedData> Data;
            std::string ContentType_;
            TContentTypeParams ContentTypeParams_;
            std::string ContentDisposition_;
            TContentTypeParams ContentDispositionParams_;
        };

        class TRequest {
        public:
            explicit TRequest(std::shared_ptr<NHTTPLikeParser::TParsedData> data);

            const std::string FirstLine() const {
                return std::string(Data->FirstLine, Data->FirstLineSize);
            }

            const THeaders& Headers() const {
                return Data->Headers;
            }

            const std::string& Method() const {
                return Method_;
            }

            const std::string& Path() const {
                return Path_;
            }

            const std::string& Protocol() const {
                return Protocol_;
            }

            const TQueryParams& QueryParams() const {
                return QueryParams_;
            }

            const TCookies& Cookies() const {
                return Cookies_;
            }

            const std::string& ContentType() const {
                return ContentType_;
            }

            const TContentTypeParams& ContentTypeParams() const {
                return ContentTypeParams_;
            }

            size_t ContentLength() const {
                return Data->BodySize;
            }

            const char* Content() const {
                return Data->Body;
            }

            const std::vector<TBodyPart>& Parts() const {
                return Parts_;
            }

            TResponse Respond(const std::string& codeAndMsg) const {
                TResponse out;
                out.FirstLine(Protocol() + " " + codeAndMsg + "\r\n");
                return out;
            }

            TResponse Respond200() const {
                return Respond("200 OK");
            }

            TResponse Respond400() const {
                return Respond("400 Bad Request");
            }

            TResponse Respond403() const {
                return Respond("403 Forbidden");
            }

            template<typename... TArgs>
            TResponse RespondContent(TArgs&&... args) const {
                auto out = Respond200();
                out.Write(std::forward<TArgs&&>(args)...);
                return out;
            }

        private:
            void ParseParts();

        private:
            std::shared_ptr<NHTTPLikeParser::TParsedData> Data;
            std::string Method_;
            std::string Path_;
            std::string Protocol_;
            TQueryParams QueryParams_;
            TCookies Cookies_;
            std::string ContentType_;
            TContentTypeParams ContentTypeParams_;
            std::vector<TBodyPart> Parts_;
        };
    }
}
