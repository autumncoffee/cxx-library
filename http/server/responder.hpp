#pragma once

#include <http/response.hpp>
#include <functional>
#include <httplike/server/client.hpp>
#include <websocket/parser/parser.hpp>

namespace NAC {
    namespace NHTTP {
        class TRequest;
    };

    namespace NHTTPServer {
        class TClient;

        class TResponder {
        public:
            using TAwaitHTTPClient = NHTTPLikeServer::TAwaitClient<NHTTPLikeServer::TClient>;

        public:
            TResponder(std::shared_ptr<TClient> client);

            TResponder() = delete;
            TResponder(const TResponder&) = default;
            TResponder(TResponder&&) = default;

            void Respond(const NHTTP::TResponse& response) const;
            void Send(const NWebSocketParser::TFrame& response) const;

            std::shared_ptr<TResponder::TAwaitHTTPClient> AwaitHTTP(
                const char* const host,
                const short port,
                TAwaitHTTPClient::TCallback&& cb,
                const size_t maxRetries = 3
            ) const;

            void OnWebSocketStart() const;
            void SetRequestPtr(const std::shared_ptr<const NHTTP::TRequest>& ptr);

        private:
            std::weak_ptr<TClient> Client;
            std::weak_ptr<const NHTTP::TRequest> RequestPtr;
        };
    }
}
