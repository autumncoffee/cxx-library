#pragma once

#include <http/response.hpp>
#include <functional>
#include <httplike/server/client.hpp>

namespace NAC {
    namespace NHTTPServer {
        class TClient;

        class TResponder {
        public:
            using TAwaitHTTPCallback = std::function<void(
                std::shared_ptr<NHTTPLikeParser::TParsedData>,
                std::shared_ptr<NHTTPLikeServer::TClient>
            )>;

        public:
            TResponder(std::shared_ptr<TClient> client);

            TResponder() = delete;
            TResponder(const TResponder&) = default;
            TResponder(TResponder&&) = default;

            void Respond(const NHTTP::TResponse& response) const;

            std::shared_ptr<NHTTPLikeServer::TClient> AwaitHTTP(
                const char* const host,
                const short port,
                TAwaitHTTPCallback&& cb,
                const size_t maxRetries = 3
            ) const;

        private:
            std::shared_ptr<TClient> Client;
        };
    }
}
