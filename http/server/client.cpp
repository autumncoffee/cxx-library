#include "client.hpp"
#include "responder.hpp"
#include <iostream>

namespace NAC {
    namespace NHTTPServer {
        std::vector<std::shared_ptr<NWebSocketParser::TFrame>> TClient::GetWSData() {
            return WebSocketParser->ExtractData();
        }

        void TClient::OnData(std::shared_ptr<NHTTPLikeParser::TParsedData> request) {
            try {
                HandleRequest(request);

            } catch(...) {
                std::cerr << "[disaster]" << std::endl;
                Drop();
            }
        }

        void TClient::OnData(std::shared_ptr<NWebSocketParser::TFrame> frame) {
            try {
                if (frame->Opcode == 0xA) {
                    return;
                }

                if (frame->Opcode == 0x9) {
                    NWebSocketParser::TFrame pong;
                    pong.Opcode = 0xA;
                    pong.IsFin = true;
                    pong.Payload.Reserve(frame->Payload.Size());
                    pong.Payload.Append(frame->Payload.Size(), frame->Payload.Data());

                    WebSocketOrigin->Send(pong);

                    return;
                }

                if (frame->Opcode == 0x8) {
                    Drop(); // TODO: send close frame
                    return;
                }

                if (!frame->IsMasked) {
                    std::cerr << "[unmasked websocket frame received]" << std::endl;
                    Drop();
                    return;
                }

                if (frame->Opcode < 0x8) {
                    HandleFrame(frame, WebSocketOrigin);

                } else {
                    std::cerr << "[unknown websocket opcode: " << (size_t)(frame->Opcode) << "]" << std::endl;
                }

            } catch(...) {
                std::cerr << "[websocket disaster]" << std::endl;
                Drop();
            }
        }

        void TClient::HandleFrame(
            std::shared_ptr<NWebSocketParser::TFrame> frame,
            const std::shared_ptr<const NHTTP::TRequest> //origin
        ) {
            std::cerr << "[websocket frame ignored, opcode: " << (size_t)(frame->Opcode) << "]" << std::endl;
            // NWebSocketParser::TFrame out;
            // out.Opcode = frame->Opcode;
            // out.IsFin = true;
            // out.Payload.Reserve(frame->Payload.Size());
            // out.Payload.Append(frame->Payload.Size(), frame->Payload.Data());
            // origin->Send(out);
        }

        void TClient::PushWriteQueue(const NWebSocketParser::TFrame& frame) {
            struct TWriteQueueItem : public NNetServer::TNetClient::TWriteQueueItem {
                TBlob Orig;
            };

            std::shared_ptr<NNetServer::TNetClient::TWriteQueueItem> item_(
                new TWriteQueueItem
            );
            auto&& item = *(TWriteQueueItem*)item_.get();
            item.Orig = (TBlob)frame;
            item.Size = item.Orig.Size();
            item.Data = item.Orig.Data();

            PushWriteQueue(std::move(item_));
        }

        void TClient::Cb(const NMuhEv::TEvSpec& event) {
            NHTTPLikeServer::TClient::Cb(event);

            if (WebSocketParser) {
                const auto& data = GetWSData();

                for (const auto& node : data) {
                    OnData(node);
                }
            }
        }

        void TClient::OnData(const size_t dataSize, char* data) {
            if (WebSocketParser) {
                WebSocketParser->Add(dataSize, data);

            } else {
                NHTTPLikeServer::TClient::OnData(dataSize, data);
            }
        }

        void TClient::HandleRequest(std::shared_ptr<NHTTPLikeParser::TParsedData> data) {
            TResponder responder(GetNewSharedPtr<TClient>());

            try {
                try {
                    auto request_ = std::make_shared<NHTTP::TRequest>(data, responder);
                    auto request = std::shared_ptr<const NHTTP::TRequest>(request_, request_.get());
                    request_->SetWeakPtr(request);

                    try {
                        HandleRequestImpl(request);

                    } catch(const std::exception& e) {
                        HandleException(*request, e);
                    }

                } catch(const std::exception& e) {
                    responder.Respond(InternalServerError(e));
                }

            } catch(...) {
                responder.Respond(InternalServerError());
            }
        }

        void TClient::HandleRequestImpl(const std::shared_ptr<const NHTTP::TRequest> request) {
            std::cerr
                << "[access] "
                << request->FirstLine()
                << std::endl
            ;

            ((TArgs*)Args.get())->Handler.Handle(
                request,
                std::vector<std::string>()
            );
        }

        void TClient::HandleException(const NHTTP::TRequest& request, const std::exception& e) {
            std::cerr
                << "[error] "
                << request.FirstLine()
                << ": "
                << e.what()
                << std::endl
            ;

            request.Send(InternalServerErrorResponse());
        }

        NHTTP::TResponse TClient::InternalServerError(const std::exception& e) const {
            std::cerr << "[unhandled exception] " << e.what() << std::endl;

            return InternalServerErrorResponse();
        }

        NHTTP::TResponse TClient::InternalServerError() const {
            std::cerr << "[unknown exception] " << std::endl;

            return InternalServerErrorResponse();
        }

        NHTTP::TResponse TClient::InternalServerErrorResponse() const {
            NHTTP::TResponse response;

            response.FirstLine("HTTP/1.1 500 Internal Server Error\r\n");

            return response;
        }

        void TClient::OnWebSocketStart(const std::shared_ptr<const NHTTP::TRequest> request) {
            WebSocketOrigin = request;
            WebSocketParser.reset(new NWebSocketParser::TParser);
        }
    }
}
