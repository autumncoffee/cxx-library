#pragma once

#include <ac-library/httplike/server/client.hpp>
#include <ac-library/http/handler/handler.hpp>
#include <ac-library/websocket/parser/parser.hpp>
#include <functional>

namespace NAC {
    namespace NHTTPServer {
        class TClient : public NHTTPLikeServer::TClient {
        public:
            struct TArgs : public NHTTPLikeServer::TClient::TArgs {
            public:
                using TRequestFactory = std::function<NHTTP::TRequest*(
                    std::shared_ptr<NHTTPLikeParser::TParsedData>,
                    const TResponder&
                )>;

            public:
                NHTTPHandler::THandler& Handler;
                TRequestFactory RequestFactory;

            public:
                TArgs(NHTTPHandler::THandler& handler, TRequestFactory&& requestFactory = TRequestFactory())
                    : Handler(handler)
                    , RequestFactory(requestFactory)
                {
                }
            };

        public:
            using NHTTPLikeServer::TClient::TClient;

        public:
            void Cb(const NMuhEv::TEvSpec& event) override;
            std::vector<std::shared_ptr<NWebSocketParser::TFrame>> GetWSData();

            using NHTTPLikeServer::TClient::PushWriteQueue;
            void PushWriteQueue(const NWebSocketParser::TFrame& frame);
            void PushWriteQueue(const NHTTP::TResponse& response);

            virtual void OnWebSocketStart(std::shared_ptr<NHTTP::TRequest> request);

        protected:
            virtual void OnData(std::shared_ptr<NWebSocketParser::TFrame> frame);
            void OnData(std::shared_ptr<NHTTPLikeParser::TParsedData> request) override;
            void OnData(const size_t dataSize, char* data) override;

            int ReadFromSocket(
                const int fh,
                void* buf,
                const size_t bufSize
            ) override;

            int WriteToSocket(
                const int fh,
                const void* buf,
                const size_t bufSize
            ) override;

            virtual void HandleRequest(std::shared_ptr<NHTTPLikeParser::TParsedData> data);
            virtual void HandleRequestImpl(std::shared_ptr<NHTTP::TRequest> request);
            virtual void HandleException(NHTTP::TRequest& request, const std::exception& e);
            virtual NHTTP::TResponse InternalServerError(const std::exception& e) const;
            virtual NHTTP::TResponse InternalServerError() const;
            virtual NHTTP::TResponse InternalServerErrorResponse() const;

            virtual void HandleFrame(
                std::shared_ptr<NWebSocketParser::TFrame> frame,
                std::shared_ptr<NHTTP::TRequest> origin
            );

        private:
            std::unique_ptr<NWebSocketParser::TParser> WebSocketParser;
            std::shared_ptr<NHTTP::TRequest> WebSocketOrigin;
            bool SSL = false;
        };
    }
}
