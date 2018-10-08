#pragma once

#include <string>
#include <string.h>
#include <utility>
#include <memory>
#include <httplike/server/client.hpp>
#include <httplike/parser/parser.hpp>
#include <http/cookies.hpp>

namespace NAC {
    namespace NHTTP {
        class TResponse {
        public:
            TResponse() = default;
            TResponse(const TResponse&) = delete;
            TResponse(TResponse&&) = default;

            TResponse& FirstLine(const std::string& data) {
                FirstLine_ = data;
                return *this;
            }

            TResponse& FirstLine(std::string&& data) {
                FirstLine_ = std::move(data);
                return *this;
            }

            TResponse& Header(const std::string& key, const std::string& value) {
                Headers_[key].emplace_back(value);
                return *this;
            }

            TResponse& Header(const std::string& key, std::string&& value) {
                Headers_[key].emplace_back(std::move(value));
                return *this;
            }

            TResponse& Header(std::string&& key, std::string&& value) {
                Headers_[std::move(key)].emplace_back(std::move(value));
                return *this;
            }

            TResponse& SetCookie(const TSetCookieSpec& spec) {
                Header("Set-Cookie", spec.Dump());

                return *this;
            }

            template<typename... TArgs>
            TResponse& Reserve(TArgs&&... args) {
                Body.Reserve(std::forward<TArgs&&>(args)...);
                return *this;
            }

            template<typename... TArgs>
            TResponse& Write(TArgs&&... args) {
                Body.Append(std::forward<TArgs&&>(args)...);
                return *this;
            }

            const std::string& FirstLine() const {
                return FirstLine_;
            }

            const NHTTPLikeParser::THeaders& Headers() const {
                return Headers_;
            }

            size_t ContentLength() const {
                return Body.Size();
            }

            const char* Content() const {
                return Body.Data();
            }

            explicit operator std::shared_ptr<NHTTPLikeParser::TParsedData>() const;

        private:
            std::string FirstLine_;
            NHTTPLikeParser::THeaders Headers_;
            TBlob Body;
        };
    }
}
