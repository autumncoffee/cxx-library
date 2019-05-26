#pragma once

#include <ac-library/httplike/parser/parser.hpp>
#include "response.hpp"
#include <ac-library/http/server/responder.hpp>
#include <ac-library/websocket/parser/parser.hpp>

#define AC_HTTP_REQUEST_CODE_SHORTCUT(code, msg) \
TResponse Respond ## code() const { \
    return Respond(#code " " msg); \
} \
\
void Send ## code() { \
    Send(Respond ## code()); \
}

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
            TRequest(
                std::shared_ptr<NHTTPLikeParser::TParsedData> data,
                const NHTTPServer::TResponder& responder
            );

            virtual ~TRequest() {
            }

            const std::string FirstLine() const {
                return std::string(Data->FirstLine, Data->FirstLineSize);
            }

            const THeaders& Headers() const {
                return Data->Headers;
            }

            template<typename TArg>
            std::string HeaderValueLower(const TArg& name) const {
                std::string value(HeaderValue(name));
                std::transform(value.begin(), value.end(), value.begin(), tolower);

                return value;
            }

            template<typename TArg>
            const std::string& HeaderValue(const TArg& name) const {
                static const std::string empty;
                const auto* values = HeaderValues(name);

                return (values ? values->front() : empty);
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

            AC_HTTP_REQUEST_CODE_SHORTCUT(200, "OK");
            AC_HTTP_REQUEST_CODE_SHORTCUT(400, "Bad Request");
            AC_HTTP_REQUEST_CODE_SHORTCUT(403, "Forbidden");
            AC_HTTP_REQUEST_CODE_SHORTCUT(404, "Not Found");

            template<typename... TArgs>
            TResponse RespondContent(TArgs&&... args) const {
                auto out = Respond200();
                out.Write(std::forward<TArgs&&>(args)...);
                return out;
            }

            void Send(const TResponse& response) {
                if (ResponseSent.exchange(true)) {
                    throw std::runtime_error("Response sent twice: " + response.FirstLine());
                }

                Responder.Respond(response);
            }

            void Send(const NWebSocketParser::TFrame& frame) const {
                Responder.Send(frame);
            }

            bool IsResponseSent() const {
                return ResponseSent.load();
            }

            template<typename... TArgs>
            auto AwaitHTTP(TArgs... args) const {
                return Responder.AwaitHTTP(std::forward<TArgs>(args)...);
            }

            bool IsWebSocketHandshake() const;

            const std::string& SecWebSocketKey() const {
                return HeaderValue("sec-websocket-key");
            }

            const std::string& SecWebSocketVersion() const {
                return HeaderValue("sec-websocket-version");
            }

            std::string GenerateSecWebSocketAccept() const;

            TResponse RespondWebSocketAccept() const {
                auto response = Respond("101 Switching Protocols");

                response.Header("Connection", "Upgrade");
                response.Header("Upgrade", "websocket");
                response.Header("Sec-WebSocket-Accept", GenerateSecWebSocketAccept());

                return response;
            }

            void SendWebSocketAccept() {
                Send(RespondWebSocketAccept());
            }

            void OnWebSocketStart() const {
                Responder.OnWebSocketStart();
            }

            void WebSocketStart() {
                SendWebSocketAccept();
                OnWebSocketStart();
            }

            void SetWeakPtr(std::shared_ptr<TRequest>& ptr) {
                Responder.SetRequestPtr(ptr);
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
            NHTTPServer::TResponder Responder;
            std::atomic<bool> ResponseSent;
        };
    }
}
