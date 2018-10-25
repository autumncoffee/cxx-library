#include "server.hpp"
#include "client.hpp"

namespace {
    using namespace NAC;
    using namespace NAC::NHTTPServer;

    static inline TServer::TArgs BuildArgs(
        const TServer::TArgs& args,
        NHTTPHandler::THandler& handler
    ) {
        TServer::TArgs out(args);

        if (!out.ClientFactory) {
            out.ClientFactory = NAC::NNetServer::TServer::TArgs::MakeClientFactory<NAC::NHTTPServer::TClient>();
        }

        if (!out.ClientArgsFactory) {
            out.ClientArgsFactory = [&handler]() {
                return new TClient::TArgs(handler);
            };
        }

        return out;
    }
}

namespace NAC {
    namespace NHTTPServer {
        TServer::TServer(const TArgs& args, NHTTPHandler::THandler& handler)
            : Server(BuildArgs(args, handler))
        {
        }
    }
}
