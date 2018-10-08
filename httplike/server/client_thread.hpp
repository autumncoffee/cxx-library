#pragma once

#include <worker_lite.hpp>
#include <unistd.h>
#include <memory>
#include <netinet/in.h>
#include <queue>
#include <httplike/server/client.hpp>
#include <httplike/parser/parser.hpp>

namespace NAC {
    namespace NHTTPLikeServer {
        using TDataCallback = std::function<std::shared_ptr<NHTTPLikeParser::TParsedData>(std::shared_ptr<NHTTPLikeParser::TParsedData> request)>;

        struct TNewClient {
            int Fh;
            std::shared_ptr<sockaddr_in> Addr;
        };

        struct TClientThreadArgs {
            TDataCallback& OnData;
            std::queue<std::shared_ptr<TNewClient>> Queue;
            NUtils::TSpinLock Mutex;
            int Fds[2];

            TClientThreadArgs(TDataCallback& onData);
            ~TClientThreadArgs();
        };

        class TClientThread : public NAC::NBase::TWorkerLite {
        public:
            explicit TClientThread(std::shared_ptr<TClientThreadArgs> args);

            virtual void Run() override;

        private:
            void accept();

        private:
            std::shared_ptr<TClientThreadArgs> Args;
            std::deque<std::shared_ptr<NHTTPLikeServer::TClient>> ActiveClients;
            int WakeupFds[2];
            char WakeupContext;
            char AcceptContext;
        };
    }
}
