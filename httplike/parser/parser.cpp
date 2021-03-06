#include "parser.hpp"
#include <ac-common/utils/string.hpp>
#include <stdexcept>
#include <math.h>
#include <iostream>

namespace NAC {
    namespace NHTTPLikeParser {
        struct TLocalParserState {
            size_t ProcessedLength = 0;
            bool IsFirst = true;
        };

        struct TParserState {
            size_t CurrentFirstLineSize = 0;
            char* CurrentFirstLine = nullptr;
            THeaders CurrentHeaders;
            TMemDisk CurrentRequest;
            bool InContent = false;
            size_t ContentLength = 0;
            size_t OriginalContentLength = 0;
            std::vector<std::shared_ptr<TParsedData>> ParsedData;
            NUtils::TSpinLock ParsedDataLock;
            bool InferContentLength = false;
            bool ChunkedEncoding = false;
            bool NoFirstLine = false;
            bool Copy = true;
            bool Stream = true;
            size_t EffectiveLength = 0;
            size_t MemMax;
            std::string DiskMask;
            TBlob Buf;

            TParserState(size_t memMax, const std::string& diskMask)
                : CurrentRequest(TMemDisk(memMax, diskMask))
                , MemMax(memMax)
                , DiskMask(diskMask)
            {
            }

            inline void AppendToCurrentRequest(const size_t size, const char* data);
            inline char CurrentRequestLastChar();
            inline void Flush(TLocalParserState& localState);
            ~TParserState();
        };

        TParsedData::~TParsedData() {
            if (FirstLine) {
                free(FirstLine);
                FirstLine = nullptr;
            }
        }

        void TParserState::AppendToCurrentRequest(const size_t size, const char* data) {
            if (Copy) {
                CurrentRequest.Append(size, data);
            }

            EffectiveLength += size;
        }

        char TParserState::CurrentRequestLastChar() {
            if (CurrentRequest) {
                return CurrentRequest[CurrentRequest.Size() - 1];

            } else {
                return 0;
            }
        }

        void TParserState::Flush(TLocalParserState& localState) {
            CurrentRequest.Finish();
            TMemDisk currentRequest(MemMax, DiskMask);
            std::swap(CurrentRequest, currentRequest);

            if (currentRequest.Size() > 0) {
                char* body = ((OriginalContentLength > 0) ? (currentRequest.Data() + (currentRequest.Size() - OriginalContentLength)) : nullptr);

                NUtils::TSpinLockGuard guard(ParsedDataLock);

                ParsedData.emplace_back(new TParsedData {
                    .FirstLineSize = CurrentFirstLineSize,
                    .FirstLine = CurrentFirstLine,
                    .Headers = std::move(CurrentHeaders),
                    .BodySize = OriginalContentLength,
                    .Body = body,
                    .Request = std::move(currentRequest)
                });

            } else if (CurrentFirstLine) {
                free(CurrentFirstLine);
            }

            InContent = false;
            ContentLength = 0;
            OriginalContentLength = 0;
            CurrentFirstLineSize = 0;
            CurrentFirstLine = nullptr;
            CurrentHeaders = THeaders();
            InferContentLength = false;
            ChunkedEncoding = false;
            NoFirstLine = false;
            Copy = true;
            Stream = true;
            EffectiveLength = 0;

            localState.IsFirst = true;
        }

        TParserState::~TParserState() {
            if (CurrentFirstLine) {
                free(CurrentFirstLine);
                CurrentFirstLine = nullptr;
            }
        }

