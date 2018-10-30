#include "responder.hpp"
#include "client.hpp"

namespace NAC {
    namespace NHTTPServer {
        TResponder::TResponder(std::shared_ptr<TClient> client)
            : Client(client)
        {
        }

        void TResponder::Respond(const NHTTP::TResponse& response) const {
            Client->PushWriteQueue((std::shared_ptr<NHTTPLikeParser::TParsedData>)response);
        }

        std::shared_ptr<TResponder::TAwaitHTTPClient> TResponder::AwaitHTTP(
            const char* const host,
            const short port,
            NHTTPLikeServer::TAwaitClient<NHTTPLikeServer::TClient>::TCallback&& cb,
            const size_t maxRetries
        ) const {
            return Client->Connect<TAwaitHTTPClient>(host, port, maxRetries, cb, Client->GetNewSharedPtr());
        }
    }
}
