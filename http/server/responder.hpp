#pragma once

#include <http/response.hpp>

namespace NAC {
    namespace NHTTPServer {
        class TClient;

        class TResponder {
        public:
            TResponder(std::shared_ptr<TClient> client);

            TResponder() = delete;
            TResponder(const TResponder&) = default;
            TResponder(TResponder&&) = default;

            void Respond(const NHTTP::TResponse& response) const;

        private:
            std::shared_ptr<TClient> Client;
        };
    }
}
