#include "responder.hpp"
#include "client.hpp"

namespace NAC {
    namespace NHTTPServer {
        TResponder::TResponder(std::shared_ptr<TClient> client)
            : Client(client)
        {
        }

        void TResponder::Respond(const NHTTP::TResponse& response) const {
            Client->PushWriteQueue((std::shared_ptr<NHTTPLikeParser::TParsedData>)response);
        }

        std::shared_ptr<NHTTPLikeServer::TClient> TResponder::AwaitHTTP(
            const char* const host,
            const short port,
            TAwaitHTTPCallback&& cb,
            const size_t maxRetries
        ) const {
            class TAwaitHTTPClient : public NHTTPLikeServer::TClient {
            public:
                struct TArgs : public NHTTPLikeServer::TClient::TArgs {
                    TAwaitHTTPCallback Cb;
                    std::shared_ptr<NNetServer::TBaseClient> OrigClient;

                    TArgs(
                        TAwaitHTTPCallback&& cb,
                        std::shared_ptr<NNetServer::TBaseClient> origClient
                    )
                        : Cb(cb)
                        , OrigClient(origClient)
                    {
                    }
                };

            public:
                using NHTTPLikeServer::TClient::TClient;

                void OnData(std::shared_ptr<NHTTPLikeParser::TParsedData> response) override {
                    ((TArgs*)Args.get())->Cb(response, GetNewSharedPtr<NHTTPLikeServer::TClient>());
                }
            };

            auto out = Client->Connect<TAwaitHTTPClient>(host, port, maxRetries, cb, Client->GetNewSharedPtr());

            if (out) {
                return std::shared_ptr<NHTTPLikeServer::TClient>(out, (NHTTPLikeServer::TClient*)out.get());
            }

            return std::shared_ptr<NHTTPLikeServer::TClient>();
        }
    }
}
