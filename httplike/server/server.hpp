#pragma once

#include <worker_lite.hpp>
#include <unistd.h>
#include <functional>
#include "client_thread.hpp"

namespace NAC {
    namespace NHTTPLikeServer {
        class TServer : public NAC::NBase::TWorkerLite {
        public:
            struct TArgs {
                size_t ThreadCount = 1;
                size_t Backlog = 10000;

                const char* BindIP4 = "127.0.0.1";
                short BindPort4 = 0;

                const char* BindIP6 = "::1";
                short BindPort6 = 0;
            };

        public:
            TServer(const TArgs& args, const TDataCallback& onData)
                : NAC::NBase::TWorkerLite()
                , OnData(onData)
                , Args(args)
            {
            }

            TServer(const TArgs& args, TDataCallback&& onData)
                : NAC::NBase::TWorkerLite()
                , OnData(std::move(onData))
                , Args(args)
            {
            }

            TServer(const TServer&) = delete;
            TServer(TServer&&) = delete;

            virtual void Run() override;

        private:
            TDataCallback OnData;
            const TArgs Args;
        };
    }
}
