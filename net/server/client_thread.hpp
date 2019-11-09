#pragma once

#include <ac-common/worker_lite.hpp>
#include <unistd.h>
#include <memory>
#include <netinet/in.h>
#include <queue>
#include "client.hpp"
#include <ac-common/muhev.hpp>

namespace NAC {
    namespace NNetServer {
        using TClientFactory = std::function<TBaseClient*(TBaseClient::TArgs*)>;
        using TClientArgsFactory = std::function<TBaseClient::TArgs*()>;

        struct TNewClient {
            int Fh;
            std::shared_ptr<sockaddr_in> Addr;
        };

        struct TClientThreadArgs {
            std::queue<std::shared_ptr<TNewClient>> Queue;
            NUtils::TSpinLock Mutex;
            int Fds[2];

            TClientFactory& ClientFactory;
            TClientArgsFactory& ClientArgsFactory;

            SSL_CTX* SSLCtx = nullptr;
            bool UseSSL = false;

            TClientThreadArgs(TClientFactory&, TClientArgsFactory&);
            ~TClientThreadArgs();
        };

        class TClientThread : public NAC::NBase::TWorkerLite {
        public:
            explicit TClientThread(std::shared_ptr<TClientThreadArgs> args);

            void Run() override;

        private:
            void Accept(NMuhEv::TLoop& loop);

        private:
            std::shared_ptr<TClientThreadArgs> Args;
            std::deque<std::shared_ptr<NNetServer::TBaseClient>> ActiveClients;
            int WakeupFds[2];
            char WakeupContext;
            char AcceptContext;
        };
    }
}
