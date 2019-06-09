#include "request.hpp"
#include "urlescape.hpp"
#include <utility>
#include <queue>
#include <ac-common/utils/string.hpp>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <ac-common/file.hpp>

namespace NAC {
    namespace NHTTP {
        TRequest::TRequest(
            std::shared_ptr<NHTTPLikeParser::TParsedData> data,
            const NHTTPServer::TResponder& responder
        )
            : TMessageBase(data)
            , Responder(responder)
            , ResponseSent(false)
        {
            std::queue<std::pair<std::string*, bool>> out {{
                {&Method_, true},
                {&Path_, true},
                {&Protocol_, false}
            }};

            size_t offset = 0;
            size_t i = 0;

            for (; i < data->FirstLineSize; ++i) {
                const bool isLast = (i == (data->FirstLineSize - 1));

                if ((data->FirstLine[i] == ' ') || isLast) {
                    auto [ str, shouldLowercase ] = out.front();

                    str->assign(data->FirstLine + offset, i + (isLast ? 1 : 0) - offset);

                    if (shouldLowercase) {
                        NStringUtils::ToLower(*str);
                    }

                    out.pop();

                    if (out.empty()) {
                        break;
                    }

                    offset = i + 1;
                }
            }

            i = offset = 0;
            bool inQueryParams = false;
            bool inKey = true;
            std::string key;
            size_t pathSize = Path_.size();

            for (; i < Path_.size(); ++i) {
                const char chr = Path_[i];

                if((chr == '?') && !inQueryParams) {
                    inQueryParams = true;
                    offset = i + 1;
                    pathSize = i;
                    continue;
                }

                if(!inQueryParams)
                    continue;

                const bool isLast = (i == (Path_.size() - 1));

                if(inKey) {
                    if((chr == '=') || (chr == '&') || isLast || (chr == '#')) {
                        key.assign(Path_.data() + offset, i + ((isLast && (chr != '=') && (chr != '&') && (chr != '#')) ? 1 : 0) - offset);
                        NStringUtils::ToLower(key);
                        URLUnescape(key);

                        offset = i + 1;

                        if((chr == '=') && !isLast) {
                            inKey = false;

                        } else if(key.size() > 0) {
                            QueryParams_[key].emplace_back();
                        }
                    }

                } else {
                    if((chr == '&') || isLast || (chr == '#')) {
                        const size_t size = i + ((isLast && (chr != '&') && (chr != '#')) ? 1 : 0) - offset;

                        if(size > 0) {
                            std::string value(Path_.data() + offset, size);
                            URLUnescape(value);

                            QueryParams_[key].emplace_back(std::move(value));

                        } else if(key.size() > 0) {
                            QueryParams_[key].emplace_back();
                        }

                        offset = i + 1;
                        inKey = true;
                    }
                }

                if(chr == '#')
                    break;
            }

            Path_.resize(pathSize);

            const auto& headers = Headers();
            const auto& cookies_ = headers.find("cookie");
            bool haveKey;

            if (cookies_ != headers.end()) {
                const auto& cookies = cookies_->second.front();

                i = offset = 0;
                inKey = true;
                haveKey = false;

                for (; i < cookies.size(); ++i) {
                    const bool isLast = (i == (cookies.size() - 1));

                    if ((cookies[i] == ' ') && inKey && (!haveKey)) {
                        offset = i + 1;
                        continue;
                    }

                    if(inKey)
                        haveKey = true;

                    if (cookies[i] == '=') {
                        NStringUtils::Strip(
                            i - offset,
                            cookies.data() + offset,
                            key
                        );

                        NStringUtils::ToLower(key);

                        offset = i + 1;
                        inKey = false;

                    } else if ((cookies[i] == ';') || isLast) {
                        std::string value;

                        NStringUtils::Strip(
                            i + ((isLast && (cookies[i] != ';')) ? 1 : 0) - offset,
                            cookies.data() + offset,
                            value
                        );

                        Cookies_.emplace(key, std::move(value));
                        offset = i + 1;
                        inKey = true;
                        haveKey = false;
                    }
                }
            }
        }

        bool TRequest::IsWebSocketHandshake() const {
            return (
                (HeaderValueLower("connection") == std::string("upgrade"))
                && (HeaderValueLower("upgrade") == std::string("websocket"))
                && (!SecWebSocketKey().empty())
                && (!SecWebSocketVersion().empty())
            );
        }

