#include "responder.hpp"
#include "client.hpp"
#include <ac-library/http/request.hpp>
#include <stdlib.h>
#include <ac-library/http/server/await_client.hpp>

#define AC_HTTP_RESPONSE_CHECKCLIENT() \
if (Client_.expired()) { \
    return; \
}

#define AC_HTTP_RESPONSE_CHECKCLIENT2(expr) \
if (Client_.expired()) { \
    return (expr); \
}

#define AC_HTTP_REPONDER_SEND_IMPL(type) \
void TResponder::Send(type data) const { \
    AC_HTTP_RESPONSE_CHECKCLIENT() \
    \
    Client()->PushWriteQueueData(data); \
}

namespace NAC {
    namespace NHTTPServer {
        TResponder::TResponder(std::shared_ptr<TClient> client)
            : Client_(client)
        {
        }

        void TResponder::Respond(const NHTTP::TResponse& response) const {
            AC_HTTP_RESPONSE_CHECKCLIENT()

            Client()->PushWriteQueue(response);
        }

        void TResponder::Respond(const NHTTP::TResponse& response, NNetServer::TWQCB&& cb) const {
            AC_HTTP_RESPONSE_CHECKCLIENT()

            Client()->PushWriteQueue(response, std::move(cb));
        }

        void TResponder::Respond(const NHTTP::TResponse& response, const NNetServer::TWQCB& cb) const {
            AC_HTTP_RESPONSE_CHECKCLIENT()

            Client()->PushWriteQueue(response, cb);
        }

        void TResponder::Send(const NWebSocketParser::TFrame& frame) const {
            AC_HTTP_RESPONSE_CHECKCLIENT()

            Client()->PushWriteQueue(frame);
        }

        std::shared_ptr<TResponder::TAwaitHTTPClient> TResponder::AwaitHTTP(
            const char* const host,
            const short port,
            const bool ssl,
            TAwaitClientCallback<>&& cb,
            const size_t maxRetries
        ) const {
            AC_HTTP_RESPONSE_CHECKCLIENT2(std::shared_ptr<TAwaitHTTPClient>())

            auto client = Client();
            return client->Connect<TAwaitHTTPClient>(host, port, ssl, maxRetries, std::forward<TAwaitClientCallback<>>(cb), client->GetNewSharedPtr());
        }

        std::shared_ptr<TResponder::TAwaitHTTPClient> TResponder::AwaitHTTP(
            const char* const host,
            const short port,
            const bool ssl,
            TAwaitClientCallback<>&& cb,
            TAwaitClientWSCallback<>&& wscb,
            const size_t maxRetries
        ) const {
            AC_HTTP_RESPONSE_CHECKCLIENT2(std::shared_ptr<TAwaitHTTPClient>())

            auto client = Client();
            return client->Connect<TAwaitHTTPClient>(host, port, ssl, maxRetries, std::forward<TAwaitClientCallback<>>(cb), std::forward<TAwaitClientWSCallback<>>(wscb), client->GetNewSharedPtr());
        }

        std::shared_ptr<TResponder::TAwaitHTTPLikeClient> TResponder::AwaitHTTPLike(
            const char* const host,
            const short port,
            const bool ssl,
            TAwaitHTTPLikeClient::TCallback&& cb,
            const size_t maxRetries
        ) const {
            AC_HTTP_RESPONSE_CHECKCLIENT2(std::shared_ptr<TAwaitHTTPLikeClient>())

            auto client = Client();
            return client->Connect<TAwaitHTTPLikeClient>(host, port, ssl, maxRetries, std::forward<TAwaitHTTPLikeClient::TCallback>(cb), client->GetNewSharedPtr());
        }

        void TResponder::OnWebSocketStart() const {
            AC_HTTP_RESPONSE_CHECKCLIENT()

            Client()->OnWebSocketStart((const std::shared_ptr<NHTTP::TRequest>)RequestPtr);
        }

        void TResponder::SetRequestPtr(std::shared_ptr<NHTTP::TRequest>& ptr) {
            if (!RequestPtr.expired()) {
                abort();
            }

            RequestPtr = ptr;
        }

        AC_HTTP_REPONDER_SEND_IMPL(const TBlob&);
        AC_HTTP_REPONDER_SEND_IMPL(TBlob&&);
        AC_HTTP_REPONDER_SEND_IMPL(const TBlobSequence&);
        AC_HTTP_REPONDER_SEND_IMPL(TBlobSequence&&);
    }
}
