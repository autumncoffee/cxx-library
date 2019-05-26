#pragma once

#include <ac-library/net/server/server.hpp>
#include <ac-library/http/handler/handler.hpp>

namespace NAC {
    namespace NHTTPServer {
        class TServer {
        public:
            struct TArgs : public NNetServer::TServer::TArgs {
            };

        public:
            TServer(const TArgs& args, NHTTPHandler::THandler& handler);

            void Run() {
                Server.Run();
            }

            void Start() {
                Server.Start();
            }

        private:
            NNetServer::TServer Server;
        };
    }
}
