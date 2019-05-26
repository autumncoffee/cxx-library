#include "client.hpp"
#include <ac-common/muhev.hpp>
#include <iostream>

namespace NAC {
    namespace NHTTPLikeServer {
        std::vector<std::shared_ptr<NHTTPLikeParser::TParsedData>> TClient::GetData() {
            return Parser.ExtractData();
        }

        void TClient::Cb(const NMuhEv::TEvSpec& event) {
            NNetServer::TNetClient::Cb(event);

            const auto& data = GetData();

            for (const auto& node : data) {
                // char firstLine[node->FirstLineSize + 1];
                // memcpy(firstLine, node->FirstLine, node->FirstLineSize);
                // firstLine[node->FirstLineSize] = '\0';
                //
                // std::cerr << firstLine << std::endl;
                //
                // for (const auto& header : node->Headers) {
                //     std::cerr << header.first << ": " << header.second << std::endl;
                // }
                //
                // if (node->BodySize > 0) {
                //     char body[node->BodySize + 1];
                //     memcpy(body, node->Body, node->BodySize);
                //     body[node->BodySize] = '\0';
                //
                //     std::cerr << body << std::endl;
                // }
                OnData(node);
            }
        }

        void TClient::OnData(const size_t dataSize, char* data) {
            Parser.Add(dataSize, data);
        }

        void TClient::PushWriteQueue(std::shared_ptr<NHTTPLikeParser::TParsedData> data) {
            struct TWriteQueueItem : public NNetServer::TNetClient::TWriteQueueItem {
                std::shared_ptr<NHTTPLikeParser::TParsedData> Orig;
            };

            std::shared_ptr<NNetServer::TNetClient::TWriteQueueItem> item_(
                new TWriteQueueItem
            );
            auto&& item = *(TWriteQueueItem*)item_.get();
            item.Concat(data->Request);
            item.Orig = data;

            PushWriteQueue(std::move(item_));
        }
    }
}
