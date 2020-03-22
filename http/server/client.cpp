#include "client.hpp"
#include "responder.hpp"
#include <iostream>

namespace NAC {
    namespace NHTTPServer {
        std::vector<std::shared_ptr<NWebSocketParser::TFrame>> TClientBase::GetWSData() {
            return WebSocketParser->ExtractData();
        }

        void TClientBase::OnData(std::shared_ptr<NHTTPLikeParser::TParsedData> request) {
            try {
                try {
                    HandleRequest(request);

                } catch (const std::exception& e) {
                    std::cerr << "[disaster]: " << e.what() << std::endl;
                    Drop();
                }

            } catch (...) {
                std::cerr << "[disaster]" << std::endl;
                Drop();
            }
        }

        void TClientBase::OnData(std::shared_ptr<NWebSocketParser::TFrame> frame) {
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

        void TClientBase::HandleFrame(
            std::shared_ptr<NWebSocketParser::TFrame> frame,
            std::shared_ptr<NHTTP::TRequest> //origin
        ) {
            std::cerr << "[websocket frame ignored, opcode: " << (size_t)(frame->Opcode) << "]" << std::endl;
            // NWebSocketParser::TFrame out;
            // out.Opcode = frame->Opcode;
            // out.IsFin = true;
            // out.Payload.Reserve(frame->Payload.Size());
            // out.Payload.Append(frame->Payload.Size(), frame->Payload.Data());
            // origin->Send(out);
        }

        void TClientBase::PushWriteQueue(const NWebSocketParser::TFrame& frame) {
            struct TWriteQueueItem : public NNetServer::TNetClient::TWriteQueueItem {
                TBlob Orig;
            };

            std::shared_ptr<NNetServer::TNetClient::TWriteQueueItem> item_(
                new TWriteQueueItem
            );
            auto&& item = *(TWriteQueueItem*)item_.get();
            item.Orig = (TBlob)frame;
            item.Concat(item.Orig);

            PushWriteQueue(std::move(item_));
        }

        void TClientBase::PushWriteQueue(const NHTTP::TResponse& response) {
            PushWriteQueueData((TBlobSequence)response);
        }

        void TClientBase::Cb(int filter, int flags) {
            NHTTPLikeServer::TClient::Cb(filter, flags);

            if (WebSocketParser) {
                const auto& data = GetWSData();

                for (const auto& node : data) {
                    OnData(node);
                }
            }
        }

        void TClientBase::OnData(const size_t dataSize, char* data) {
            if (WebSocketParser) {
                // std::cerr << "OnData(" << std::to_string(dataSize) << "): ";
                //
                // for (size_t i = 0; i < dataSize; ++i) {
                //     static const char CharTable[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                //     std::cerr << CharTable[((unsigned char)(data[i])) >> 4] << CharTable[((unsigned char)(data[i])) & 0xf] << ' ';
                // }
                //
                // std::cerr << std::endl;
                WebSocketParser->Add(dataSize, data);

            } else {
                NHTTPLikeServer::TClient::OnData(dataSize, data);
            }
        }

        void TClient::HandleRequest(std::shared_ptr<NHTTPLikeParser::TParsedData> data) {
            TResponder responder(GetNewSharedPtr<TClient>());
            auto&& requestFactory = ((TArgs*)Args.get())->RequestFactory;

            try {
                try {
                    std::shared_ptr<NHTTP::TRequest> request(requestFactory
                        ? requestFactory(data, responder)
                        : new NHTTP::TRequest(data, responder)
                    );
                    request->SetWeakPtr(request);

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

        void TClient::HandleRequestImpl(std::shared_ptr<NHTTP::TRequest> request) {
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

        void TClient::HandleException(NHTTP::TRequest& request, const std::exception& e) {
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

        void TClientBase::OnWebSocketStart(std::shared_ptr<NHTTP::TRequest> request) {
            WebSocketOrigin = request;
            WebSocketParser.reset(new NWebSocketParser::TParser);
        }
    }
}
