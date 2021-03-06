#pragma once

#include <ac-common/worker_lite.hpp>
#include <unistd.h>
#include <functional>
#include "client_thread.hpp"

namespace NAC {
    namespace NNetServer {
        class TServer : public NAC::NBase::TWorkerLite {
        public:
            struct TArgs {
                size_t ThreadCount = 1;
                size_t Backlog = 10000;

                const char* BindIP4 = "127.0.0.1";
                unsigned short BindPort4 = 0;

                const char* BindIP6 = "::1";
                unsigned short BindPort6 = 0;

                TClientFactory ClientFactory;
                TClientArgsFactory ClientArgsFactory;

                SSL_CTX* SSLCtx = nullptr;
                bool UseSSL = false;
                bool InitSSL = true;

                template<typename T>
                static inline TClientFactory MakeClientFactory() {
                    static_assert(std::is_base_of<TBaseClient, T>::value);
                    static_assert(std::is_base_of<TBaseClient::TArgs, typename T::TArgs>::value);

                    return [](TBaseClient::TArgs* const args) -> TBaseClient* {
                        return new T((typename T::TArgs*)args);
                    };
                }

                virtual ~TArgs() {
                }
            };

        public:
            TServer(const TArgs& args);
            TServer(const TServer&) = delete;
            TServer(TServer&&) = delete;

            void Run() override;

        private:
            TArgs Args;
        };
    }
}
