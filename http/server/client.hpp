#pragma once

#include <httplike/server/client.hpp>
#include <http/handler/handler.hpp>

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

        protected:
            void OnData(std::shared_ptr<NHTTPLikeParser::TParsedData> request) override;

            virtual void HandleRequest(std::shared_ptr<NHTTPLikeParser::TParsedData> data);
            virtual void HandleRequestImpl(const std::shared_ptr<const NHTTP::TRequest> request);
            virtual void HandleException(const NHTTP::TRequest& request, const std::exception& e);
            virtual NHTTP::TResponse InternalServerError(const std::exception& e) const;
            virtual NHTTP::TResponse InternalServerError() const;
            virtual NHTTP::TResponse InternalServerErrorResponse() const;
        };
    }
}
