#include "request.hpp"
#include "urlescape.hpp"
#include <algorithm>
#include <utility>
#include <queue>
#include <utils/string.hpp>
#include <openssl/sha.h>
#include <openssl/evp.h>
// #include <iostream>

namespace {
    template<typename TContentTypeParams>
    void ParseContentType(
        const NAC::NHTTP::THeaders& headers,
        const std::string& headersKey,
        std::string& contentTypeDest,
        TContentTypeParams& contentTypeParamsDest
    ) {
        const auto& contentType_ = headers.find(headersKey);

        if (contentType_ != headers.end()) {
            const auto& contentType = contentType_->second.front();

            size_t i = 0;
            size_t offset = 0;
            bool inKey = true;
            bool haveKey = false;
            bool inQueryParams = false;
            std::string key;

            for (; i < contentType.size(); ++i) {
                const char chr = contentType[i];
                const bool isLast = (i == (contentType.size() - 1));

                if(!inQueryParams && (isLast || (chr == ';'))) {
                    NAC::NStringUtils::Strip(
                        (i + (isLast ? 1 : 0) - offset),
                        contentType.data() + offset,
                        contentTypeDest
                    );

                    std::transform(contentTypeDest.begin(), contentTypeDest.end(), contentTypeDest.begin(), tolower);

                    inQueryParams = true;
                    offset = i + 1;
                    continue;
                }

                if(!inQueryParams)
                    continue;

                if ((chr == ' ') && inKey && (!haveKey)) {
                    offset = i + 1;
                }

                if(inKey)
                    haveKey = true;

                if (chr == '=') {
                    NAC::NStringUtils::Strip(
                        i - offset,
                        contentType.data() + offset,
                        key
                    );

                    std::transform(key.begin(), key.end(), key.begin(), tolower);

                    offset = i + 1;
                    inKey = false;

                } else if ((chr == ';') || isLast) {
                    std::string value;

                    NAC::NStringUtils::Strip(
                        i + (isLast ? 1 : 0) - offset,
                        contentType.data() + offset,
                        value
                    );

                    contentTypeParamsDest.emplace(key, std::move(value));
                    offset = i + 1;
                    inKey = true;
                    haveKey = false;
                }
            }
        }
    }
}

namespace NAC {
    namespace NHTTP {
        TBodyPart::TBodyPart(std::shared_ptr<NHTTPLikeParser::TParsedData> data)
            : Data(data)
        {
            ParseContentType(Headers(), "content-type", ContentType_, ContentTypeParams_);
            ParseContentType(Headers(), "content-disposition", ContentDisposition_, ContentDispositionParams_);
        }

        TRequest::TRequest(
            std::shared_ptr<NHTTPLikeParser::TParsedData> data,
            const NHTTPServer::TResponder& responder
        )
            : Data(data)
            , Responder(responder)
            , ResponseSent(new std::atomic<bool>(false))
        {
            std::queue<std::pair<std::string*, bool>> out {{
                {&Method_, true},
                {&Path_, true},
                {&Protocol_, false}
            }};

            size_t offset = 0;
            size_t i = 0;

            for (; i < Data->FirstLineSize; ++i) {
                const bool isLast = (i == (Data->FirstLineSize - 1));

                if ((Data->FirstLine[i] == ' ') || isLast) {
                    auto&& spec = out.front();
                    auto* str = std::get<0>(spec);

                    str->assign(Data->FirstLine + offset, i + (isLast ? 1 : 0) - offset);

                    if(std::get<1>(spec))
                        std::transform(str->begin(), str->end(), str->begin(), tolower);

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
                        std::transform(key.begin(), key.end(), key.begin(), tolower);
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
                        NAC::NStringUtils::Strip(
                            i - offset,
                            cookies.data() + offset,
                            key
                        );

                        std::transform(key.begin(), key.end(), key.begin(), tolower);

                        offset = i + 1;
                        inKey = false;

                    } else if ((cookies[i] == ';') || isLast) {
                        std::string value;

                        NAC::NStringUtils::Strip(
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

            ParseContentType(headers, "content-type", ContentType_, ContentTypeParams_);

            ParseParts();
        }

        void TRequest::ParseParts() {
            // std::cerr << std::endl << std::endl << "------------" << std::endl << std::endl << std::endl;
            const auto& contentType = ContentType();

            if (strncmp(contentType.data(), "multipart/", 10) != 0) {
                return;
            }

            const auto& boundary_ = ContentTypeParams_.find("boundary");

            if (boundary_ == ContentTypeParams_.end()) {
                return;
            }

            std::string boundary;
            boundary.reserve(boundary_->second.size() + 2);
            boundary += "--";
            boundary += boundary_->second;

            const char* pos = Content();
            const char* end = pos + ContentLength();
            const char* nextPos;
            bool isFirst = true;

            NHTTPLikeParser::TParser parser;

            while ((pos = std::search(pos, end, boundary.begin(), boundary.end())) != end) {
                if(isFirst) {
                    isFirst = false;
                    boundary = std::string("\r\n") + boundary;
                }

                pos += boundary.size();

                nextPos = std::search(pos, end, boundary.begin(), boundary.end());

                if(nextPos == end) {
                    break;
                }

                parser.MessageNoFirstLine((uintptr_t)nextPos - (uintptr_t)pos, (char*)pos);
                pos = nextPos;
            }

            const auto& parts = parser.ExtractData();

            for(const auto& part : parts) {
                Parts_.emplace_back(part);
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
    }
}
