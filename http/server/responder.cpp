#include "responder.hpp"
#include "client.hpp"
#include <http/request.hpp>
#include <stdlib.h>

namespace NAC {
    namespace NHTTPServer {
        TResponder::TResponder(std::shared_ptr<TClient> client)
            : Client(client)
        {
        }

        void TResponder::Respond(const NHTTP::TResponse& response) const {
            Client->PushWriteQueue((std::shared_ptr<NHTTPLikeParser::TParsedData>)response);
        }

        void TResponder::Send(const NWebSocketParser::TFrame& frame) const {
            Client->PushWriteQueue(frame);
        }

        std::shared_ptr<TResponder::TAwaitHTTPClient> TResponder::AwaitHTTP(
            const char* const host,
            const short port,
            NHTTPLikeServer::TAwaitClient<NHTTPLikeServer::TClient>::TCallback&& cb,
            const size_t maxRetries
        ) const {
            return Client->Connect<TAwaitHTTPClient>(host, port, maxRetries, cb, Client->GetNewSharedPtr());
        }

        void TResponder::OnWebSocketStart() const {
            Client->OnWebSocketStart((const std::shared_ptr<const NHTTP::TRequest>)RequestPtr);
        }

        void TResponder::SetRequestPtr(const std::shared_ptr<const NHTTP::TRequest>& ptr) {
            if(!RequestPtr.expired()) {
                abort();
            }

            RequestPtr = ptr;
        }
    }
}
