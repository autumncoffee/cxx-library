#pragma once

#include "await_client_fwd.hpp"
#include "client.hpp"
#include <ac-library/http/message.hpp>
#include <memory>
#include <utility>
#include <functional>

namespace NAC {
    namespace NHTTP {
        class TIncomingResponse : public TMessageBase {
        public:
            TIncomingResponse(std::shared_ptr<NHTTPLikeParser::TParsedData> data);

            size_t StatusCode() const {
                return StatusCode_;
            }

        private:
            size_t StatusCode_ = 0;
        };
    }

    namespace NHTTPServer {
        template<typename TBase>
        class TAwaitClient : public TBase {
            static_assert(std::is_base_of<TClientBase, TBase>::value);
            static_assert(!std::is_base_of<TAwaitClient, TBase>::value);

        public:
            using TCallback = TAwaitClientCallback<TBase>;
            using TWSCallback = TAwaitClientWSCallback<TBase>;

            struct TArgs : public TBase::TArgs {
                TCallback Cb;
                TWSCallback WSCb;
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

                template<typename... TArgs_>
                TArgs(
                    TCallback&& cb,
                    TWSCallback&& wscb,
                    std::shared_ptr<NNetServer::TBaseClient> origClient,
                    TArgs_&&... args
                )
                    : TBase::TArgs(std::forward<TArgs_>(args)...)
                    , Cb(std::forward<TCallback>(cb))
                    , WSCb(std::forward<TWSCallback>(wscb))
                    , OrigClient(origClient)
                {
                }
            };

        public:
            using TBase::TBase;

            void HandleRequest(std::shared_ptr<NHTTPLikeParser::TParsedData> data) override {
                ((TArgs*)TBase::Args.get())->Cb(std::make_shared<NHTTP::TIncomingResponse>(data), TBase::template GetNewSharedPtr<TBase>());
            }

            void HandleFrame(std::shared_ptr<NWebSocketParser::TFrame> data, std::shared_ptr<NHTTP::TRequest> origin) override {
                auto&& wscb = ((TArgs*)TBase::Args.get())->WSCb;

                if (wscb) {
                    wscb(data, origin, TBase::template GetNewSharedPtr<TBase>());
                }
            }
        };
    }
}
