#include "server.hpp"
#include "client_thread.hpp"
#include "client.hpp"

#include <muhev.hpp>
#include <utils/socket.hpp>
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
            for(int i = 0; i < 2; ++i) {
                close(Fds[i]);
            }
        }

        TClientThread::TClientThread(std::shared_ptr<TClientThreadArgs> args)
            : NAC::NBase::TWorkerLite()
            , Args(args)
        {
            AcceptContext = WakeupContext = '\0';

            if(socketpair(PF_LOCAL, SOCK_STREAM, 0, WakeupFds) == -1) {
                perror("socketpair");
                abort();
            }
        }

        void TClientThread::Run() {
            NMuhEv::TLoop loop;

            while(true) {
                auto list = NMuhEv::MakeEvList(ActiveClients.size() + 2);
                decltype(ActiveClients) newActiveClients;

                for(auto client : ActiveClients) {
                    if(!client->IsAlive()) {
                        continue;
                    }

                    newActiveClients.push_back(client);

                    loop.AddEvent(NMuhEv::TEvSpec {
                        .Ident = (uintptr_t)client->GetFh(),
                        .Filter = (NMuhEv::MUHEV_FILTER_READ | (
                            client->ShouldWrite() ? NMuhEv::MUHEV_FILTER_WRITE : 0
                        )),
                        .Flags = NMuhEv::MUHEV_FLAG_NONE,
                        .Ctx = client.get()
                    }, list);
                }

                ActiveClients.swap(newActiveClients);

                loop.AddEvent(NMuhEv::TEvSpec {
                    .Ident = (uintptr_t)Args->Fds[0],
                    .Filter = NMuhEv::MUHEV_FILTER_READ,
                    .Flags = NMuhEv::MUHEV_FLAG_NONE,
                    .Ctx = &AcceptContext
                }, list);

                loop.AddEvent(NMuhEv::TEvSpec {
                    .Ident = (uintptr_t)WakeupFds[0],
                    .Filter = NMuhEv::MUHEV_FILTER_READ,
                    .Flags = NMuhEv::MUHEV_FLAG_NONE,
                    .Ctx = &WakeupContext
                }, list);

                int triggeredCount = loop.Wait(list);
                // NUtils::cluck(3, "Got %d events in thread %llu", triggeredCount, NUtils::ThreadId());
                if(triggeredCount < 0) {
                    perror("kevent");
                    abort();

                } else if(triggeredCount > 0) {
                    for(int i = 0; i < triggeredCount; ++i) {
                        const auto& event = NMuhEv::GetEvent(list, i);

                        if((event.Ctx == &AcceptContext) || (event.Ctx == &WakeupContext)) {
                            // if(event.Ident == ((Args*)args)->fds[0]) {
                                if(event.Flags & NMuhEv::MUHEV_FLAG_EOF) {
                                    // NUtils::cluck(1, "EV_EOF");
                                }

                                if(event.Flags & NMuhEv::MUHEV_FLAG_ERROR) {
                                    // NUtils::cluck(1, "EV_ERROR");
                                }

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

                                    if(rv < 0) {
                                        perror("recvfrom");
                                        abort();
                                    }
                                }

                                // NUtils::cluck(1, "accept()");
                                if(event.Ctx == &AcceptContext) {
                                    Accept();
                                }

                            // } else {
                            //     NUtils::cluck(1, "wut");
                            // }

                        } else {
                            auto client = (NNetServer::TBaseClient*)event.Ctx;

                            if(client->IsAlive()) {
                                // NUtils::cluck(1, "client_cb()");
                                client->Cb(event);

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

        void TClientThread::Accept() {
            NUtils::TSpinLockGuard guard(Args->Mutex);

            while(!Args->Queue.empty()) {
                auto newClient = Args->Queue.front();
                Args->Queue.pop();

                std::shared_ptr<NNetServer::TBaseClient> client(
                    Args->ClientFactory(
                        (Args->ClientArgsFactory
                            ? Args->ClientArgsFactory()
                            : new NNetServer::TBaseClient::TArgs
                        ),
                        newClient->Fh,
                        WakeupFds[1],
                        newClient->Addr
                    )
                );

                client->SetWeakPtr(client);
                ActiveClients.push_back(client);
            }
        }
    }
}
