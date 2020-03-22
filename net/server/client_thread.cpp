#include "server.hpp"
#include "client_thread.hpp"
#include "client.hpp"
#include "add_client.hpp"

#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>

namespace NAC {
    namespace NNetServer {
        TClientThreadArgs::TClientThreadArgs(
            TClientFactory& clientFactory,
            TClientArgsFactory& clientArgsFactory
        )
            : ClientFactory(clientFactory)
            , ClientArgsFactory(clientArgsFactory)
        {
        }

        TClientThread::TClientThread(std::shared_ptr<TClientThreadArgs> args)
            : NAC::NBase::TWorkerLite()
            , Args(args)
        {
            Acceptor = Loop.NewTrigger([this](){
                try {
                    Accept(Loop);

                } catch (const std::exception& e) {
                    std::cerr << "Failed to accept client: " << e.what() << std::endl;
                }
            });
        }

        void TClientThread::Run() {
            while (true) {
                decltype(ActiveClients) newActiveClients;

                for (auto client : ActiveClients) {
                    if (!client->IsAlive()) {
                        continue;
                    }

                    newActiveClients.emplace_back(client);
                }

                ActiveClients.swap(newActiveClients);

                if (!Loop.Wait()) {
                    perror("kevent");
                    abort();
                }
            }
        }

        void TClientThread::Accept(NMuhEv::TLoop& loop) {
            NUtils::TSpinLockGuard guard(Args->Mutex);

            while (!Args->Queue.empty()) {
                auto newClient = Args->Queue.front();
                Args->Queue.pop();

                std::unique_ptr<NNetServer::TBaseClient::TArgs> clientArgs(Args->ClientArgsFactory
                    ? Args->ClientArgsFactory()
                    : new NNetServer::TNetClient::TArgs
                );

                if (clientArgs->AddClient) {
                    abort();
                }

                clientArgs->Loop = &loop;

                if (auto* netClientArgs = dynamic_cast<NNetServer::TNetClient::TArgs*>(clientArgs.get())) {
                    netClientArgs->Fh = newClient->Fh;
                    netClientArgs->Addr = newClient->Addr;
                    netClientArgs->SSLCtx = Args->SSLCtx;
                    netClientArgs->UseSSL = Args->UseSSL;
                }

                auto&& addClient = clientArgs->AddClient = [this](std::shared_ptr<NNetServer::TBaseClient> client) {
                    client->SetWeakPtr(client);
                    client->OnWire();
                    ActiveClients.emplace_back(client);
                };

                std::shared_ptr<NNetServer::TBaseClient> client(
                    Args->ClientFactory(clientArgs.release())
                );

                addClient(client);
            }
        }
    }
}
