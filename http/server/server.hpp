#pragma once

#include <httplike/parser/parser.hpp>
#include <httplike/server/server.hpp>
#include <http/handler/handler.hpp>

namespace NAC {
    namespace NHTTPServer {
        class TServer {
        public:
            struct TArgs : public NHTTPLikeServer::TServer::TArgs {
            };

        public:
            TServer(
                const TArgs& args,
                NHTTPHandler::THandler& handler
            );

            virtual NHTTP::TResponse HandleRequest(std::shared_ptr<NHTTPLikeParser::TParsedData> data);
            virtual NHTTP::TResponse HandleRequestImpl(const NHTTP::TRequest& request);
            virtual NHTTP::TResponse HandleException(const NHTTP::TRequest& request, const std::exception& e);
            virtual NHTTP::TResponse InternalServerError(const std::exception& e) const;
            virtual NHTTP::TResponse InternalServerError() const;
            virtual NHTTP::TResponse InternalServerErrorResponse() const;

            void Run() {
                Server.Run();
            }

            void Start() {
                Server.Start();
            }

        protected:
            NHTTPHandler::THandler& Handler;

        private:
            NHTTPLikeServer::TServer Server;
        };
    }
}
