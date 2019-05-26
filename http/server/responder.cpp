#include "responder.hpp"
#include "client.hpp"
#include <ac-library/http/request.hpp>
#include <stdlib.h>

namespace NAC {
    namespace NHTTPServer {
        TResponder::TResponder(std::shared_ptr<TClient> client)
            : Client_(client)
        {
        }

        void TResponder::Respond(const NHTTP::TResponse& response) const {
            if (Client_.expired()) {
                return;
            }

            Client()->PushWriteQueue(response);
        }

        void TResponder::Send(const NWebSocketParser::TFrame& frame) const {
            if (Client_.expired()) {
                return;
            }

            Client()->PushWriteQueue(frame);
        }

        std::shared_ptr<TResponder::TAwaitHTTPClient> TResponder::AwaitHTTP(
            const char* const host,
            const short port,
            TAwaitHTTPClient::TCallback&& cb,
            const size_t maxRetries
        ) const {
            if (Client_.expired()) {
                return std::shared_ptr<TAwaitHTTPClient>();
            }

            auto client = Client();
            return client->Connect<TAwaitHTTPClient>(host, port, maxRetries, std::forward<TAwaitHTTPClient::TCallback>(cb), client->GetNewSharedPtr());
        }

        void TResponder::OnWebSocketStart() const {
            if (Client_.expired()) {
                return;
            }

            Client()->OnWebSocketStart((const std::shared_ptr<const NHTTP::TRequest>)RequestPtr);
        }

        void TResponder::SetRequestPtr(std::shared_ptr<const NHTTP::TRequest>& ptr) {
            if(!RequestPtr.expired()) {
                abort();
            }

            RequestPtr = ptr;
        }
    }
}
