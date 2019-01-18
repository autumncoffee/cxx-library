#pragma once

#include <worker_lite.hpp>
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

                template<typename T>
                static inline TClientFactory MakeClientFactory() {
                    static_assert(std::is_base_of<TBaseClient, T>::value);
                    static_assert(std::is_base_of<TBaseClient::TArgs, typename T::TArgs>::value);

                    return [](TBaseClient::TArgs* const args) -> TBaseClient* {
                        return new T(args);
                    };
                }
            };

        public:
            TServer(const TArgs& args)
                : NAC::NBase::TWorkerLite()
                , Args(args)
            {
            }

            TServer(const TServer&) = delete;
            TServer(TServer&&) = delete;

            void Run() override;

        private:
            TArgs Args;
        };
    }
}
