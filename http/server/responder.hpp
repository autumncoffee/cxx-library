#pragma once

#include <http/response.hpp>
#include <functional>
#include <httplike/server/client.hpp>

namespace NAC {
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

            std::shared_ptr<TResponder::TAwaitHTTPClient> AwaitHTTP(
                const char* const host,
                const short port,
                TAwaitHTTPClient::TCallback&& cb,
                const size_t maxRetries = 3
            ) const;

        private:
            std::shared_ptr<TClient> Client;
        };
    }
}