        static inline void ParseLine(
            TLocalParserState& localState,
            TParserState& state,
            size_t lineSize,
            char* line,
            const size_t dataSize
        ) {
            localState.ProcessedLength += lineSize;

            if ((state.EffectiveLength == 0) && ((lineSize == 0) || ((lineSize == 1) && (line[0] == '\r')))) {
                if ((lineSize == 1) && (line[0] == '\r') && (localState.ProcessedLength < dataSize)) {
                    localState.ProcessedLength += 1; // '\n'
                }

                return;
            }

            if (!state.CurrentRequest && !state.Copy) {
                state.CurrentRequest.Wrap(dataSize, line, /* own = */false);
            }

            // {
            //     char line_[lineSize + 1];
            //     memcpy(line_, line, lineSize);
            //     line_[lineSize] = '\0';
            //     std::cerr << "parse: " << line_ << std::endl;
            // }

            if (state.InContent) {
                if (state.ChunkedEncoding && (state.ContentLength == 0)) {
                    if (localState.ProcessedLength >= dataSize) {
                        // std::cerr << "BUF CHUNK SIZE" << std::endl;
                        // we're in the middle of data here and need more read()'s
                        if (lineSize > 0) {
                            state.Buf.Append(lineSize, line);
                        }

                        return;
                    }

                    localState.ProcessedLength += 1; // '\n'

                    if (state.Buf.Size() > 0) {
                        state.Buf.Append(lineSize, line);
                        line = state.Buf.Data();
                        lineSize = state.Buf.Size();
                    }

                    size_t len(0);

                    for (; len < lineSize; ++len) {
                        if (line[len] == '\r') {
                            break;
                        }
                    }

                    for (size_t i = 0; i < len; ++i) {
                        static const unsigned char hex[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

                        state.ContentLength += ((size_t)hex[line[i]]) * ((size_t)powl(16, (len - i - 1)));
                    }

                    state.Buf.Shrink(0);
                    state.OriginalContentLength += state.ContentLength;

                    // std::cerr << "state.ContentLength: " << state.ContentLength << std::endl;

                    if (state.ContentLength == 0) {
                        state.Flush(localState);
                    }

                    return;
                }

                if (lineSize >= state.ContentLength) {
                    // std::cerr << "woot 1" << std::endl;
                    if (state.ContentLength > 0) {
                        state.AppendToCurrentRequest(state.ContentLength, line);

                        line += state.ContentLength;
                        lineSize -= state.ContentLength;
                        state.ContentLength = 0;
                    }

                    if (state.ChunkedEncoding) {
                        if ((lineSize > 1) || ((lineSize == 1) && (line[0] != '\r'))) {
                            throw std::logic_error("Bad request, state.OriginalContentLength: " + std::to_string(state.OriginalContentLength));
                        }

                        if (localState.ProcessedLength < dataSize) {
                            localState.ProcessedLength += 1; // '\n'
                        }

                        // std::cerr << "awaiting flush [1], state.ContentLength: " << state.ContentLength << std::endl;
                        return;

                    } else {
                        if ((lineSize == 0) && (localState.ProcessedLength < dataSize)) {
                            localState.ProcessedLength += 1; // '\n'
                        }

                        state.Flush(localState);
                    }

                } else {
                    // std::cerr << "woot 2 | " << lineSize << " | " << localState.ProcessedLength << " | " << dataSize << std::endl;
                    if (lineSize > 0) {
                        state.AppendToCurrentRequest(lineSize, line);
                        state.ContentLength -= lineSize;
                        lineSize = 0;
                    }

                    if (localState.ProcessedLength < dataSize) {
                        // std::cerr << "woot 3" << std::endl;
                        localState.ProcessedLength += 1;
                        state.AppendToCurrentRequest(1, "\n");
                        state.ContentLength -= 1;

                        if (state.ContentLength == 0) {
                            // std::cerr << "woot 4" << std::endl;

                            if (state.ChunkedEncoding) {
                                // std::cerr << "awaiting flush [2], state.ContentLength: " << state.ContentLength << std::endl;
                                return;

                            } else {
                                state.Flush(localState);
                            }
                        }
                    }
                }

                if (lineSize == 0) {
                    // std::cerr << "WOOT, state.ContentLength: " << state.ContentLength << std::endl;
                    return;
                }
            }

            if (!state.InContent) {
                const bool takeLastLine = (state.Stream && localState.IsFirst && (state.EffectiveLength > 0) && (state.CurrentRequestLastChar() != '\n'));
                bool alreadyAddedLinebreak = false;

                if (lineSize > 0) {
                    state.AppendToCurrentRequest(lineSize, line);

                    if (localState.ProcessedLength < dataSize) {
                        localState.ProcessedLength += 1;
                        state.AppendToCurrentRequest(1, "\n");
                        alreadyAddedLinebreak = true;
                    }
                }

                if (takeLastLine) {
                    char* newLine = nullptr;
                    size_t newLineSize = 0;

                    for (ssize_t i = (state.EffectiveLength - 1); i >= 0; --i) {
                        if (state.CurrentRequest[i] == '\n') {
                            newLine = state.CurrentRequest.Data() + i + 1;
                            newLineSize = state.EffectiveLength - (i + 1);
                            break;
                        }
                    }

                    if (newLine) {
                        line = newLine;
                        lineSize = newLineSize;

                    } else {
                        line = state.CurrentRequest.Data();
                        lineSize = state.EffectiveLength;
                    }
                }

                if (!alreadyAddedLinebreak && (localState.ProcessedLength < dataSize)) {
                    localState.ProcessedLength += 1;
                    state.AppendToCurrentRequest(1, "\n");
                }

                if ((lineSize == 0) || ((lineSize == 1) && (line[0] == '\r'))) {
                    std::vector<std::pair<size_t, char*>> requestArray;

                    {
                        size_t prevOffset = 0;
                        size_t i = 0;

                        for(; i < state.EffectiveLength; ++i) {
                            if (state.CurrentRequest[i] == '\n') {
                                requestArray.emplace_back(i - prevOffset, state.CurrentRequest.Data() + prevOffset);
                                prevOffset = i + 1;
                            }
                        }

                        if ((i - prevOffset) > 0) {
                            requestArray.emplace_back(i - prevOffset, state.CurrentRequest.Data() + prevOffset);
                        }
                    }

                    if (!state.NoFirstLine && (requestArray.size() > 0)) {
                        NStringUtils::Strip(
                            std::get<0>(requestArray[0]),
                            std::get<1>(requestArray[0]),
                            state.CurrentFirstLineSize,
                            state.CurrentFirstLine
                        );
                    }

                    if (requestArray.size() > (state.NoFirstLine ? 0 : 1)) {
                        bool isFirst = !state.NoFirstLine;
                        size_t prevOffset;
                        std::string key;
                        std::string value;
                        bool haveKey = false;

                        for (const auto& requestLine : requestArray) {
                            if (isFirst) {
                                isFirst = false;
                                continue;
                            }

                            const size_t size = std::get<0>(requestLine);
                            const char* data = std::get<1>(requestLine);

                            if ((size == 0) || ((size == 1) && (data[0] == '\r'))) {
                                continue;
                            }

                            prevOffset = 0;
                            haveKey = false;

                            for (size_t i = 0; i < size; ++i) {
                                if (data[i] == ':') {
                                    NStringUtils::Strip(
                                        i - prevOffset,
                                        data,
                                        key
                                    );

                                    NStringUtils::ToLower(key);

                                    prevOffset = i + 1;
                                    haveKey = true;
                                    break;
                                }
                            }

                            if (!haveKey) {
                                continue;
                            }

                            NStringUtils::Strip(
                                size - prevOffset,
                                data + prevOffset,
                                value
                            );

                            static const std::string transferEncoding("transfer-encoding");

                            if (key == transferEncoding) {
                                static const std::string chunked("chunked");

                                std::string valueLower(value);
                                NStringUtils::ToLower(valueLower);

                                if (valueLower == chunked) {
                                    state.ChunkedEncoding = true;
                                    continue;
                                }
                            }

                            state.CurrentHeaders[key].emplace_back(value);
                        }
                    }

                    state.InContent = false;

                    {
                        const auto& contentLength = state.CurrentHeaders.find("content-length");

                        if (contentLength != state.CurrentHeaders.end()) {
                            state.OriginalContentLength = state.ContentLength = atoi(contentLength->second[0].data());

                        } else if (state.InferContentLength) {
                            state.OriginalContentLength = state.ContentLength = (dataSize - localState.ProcessedLength);
                        }

                        state.InContent = ((state.OriginalContentLength > 0) || state.ChunkedEncoding);
                    }

                    // std::cerr << "InContent: " << state.InContent << ", " << state.OriginalContentLength << std::endl;

                    if (!state.InContent) {
                        state.Flush(localState);
                    }
                }
            }

            localState.IsFirst = false;
        }

        static inline void ParseData(TParserState& state, const size_t dataSize, char* data) {
            TLocalParserState localState;
            size_t i = 0;
            size_t prevOffset = 0;

            for (; i < dataSize; ++i) {
                if (data[i] == '\n') {
                    ParseLine(
                        localState,
                        state,
                        i - prevOffset,
                        data + prevOffset,
                        dataSize
                    );

                    prevOffset = (i + 1);
                }
            }

            if ((i - prevOffset) > 0) {
                ParseLine(
                    localState,
                    state,
                    i - prevOffset,
                    data + prevOffset,
                    dataSize
                );
            }

            if (dataSize != localState.ProcessedLength) {
                std::cerr << "[WARN] dataSize: " << dataSize << ", localState.ProcessedLength: " << localState.ProcessedLength << std::endl;
            }
        }

        TParser::TParser(size_t memMax, const std::string& diskMask)
            : State(new TParserState(memMax, diskMask))
        {
        }

        void TParser::Add(const size_t dataSize, char* data) {
            ParseData(*State, dataSize, data);
        }

        void TParser::Message(const size_t dataSize, char* data, bool copy) {
            State->InferContentLength = true;
            State->Copy = copy;
            State->Stream = false;

            Add(dataSize, data);
        }

        void TParser::MessageNoFirstLine(const size_t dataSize, char* data, bool copy) {
            State->NoFirstLine = true;

            Message(dataSize, data, copy);
        }

        std::vector<std::shared_ptr<TParsedData>> TParser::ExtractData() {
            std::vector<std::shared_ptr<TParsedData>> out;

            {
                NUtils::TSpinLockGuard guard(State->ParsedDataLock);

                std::swap(State->ParsedData, out);
            }

            return out;
        }
    }
}