        std::string TRequest::GenerateSecWebSocketAccept() const {
            static const std::string magic("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

            std::string tmp(SecWebSocketKey() + magic);

            unsigned char hashed[SHA_DIGEST_LENGTH];
            SHA1((const unsigned char*)tmp.data(), tmp.size(), hashed);

            const size_t outLen(1 + (size_t)((((double)SHA_DIGEST_LENGTH / 3.0) * 4.0) + 0.5));
            unsigned char out[outLen];
            EVP_EncodeBlock(out, hashed, SHA_DIGEST_LENGTH);

            return std::string((const char*)out, outLen);
        }

        TRangeHeaderValue TRequest::Range() const {
            const auto& raw = HeaderValue("range");
            TRangeHeaderValue out;

            if (raw.empty()) {
                return out;
            }

            const size_t rawSize(raw.size());
            TBlob tmp;
            TRangeSpec tmpSpec;

            tmp.Reserve(5);

            for (size_t i = 0; i < rawSize; ++i) {
                const char chr(raw[i]);

                if ((chr != ' ') && (chr != ',') && (chr != '=') && (chr != '-')) {
                    tmp.Append(1, &chr);
                }

                if (chr == '=') {
                    if (out.Unit.empty()) {
                        out.Unit = (std::string)tmp;
                        tmp.Shrink(0);

                    } else {
                        throw std::logic_error("Invalid range header");
                    }

                    if (i == (rawSize - 1)) {
                        throw std::logic_error("Invalid range: either start or end should be present");
                    }

                } else if (chr == '-') {
                    if (tmp.Size() > 0) {
                        tmpSpec.Start = NStringUtils::FromString<size_t>(tmp);
                        tmp.Shrink(0);
                    }

                    if (i == (rawSize - 1)) {
                        if (!tmpSpec.Start) {
                            throw std::logic_error("Invalid range: either start or end should be present");
                        }

                        out.Ranges.emplace_back(std::move(tmpSpec));
                    }

                } else if ((chr == ',') || (i == (rawSize - 1))) {
                    if (tmp.Size() > 0) {
                        tmpSpec.End = NStringUtils::FromString<size_t>(tmp);
                        tmp.Shrink(0);
                    }

                    if (tmpSpec.Start) {
                        if (tmpSpec.End && (*tmpSpec.End <= *tmpSpec.Start)) {
                            throw std::logic_error("Invalid range: end should be greater than start");
                        }

                    } else if (!tmpSpec.End) {
                        throw std::logic_error("Invalid range: either start or end should be present");
                    }

                    out.Ranges.emplace_back(std::move(tmpSpec));
                    tmpSpec = decltype(tmpSpec)();
                }
            }

            return out;
        }

        TResponse TRequest::RespondFile(
            const std::string& path,
            const std::string& contentType,
            size_t size,
            size_t offset
        ) {
            const bool isHead(IsHead());
            auto file = std::make_shared<TFile>(path, (isHead ? TFile::ACCESS_INFO : TFile::ACCESS_RDONLY));

            if (!file->IsOK()) {
                return Respond404();
            }

            if (size > 0) {
                if (
                    (size > file->Size())
                    || ((offset > 0) && ((offset + size) >= file->Size()))
                ) {
                    throw std::runtime_error("Reading past the end of file");
                }
            }

            auto response = RespondFile(
                ((size == 0) ? file->Size() : size),
                (isHead ? nullptr : (file->Data() + ((size == 0) ? 0 : offset))),
                contentType
            );

            if (!isHead) {
                response.Memorize(file);
            }

            return response;
        }

        TResponse TRequest::RespondFile(
            size_t size,
            char* data,
            const std::string& contentType
        ) {
            if (IsHead()) {
                auto response = Respond200();
                response.DoNotAddContentLength();
                response.Header("Content-Length", std::to_string(size));
                response.Header("Accept-Ranges", "bytes");
                response.Header("Content-Type", contentType);

                return response;
            }

            auto range = Range();

            if (range.Ranges.empty()) {
                auto response = Respond200();
                response.Header("Content-Type", contentType);
                response.Wrap(size, data);

                return response;
            }

            const bool onlyOnePart(range.Ranges.size() == 1);
            auto response = Respond206();

            for (const auto& spec : range.Ranges) {
                char* start = nullptr;
                size_t partSize = 0;
                TMaybe<size_t> end(spec.End);
                unsigned char sizeMod = 0;

                if (end) {
                    if (spec.Start) {
                        ++(*end);
                        ++sizeMod;
                    }

                    if (*end > size) {
                        return Respond416();
                    }
                }

                if (spec.Start) {
                    if (end) {
                        partSize = *end - *spec.Start;

                    } else {
                        partSize = size - *spec.Start;
                    }

                    start = data + *spec.Start;

                } else {
                    partSize = *end;
                    start = data + size - partSize;
                }

                const size_t fromByte(start - data);
                auto part = (onlyOnePart ? std::move(response) : TResponse());

                part.Header("Content-Type", contentType);
                part.Header("Content-Range", range.Unit + " " + std::to_string(fromByte) + "-" + std::to_string(fromByte + partSize - sizeMod) + "/" + std::to_string(size));
                part.Wrap(partSize, start);

                if (onlyOnePart) {
                    return part;
                }

                response.AddPart(std::move(part));
            }

            response.Header("Content-Type", "multipart/byteranges");
            return response;
        }
    }
}
