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
            if(socketpair(PF_LOCAL, SOCK_STREAM, 0, Fds) == -1) {
                perror("socketpair");
                abort();
            }
        }

        TClientThreadArgs::~TClientThreadArgs() {
            for (int i = 0; i < 2; ++i) {
                close(Fds[i]);
            }
        }

        TClientThread::TClientThread(std::shared_ptr<TClientThreadArgs> args)
            : NAC::NBase::TWorkerLite()
            , Args(args)
        {
            AcceptContext = WakeupContext = '\0';

            if (socketpair(PF_LOCAL, SOCK_STREAM, 0, WakeupFds) == -1) {
                perror("socketpair");
                abort();
            }
        }

        void TClientThread::Run() {
            NMuhEv::TLoop loop;

            loop.AddEvent(NMuhEv::TEvSpec {
                .Ident = (uintptr_t)Args->Fds[0],
                .Filter = NMuhEv::MUHEV_FILTER_READ,
                .Flags = NMuhEv::MUHEV_FLAG_NONE,
                .Ctx = &AcceptContext
            }, /* mod = */false);

            loop.AddEvent(NMuhEv::TEvSpec {
                .Ident = (uintptr_t)WakeupFds[0],
                .Filter = NMuhEv::MUHEV_FILTER_READ,
                .Flags = NMuhEv::MUHEV_FLAG_NONE,
                .Ctx = &WakeupContext
            }, /* mod = */false);

            while (true) {
                std::vector<NMuhEv::TEvSpec> list;
                list.reserve(ActiveClients.size() + 2);

                decltype(ActiveClients) newActiveClients;

                for (auto client : ActiveClients) {
                    if (!client->IsAlive()) {
                        continue;
                    }

                    newActiveClients.emplace_back(client);
                }

                ActiveClients.swap(newActiveClients);

                bool ok = loop.Wait(list);
                // NUtils::cluck(3, "Got %d events in thread %llu", triggeredCount, NUtils::ThreadId());

                if (!ok) {
                    perror("kevent");
                    abort();

                } else if (list.size() > 0) {
                    for (const auto& event : list) {
                        if ((event.Ctx == &AcceptContext) || (event.Ctx == &WakeupContext)) {
                            {
                                char dummy[128];
                                int rv = recvfrom(
                                    event.Ident,
                                    dummy,
                                    128,
                                    MSG_DONTWAIT,
                                    NULL,
                                    0
                                );

                                if (rv < 0) {
                                    perror("recvfrom");
                                    abort();
                                }
                            }

                            if (event.Ctx == &AcceptContext) {
                                try {
                                    Accept(loop);

                                } catch (const std::exception& e) {
                                    std::cerr << "Failed to accept client: " << e.what() << std::endl;
                                }
                            }

                        } else {
                            auto client = (NNetServer::TBaseClient*)event.Ctx;

                            if (client->IsAlive()) {
                                try {
                                    client->Cb(event);

                                } catch (const std::exception& e) {
                                    std::cerr << "Failed to execute client callback: " << e.what() << std::endl;
                                    client->Drop();
                                }

                            } else {
                                // TODO
                                // NUtils::cluck(1, "dead client");
                            }
                        }
                    }

                } else {
                    // NUtils::cluck(1, "nothing is here");
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
                clientArgs->WakeupFd = WakeupFds[1];

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
