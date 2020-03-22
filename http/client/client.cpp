#include <ac-library/http/server/await_client.hpp>
#include "client.hpp"
#include <ac-library/net/client/connect.hpp>
#include <ac-library/net/server/client.hpp>

namespace NAC {
    namespace NHTTPClient {
        std::shared_ptr<TAwaitHTTPClient> DoConnect(
            std::unique_ptr<typename TAwaitHTTPClient::TArgs>&& clientArgs,
            NMuhEv::TLoop* loop,
            const std::string& hostname,
            unsigned short port,
            void* sslCtx
        ) {
            std::shared_ptr<sockaddr_in> addr;
            socklen_t addrLen;
            int fh;

            if (!NNetClient::Connect(hostname.c_str(), port, addr, addrLen, fh, /* maxRetries = */3)) {
                return std::unique_ptr<TAwaitHTTPClient>();
            }

            clientArgs->Loop = loop;
            clientArgs->Fh = fh;
            clientArgs->Addr = addr;
            clientArgs->SSLIsClient = true;
            clientArgs->SSLCtx = (SSL_CTX*)sslCtx;
            clientArgs->UseSSL = (sslCtx != nullptr);

            auto client = std::make_shared<TAwaitHTTPClient>(clientArgs.release());

            client->SetWeakPtr(client);
            client->OnWire();

            return client;
        }

        void DoRequest(
            const NHTTP::TResponse& request,
            TAwaitHTTPClient& client
        ) {
            auto out = std::make_shared<NNetServer::TNetClient::TWriteQueueItem>();
            out->Concat((TBlobSequence)request);
            client.PushWriteQueue(out);
        }
    }
}
