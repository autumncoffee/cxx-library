#pragma once

#include <memory>
#include <ac-common/muhev.hpp>
#include <utility>
#include <ac-library/http/response.hpp>
#include <ac-library/http/server/await_client_fwd.hpp>

namespace NAC {
    namespace NHTTPClient {
        using TAwaitHTTPClient = NAC::NHTTPServer::TAwaitClient<>;

        std::shared_ptr<TAwaitHTTPClient> DoConnect(
            std::unique_ptr<typename TAwaitHTTPClient::TArgs>&& clientArgs,
            NMuhEv::TLoop* loop,
            const std::string& hostname,
            unsigned short port,
            void* sslCtx = nullptr
        );

        template<typename TCb, typename... TArgs>
        std::shared_ptr<TAwaitHTTPClient> Connect(
            TCb&& cb,
            TArgs&&... args
        ) {
            std::unique_ptr<typename TAwaitHTTPClient::TArgs> clientArgs(
                new typename TAwaitHTTPClient::TArgs(
                    std::forward<TCb>(cb),
                    nullptr
                )
            );

            return DoConnect(std::move(clientArgs), std::forward<TArgs>(args)...);
        }

        void DoRequest(
            const NHTTP::TResponse& request,
            TAwaitHTTPClient& client
        );

        template<typename... TArgs>
        std::shared_ptr<TAwaitHTTPClient> Request(
            const NHTTP::TResponse& request,
            TArgs&&... args
        ) {
            auto client = Connect(std::forward<TArgs>(args)...);

            DoRequest(request, *client);

            return client;
        }
    }
}
