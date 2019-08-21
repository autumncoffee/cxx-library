#pragma once

#include "response.hpp"
#include <ac-library/http/server/responder.hpp>
#include <ac-library/websocket/parser/parser.hpp>
#include <ac-common/maybe.hpp>
#include <utility>
#include "message.hpp"

#define AC_HTTP_REQUEST_CODE_SHORTCUT(code, msg) \
TResponse Respond ## code() const { \
    return Respond(#code " " msg); \
} \
\
void Send ## code() { \
    Send(Respond ## code()); \
}

#define AC_HTTP_REQUEST_SEND(type) \
void Send(type data) const { \
    Responder.Send(std::forward<type>(data)); \
}

namespace NAC {
    namespace NHTTP {
        using TQueryParams = std::unordered_map<std::string, std::vector<std::string>>;
        using TCookies = std::unordered_map<std::string, std::string>;

        struct TRangeSpec {
            TMaybe<size_t> Start;
            TMaybe<size_t> End;
        };

        struct TRangeHeaderValue {
            std::string Unit;
            std::vector<TRangeSpec> Ranges;
        };

        class TRequest : public TMessageBase {
        public:
            TRequest(
                std::shared_ptr<NHTTPLikeParser::TParsedData> data,
                const NHTTPServer::TResponder& responder
            );

            virtual ~TRequest() {
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

            template<typename TArg>
            const std::vector<std::string>* QueryParams(const TArg& name) const {
                const auto& params = QueryParams();
                const auto& values = params.find(name);

                if ((values == params.end()) || values->second.empty()) {
                    return nullptr;
                }

                return &values->second;
            }

            template<typename TArg>
            const std::string& QueryParam(const TArg& name) const {
                static const std::string empty;
                const auto* values = QueryParams(name);

                return (values ? values->front() : empty);
            }

            const TCookies& Cookies() const {
                return Cookies_;
            }

            bool IsHead() const {
                return (Method() == "head");
            }

            bool IsGet() const {
                return (Method() == "get");
            }

            bool IsPost() const {
                return (Method() == "post");
            }

            bool IsPatch() const {
                return (Method() == "patch");
            }

            bool IsDelete() const {
                return (Method() == "delete");
            }

            bool IsPut() const {
                return (Method() == "put");
            }

            TResponse Respond(const std::string& codeAndMsg) const {
                TResponse out;
                out.FirstLine(Protocol() + " " + codeAndMsg + "\r\n");
                return out;
            }

            #include "request.status_codes"

            template<typename... TArgs>
            TResponse RespondContent(TArgs&&... args) const {
                auto out = Respond200();
                out.Write(std::forward<TArgs&&>(args)...);
                return out;
            }

            TResponse RespondFile(
                size_t size,
                char* data,
                const std::string& contentType = "application/octet-stream"
            );

            TResponse RespondFile(
                size_t size,
                const char* data,
                const std::string& contentType = "application/octet-stream"
            ) {
                return RespondFile(size, (char*)data, contentType);
            }

            TResponse RespondFile(
                const std::string& path,
                const std::string& contentType = "application/octet-stream",
                size_t size = 0,
                size_t offset = 0
            );

            TResponse RespondFile(
                const std::string& path,
                size_t size,
                size_t offset,
                const std::string& contentType = "application/octet-stream"
            ) {
                return RespondFile(path, contentType, size, offset);
            }

            void Send(const TResponse& response) {
                if (ResponseSent.exchange(true)) {
                    throw std::runtime_error("Response sent twice: " + response.FirstLine());
                }

                Responder.Respond(response);
            }

            template<typename... TArgs>
            void SendFile(TArgs&&... args) {
                Send(RespondFile(std::forward<TArgs>(args)...));
            }

            AC_HTTP_REQUEST_SEND(const NWebSocketParser::TFrame&);
            AC_HTTP_REQUEST_SEND(const TBlob&);
            AC_HTTP_REQUEST_SEND(TBlob&&);
            AC_HTTP_REQUEST_SEND(const TBlobSequence&);
            AC_HTTP_REQUEST_SEND(TBlobSequence&&);

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
                auto response = Respond101();

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

            TRangeHeaderValue Range() const;

        private:
            std::string Method_;
            std::string Path_;
            std::string Protocol_;
            TQueryParams QueryParams_;
            TCookies Cookies_;
            NHTTPServer::TResponder Responder;
            std::atomic<bool> ResponseSent;
        };
    }
}
