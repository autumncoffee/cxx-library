#pragma once

#include <ac-library/http/response.hpp>
#include <functional>
#include <ac-library/httplike/server/client.hpp>
#include <ac-library/websocket/parser/parser.hpp>
#include <ac-common/str.hpp>
#include <ac-common/string_sequence.hpp>
#include <ac-library/http/server/await_client_fwd.hpp>

namespace NAC {
    namespace NHTTP {
        class TRequest;
    }

    namespace NHTTPServer {
        class TClient;

        class TResponder {
        public:
            using TAwaitHTTPClient = NHTTPServer::TAwaitClient<>;
            using TAwaitHTTPLikeClient = NHTTPLikeServer::TAwaitClient<>;

        public:
            TResponder(std::shared_ptr<TClient> client);

            TResponder() = delete;
            TResponder(const TResponder&) = default;
            TResponder(TResponder&&) = default;

            void Respond(const NHTTP::TResponse& response) const;
            void Respond(const NHTTP::TResponse& response, NNetServer::TWQCB&& cb) const;
            void Respond(const NHTTP::TResponse& response, const NNetServer::TWQCB& cb) const;

            void Send(const NWebSocketParser::TFrame& response) const;

            void Send(const TBlob&) const;
            void Send(TBlob&&) const;

            void Send(const TBlobSequence&) const;
            void Send(TBlobSequence&&) const;

            std::shared_ptr<TResponder::TAwaitHTTPClient> AwaitHTTP(
                const char* const host,
                const short port,
                const bool ssl,
                TAwaitClientCallback<>&& cb,
                const size_t maxRetries = 3
            ) const;

            std::shared_ptr<TResponder::TAwaitHTTPClient> AwaitHTTP(
                const char* const host,
                const short port,
                TAwaitClientCallback<>&& cb,
                const size_t maxRetries = 3
            ) const {
                return AwaitHTTP(host, port, false, std::forward<TAwaitClientCallback<>>(cb), maxRetries);
            }

            std::shared_ptr<TResponder::TAwaitHTTPClient> AwaitHTTP(
                const char* const host,
                const short port,
                const bool ssl,
                TAwaitClientCallback<>&& cb,
                TAwaitClientWSCallback<>&& wscb,
                const size_t maxRetries = 3
            ) const;

            std::shared_ptr<TResponder::TAwaitHTTPClient> AwaitHTTP(
                const char* const host,
                const short port,
                TAwaitClientCallback<>&& cb,
                TAwaitClientWSCallback<>&& wscb,
                const size_t maxRetries = 3
            ) const {
                return AwaitHTTP(host, port, false, std::forward<TAwaitClientCallback<>>(cb), std::forward<TAwaitClientWSCallback<>>(wscb), maxRetries);
            }

            std::shared_ptr<TResponder::TAwaitHTTPLikeClient> AwaitHTTPLike(
                const char* const host,
                const short port,
                const bool ssl,
                TAwaitHTTPLikeClient::TCallback&& cb,
                const size_t maxRetries = 3
            ) const;

            std::shared_ptr<TResponder::TAwaitHTTPLikeClient> AwaitHTTPLike(
                const char* const host,
                const short port,
                TAwaitHTTPLikeClient::TCallback&& cb,
                const size_t maxRetries = 3
            ) const {
                return AwaitHTTPLike(host, port, false, std::forward<TAwaitHTTPLikeClient::TCallback>(cb), maxRetries);
            }

            void OnWebSocketStart() const;
            void SetRequestPtr(std::shared_ptr<NHTTP::TRequest>& ptr);

        private:
            std::shared_ptr<TClient> Client() const {
                return (std::shared_ptr<TClient>)Client_;
            }

        private:
            std::weak_ptr<TClient> Client_;
            std::weak_ptr<NHTTP::TRequest> RequestPtr;
        };
    }
}
