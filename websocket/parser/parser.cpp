#include "parser.hpp"
#include <arpa/inet.h>
#include <ac-common/spin_lock.hpp>
#include <cmath>
#include <cstdint>
#include <assert.h>
#include <iostream>

#ifndef htonll
#define htonll(x) ((1 == htonl(1)) ? (x) : ((uint64_t)htonl(\
    (x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#endif

#ifndef ntohll
#define ntohll(x) ((1 == ntohl(1)) ? (x) : ((uint64_t)ntohl(\
    (x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

namespace NAC {
    namespace NWebSocketParser {
        TBlob TFrame::Header() const {
            static const char dummy[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            static_assert(sizeof(dummy) == 14);

            TBlob out;
            out.Reserve(sizeof(dummy));
            out.Append(sizeof(dummy), dummy);

            auto* rawOut = out.Data();

            {
                unsigned char firstByte(Opcode);

                if (IsFin) {
                    firstByte |= 128;
                }

                if (RSV1) {
                    firstByte |= 64;
                }

                if (RSV2) {
                    firstByte |= 32;
                }

                if (RSV3) {
                    firstByte |= 16;
                }

                rawOut[0] = firstByte;
            }

            size_t offset = 2;

            {
                unsigned char secondByte(0);
                uint64_t payloadSize(Payload.Size());

                if (payloadSize < 126) {
                    secondByte = payloadSize;
                    payloadSize = 0;

                } else if (payloadSize < powl(2, 16)) {
                    uint16_t shortSize(payloadSize);

                    secondByte = 126;
                    shortSize = htons(shortSize);

                    memcpy(&rawOut[offset], &shortSize, sizeof(shortSize));
                    offset += sizeof(shortSize);

                } else {
                    secondByte = 127;
                    payloadSize = htonll(payloadSize);

                    memcpy(&rawOut[offset], &payloadSize, sizeof(payloadSize));
                    offset += sizeof(payloadSize);
                }

                if (IsMasked) {
                    secondByte |= 128;
                }

                rawOut[1] = secondByte;
            }

            if (IsMasked) {
                memcpy(&rawOut[offset], Mask, sizeof(Mask));
                offset += sizeof(Mask);
            }

            out.Shrink(offset);

            return out;
        }

        TBlob TFrame::MaskedPayload() const {
            TBlob out;
            out.Reserve(Payload.Size());
            auto* outData = out.Data();
            auto* inData = Payload.Data();

            for (size_t i = 0; i < Payload.Size(); ++i) {
                outData[i] = inData[i] ^ Mask[i % sizeof(Mask)];
            }

            return out;
        }

        TFrame::operator TBlob() const {
            TBlob out;
            auto header = Header();

            out.Reserve(header.Size() + Payload.Size());
            out.Append(header.Size(), header.Data());

            if (IsMasked) {
                auto payload = MaskedPayload();
                out.Append(payload.Size(), payload.Data());

            } else {
                out.Append(Payload.Size(), Payload.Data());
            }

            return out;
        }

        class TParserState {
        private:
            enum TStage {
                STAGE_START,
                STAGE_PAYLOAD_SIZE,
                STAGE_MASK_KEY,
                STAGE_PAYLOAD
            };

        private:
            inline void ProcessMessage(const TBlob& msg) {
                size_t msgOffset = 0;
                auto* data = msg.Data();

                if (!CurrentFrame) {
                    if (msg.Size() >= 2) {
                        msgOffset += 2;

                        if (Buf.Size() > 0) {
                            Offset += 2;
                        }

                        CurrentFrame.reset(new TFrame);
                        auto& frame = *CurrentFrame;
                        // std::cerr << "[ws] create frame" << std::endl;

                        {
                            const unsigned char firstByte(data[0]);

                            frame.IsFin = firstByte & 128;
                            frame.RSV1 = firstByte & 64;
                            frame.RSV2 = firstByte & 32;
                            frame.RSV3 = firstByte & 16;
                            frame.Opcode = firstByte & (8 + 4 + 2 + 1);

                            // std::cerr << "[ws] opcode: " << std::to_string(frame.Opcode) << std::endl;
                        }

                        {
                            unsigned char secondByte(data[1]);

                            frame.IsMasked = secondByte & 128;

                            if (frame.IsMasked) {
                                secondByte ^= 128;
                            }

                            switch (secondByte) {
                                case 127: {
                                    SizeLen = sizeof(uint64_t);
                                    break;
                                }

                                case 126: {
                                    SizeLen = sizeof(uint16_t);
                                    break;
                                }

                                default:
                                    PayloadLen = secondByte;
                            }

                            // std::cerr << "[ws] SizeLen: " << std::to_string(SizeLen) << std::endl;
                            // std::cerr << "[ws] PayloadLen: " << std::to_string(PayloadLen) << std::endl;
                        }

                    } else if (Buf.Size() == 0) {
                        Buf.Append(msg.Size(), msg.Data());
                        // std::cerr << "[ws] bufferize [1]" << std::endl;
                    }
                }

                if (!CurrentFrame) {
                    // std::cerr << "[ws] no current frame" << std::endl;
                    return;
                }

                if (Stage == STAGE_START) {
                    if (SizeLen > 0) {
                        Stage = STAGE_PAYLOAD_SIZE;

                    } else {
                        Stage = STAGE_MASK_KEY;
                    }
                }

                // std::cerr << "[ws] stage: " << std::to_string(Stage) << std::endl;

                if (Stage == STAGE_PAYLOAD_SIZE) {
                    const size_t leftoverSize(msg.Size() - msgOffset);

                    if (leftoverSize < SizeLen) {
                        if ((Buf.Size() == 0) && (leftoverSize > 0)) {
                            Buf.Append(leftoverSize, msg.Data() + msgOffset);
                            // std::cerr << "[ws] bufferize [2]" << std::endl;
                        }

                        // std::cerr << "[ws] waiting for more [1]" << std::endl;

                        return;
                    }

                    switch (SizeLen) {
                        case sizeof(uint16_t): {
                            uint16_t shortPayloadLen;
                            memcpy(&shortPayloadLen, &data[msgOffset], sizeof(shortPayloadLen));
                            PayloadLen = ntohs(shortPayloadLen);
                            break;
                        }

                        case sizeof(uint64_t): {
                            memcpy(&PayloadLen, &data[msgOffset], sizeof(PayloadLen));
                            PayloadLen = ntohll(PayloadLen);
                            break;
                        }

                        default:
                            assert(false);
                    }

                    Stage = STAGE_MASK_KEY;
                    msgOffset += SizeLen;

                    if (Buf.Size() > 0) {
                        Offset += SizeLen;
                    }

                    SizeLen = 0;
                }

                if (Stage == STAGE_MASK_KEY) {
                    if (CurrentFrame->IsMasked) {
                        const size_t leftoverSize(msg.Size() - msgOffset);

                        if (leftoverSize < sizeof(CurrentFrame->Mask)) {
                            if ((Buf.Size() == 0) && (leftoverSize > 0)) {
                                Buf.Append(leftoverSize, msg.Data() + msgOffset);
                                // std::cerr << "[ws] bufferize [3]" << std::endl;
                            }

                            return;
                        }

                        memcpy(CurrentFrame->Mask, &data[msgOffset], sizeof(CurrentFrame->Mask));
                        msgOffset += sizeof(CurrentFrame->Mask);

                        if (Buf.Size() > 0) {
                            Offset += sizeof(CurrentFrame->Mask);
                        }
                    }

                    Stage = STAGE_PAYLOAD;
                }

                assert(Stage == STAGE_PAYLOAD);

                if (PayloadLen == 0) {
                    Flush();

                } else if ((msg.Size() - msgOffset) >= PayloadLen) {
                    // std::cerr << "[ws] payload len: " << std::to_string(PayloadLen) << std::endl;
                    CurrentFrame->Payload.Reserve(PayloadLen);
                    CurrentFrame->Payload.Append(PayloadLen, msg.Data() + msgOffset);
                    msgOffset += PayloadLen;

                    if (Buf.Size() > 0) {
                        Offset += PayloadLen;
                    }

                    if (CurrentFrame->IsMasked) {
                        auto* data = CurrentFrame->Payload.Data();

                        for (size_t i = 0; i < PayloadLen; ++i) {
                            data[i] = data[i] ^ CurrentFrame->Mask[i % sizeof(CurrentFrame->Mask)];
                        }
                    }

                    PayloadLen = 0;

                    Flush();
                }

                if ((msg.Size() > msgOffset) && (Buf.Size() == 0)) {
                    Buf.Append(msg.Size() - msgOffset, msg.Data() + msgOffset);
                    // std::cerr << "[ws] bufferize [4]" << std::endl;
                }
            }

        public:
            inline void ParseData(const size_t dataSize, char* data) {
                bool first(true);

                while (true) {
                    const size_t frameCount(Frames.size());
                    TBlob msg;

                    if (Buf.Size() > 0) {
                        if (first) {
                            Buf.Append(dataSize, data);
                        }

                        msg.Wrap(Buf.Size() - Offset, Buf.Data() + Offset, /* own = */false);

                    } else if (!first) {
                        std::cerr << "[websocket] weird loop" << std::endl;
                        break;

                    } else {
                        msg.Wrap(dataSize, data, /* own = */false);
                    }

                    ProcessMessage(msg);

                    if ((frameCount == Frames.size()) || ((Buf.Size() - Offset) == 0)) {
                        break;
                    }

                    first = false;
                }

                if ((Buf.Size() > 0) && (Buf.Size() == Offset)) {
                    // std::cerr << "[ws] shrink" << std::endl;
                    Buf.Shrink(0);
                    Offset = 0;
                }

                if (Offset > (10 * 1024 * 1024)) {
                    // std::cerr << "[ws] chop" << std::endl;
                    Buf.Chop(Offset);
                    Offset = 0;
                }
            }

            std::vector<std::shared_ptr<TFrame>> ExtractData() {
                std::vector<std::shared_ptr<TFrame>> out;

                {
                    NUtils::TSpinLockGuard guard(FramesLock);
                    std::swap(Frames, out);
                }

                return out;
            }

        private:
            void Flush() {
                {
                    NUtils::TSpinLockGuard guard(FramesLock);
                    Frames.emplace_back(CurrentFrame);
                }

                CurrentFrame.reset();
                SizeLen = 0;
                PayloadLen = 0;
                Stage = STAGE_START;
            }

        private:
            TBlob Buf;
            size_t Offset = 0;
            std::shared_ptr<TFrame> CurrentFrame;
            size_t SizeLen = 0;
            uint64_t PayloadLen = 0;
            std::vector<std::shared_ptr<TFrame>> Frames;
            NUtils::TSpinLock FramesLock;
            TStage Stage = STAGE_START;
        };

        TParser::TParser()
            : State(new TParserState)
        {
        }

        void TParser::Add(const size_t dataSize, char* data) {
            State->ParseData(dataSize, data);
        }

        std::vector<std::shared_ptr<TFrame>> TParser::ExtractData() {
            return State->ExtractData();
        }
    }
}
