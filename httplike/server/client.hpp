#pragma once

#include <ac-library/net/server/client.hpp>
#include <ac-library/httplike/parser/parser.hpp>
#include <functional>

namespace NAC {
    namespace NHTTPLikeServer {
        class TClient : public NNetServer::TNetClient {
        public:
            using TArgs = NNetServer::TNetClient::TArgs;

        public:
            using NNetServer::TNetClient::TNetClient;
            using NNetServer::TNetClient::PushWriteQueue;

            void Cb(int, int) override;
            void PushWriteQueue(std::shared_ptr<NHTTPLikeParser::TParsedData> data);
            std::vector<std::shared_ptr<NHTTPLikeParser::TParsedData>> GetData();

        protected:
            void OnData(const size_t dataSize, char* data) override;
            virtual void OnData(std::shared_ptr<NHTTPLikeParser::TParsedData> request) = 0;

        private:
            NHTTPLikeParser::TParser Parser;
        };

        template<typename TBase = TClient>
        class TAwaitClient : public TBase {
            static_assert(std::is_base_of<TClient, TBase>::value);
            static_assert(!std::is_base_of<TAwaitClient, TBase>::value);

        public:
            using TCallback = std::function<void(
                std::shared_ptr<NHTTPLikeParser::TParsedData>,
                std::shared_ptr<TBase>
            )>;

            struct TArgs : public TBase::TArgs {
                TCallback Cb;
                std::shared_ptr<NNetServer::TBaseClient> OrigClient;

                template<typename... TArgs_>
                TArgs(
                    TCallback&& cb,
                    std::shared_ptr<NNetServer::TBaseClient> origClient,
                    TArgs_&&... args
                )
                    : TBase::TArgs(std::forward<TArgs_>(args)...)
                    , Cb(std::forward<TCallback>(cb))
                    , OrigClient(origClient)
                {
                }
            };

        public:
            using TBase::TBase;

            void OnData(std::shared_ptr<NHTTPLikeParser::TParsedData> response) override {
                ((TArgs*)TBase::Args.get())->Cb(response, TBase::template GetNewSharedPtr<TBase>());
            }
        };
    }
}
