#pragma once

#include <ac-library/httplike/server/client.hpp>
#include <ac-library/http/handler/handler.hpp>
#include <ac-library/websocket/parser/parser.hpp>

namespace NAC {
    namespace NHTTPServer {
        class TClient : public NHTTPLikeServer::TClient {
        public:
            struct TArgs : public NHTTPLikeServer::TClient::TArgs {
                NHTTPHandler::THandler& Handler;

                TArgs(NHTTPHandler::THandler& handler)
                    : Handler(handler)
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

            virtual void OnWebSocketStart(std::shared_ptr<const NHTTP::TRequest> request);

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
            virtual void HandleRequestImpl(std::shared_ptr<const NHTTP::TRequest> request);
            virtual void HandleException(const NHTTP::TRequest& request, const std::exception& e);
            virtual NHTTP::TResponse InternalServerError(const std::exception& e) const;
            virtual NHTTP::TResponse InternalServerError() const;
            virtual NHTTP::TResponse InternalServerErrorResponse() const;

            virtual void HandleFrame(
                std::shared_ptr<NWebSocketParser::TFrame> frame,
                std::shared_ptr<const NHTTP::TRequest> origin
            );

        private:
            std::unique_ptr<NWebSocketParser::TParser> WebSocketParser;
            std::shared_ptr<const NHTTP::TRequest> WebSocketOrigin;
            bool SSL = false;
        };
    }
}
