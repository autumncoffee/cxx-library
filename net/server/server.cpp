#include "server.hpp"

#include <muhev.hpp>
#include <fcntl.h>
#include <iostream>
#include <round_robin_vector.hpp>
#include <arpa/inet.h>

namespace {
    template<typename T>
    static inline void SetupSocket(
        const int fh,
        T& addr,
        const size_t backlog
    ) {
        static const int yes = 1;

        if(setsockopt(fh, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            perror("setsockopt");
            throw std::runtime_error("Socket error");
        }

        if(bind(fh, (sockaddr*)&addr, sizeof(addr)) != 0) {
            perror("bind");
            throw std::runtime_error("Socket error");
        }

        fcntl(fh, F_SETFL, fcntl(fh, F_GETFL, 0) | O_NONBLOCK);
        listen(fh, backlog);
    }
}

namespace NAC {
    using TClientThreadPair = std::pair<
        std::shared_ptr<NNetServer::TClientThreadArgs>,
        std::shared_ptr<NNetServer::TClientThread>
    >;

    template<>
    inline TClientThreadPair NUtils::TRoundRobinVector<TClientThreadPair>::Next() {
        return NextImpl();
    }

    namespace NNetServer {
        void TServer::Run() {
            std::vector<std::pair<int, std::shared_ptr<sockaddr_storage>>> binds;
            binds.reserve(2);

            if (Args.BindIP4 && Args.BindPort4) {
                in_addr ia;

                if (inet_pton(PF_INET, Args.BindIP4, &ia) < 1) {
                    perror("inet_pton");

                } else {
                    auto addr_ = std::make_shared<sockaddr_storage>();
                    auto&& addr = *(sockaddr_in*)addr_.get();
                    memset(&addr, 0, sizeof(sockaddr_in));

                    int fh = socket(PF_INET, SOCK_STREAM, 0);

                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(Args.BindPort4);
                    addr.sin_addr = ia;

                    SetupSocket(fh, addr, Args.Backlog);

                    binds.emplace_back(fh, addr_);
                }
            }

            if (Args.BindIP6 && Args.BindPort6) {
                in6_addr ia;

                if (inet_pton(PF_INET6, Args.BindIP6, &ia) < 1) {
                    perror("inet_pton");

                } else {
                    auto addr_ = std::make_shared<sockaddr_storage>();
                    auto&& addr = *(sockaddr_in6*)addr_.get();
                    memset(&addr, 0, sizeof(sockaddr_in6));

                    int fh = socket(PF_INET6, SOCK_STREAM, 0);

                    addr.sin6_family = AF_INET6;
                    addr.sin6_port = htons(Args.BindPort6);
                    addr.sin6_addr = ia;

                    SetupSocket(fh, addr, Args.Backlog);

                    binds.emplace_back(fh, addr_);
                }
            }

            if (binds.size() == 0) {
                std::cerr << "Nowhere to bind" << std::endl;
                return;
            }

            NUtils::TRoundRobinVector<TClientThreadPair> threads;

            for (size_t i = 0; i < Args.ThreadCount; ++i) {
                std::shared_ptr<TClientThreadArgs> args;
                args.reset(new TClientThreadArgs(
                    Args.ClientFactory,
                    Args.ClientArgsFactory
                ));

                std::shared_ptr<TClientThread> thread;
                thread.reset(new TClientThread(args));

                threads.push_back(TClientThreadPair(args, thread));

                thread->Start();
            }

            std::cerr << "ready!" << std::endl;

            NMuhEv::TLoop loop;

            while(true) {
                auto list = NMuhEv::MakeEvList(binds.size());

                for (const auto& node : binds) {
                    loop.AddEvent(NMuhEv::TEvSpec {
                        .Ident = (uintptr_t)std::get<0>(node),
                        .Filter = NMuhEv::MUHEV_FILTER_READ,
                        .Flags = NMuhEv::MUHEV_FLAG_NONE,
                        .Ctx = nullptr
                    }, list);
                }

                int triggeredCount = loop.Wait(list);

                if(triggeredCount < 0) {
                    perror("kevent");
                    abort();

                } else if(triggeredCount > 0) {
                    for(int i = 0; i < triggeredCount; ++i) {
                        const auto& event = NMuhEv::GetEvent(list, i);
                        auto addr = std::make_shared<sockaddr_in>();
                        socklen_t len = sizeof(sockaddr_in);

                        int fh = accept(event.Ident, (sockaddr*)addr.get(), &len);

                        if(fh < 0) {
                            perror("accept");
                            // free(addr);
                            continue;
                        }

                        std::shared_ptr<TNewClient> newClient;
                        newClient.reset(new TNewClient {
                            .Fh = fh,
                            .Addr = addr
                        });

                        auto thread = threads.Next();

                        NUtils::TSpinLockGuard guard(thread.first->Mutex);

                        thread.first->Queue.push(newClient);

                        // NUtils::cluck(1, "push_new_client()");
                        ::write(thread.first->Fds[1], "1", 1);
                    }
                }
            }
        }
    }
}
