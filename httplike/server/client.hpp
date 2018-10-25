#pragma once

#include <net/server/client.hpp>
#include <httplike/parser/parser.hpp>

namespace NAC {
    namespace NHTTPLikeServer {
        class TClient : public NNetServer::TNetClient {
        public:
            using TArgs = NNetServer::TNetClient::TArgs;

        public:
            using NNetServer::TNetClient::TNetClient;

            void Cb(const NMuhEv::TEvSpec& event) override;
            void PushWriteQueue(std::shared_ptr<NHTTPLikeParser::TParsedData> data);
            std::vector<std::shared_ptr<NHTTPLikeParser::TParsedData>> GetData();

        protected:
            void OnData(const size_t dataSize, char* data) override;
            virtual void OnData(std::shared_ptr<NHTTPLikeParser::TParsedData> request) = 0;

        private:
            NHTTPLikeParser::TParser Parser;
        };
    }
}
