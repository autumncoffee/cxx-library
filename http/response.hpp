#pragma once

#include <string>
#include <string.h>
#include <utility>
#include <memory>
#include <ac-library/httplike/server/client.hpp>
#include <ac-library/httplike/parser/parser.hpp>
#include <ac-library/http/cookies.hpp>
#include <ac-common/str.hpp>
#include <ac-common/string_sequence.hpp>
#include <ac-common/tmpmem.hpp>
#include <vector>
#include <ac-common/utils/string.hpp>

namespace NAC {
    namespace NHTTP {
        class TResponse : public TWithTmpMem {
        public:
            TResponse()
                : Body(new TBlob)
            {
                Memorize(Body);
            }

            TResponse(const TResponse&) = delete;
            TResponse(TResponse&&) = default;

            void AddPart(TResponse&& part) {
                Parts.emplace_back(std::move(part));
            }

            TResponse& FirstLine(const std::string& data) {
                FirstLine_ = data;
                return *this;
            }

            TResponse& FirstLine(std::string&& data) {
                FirstLine_ = std::move(data);
                return *this;
            }

            TResponse& Header(const std::string& key, const std::string& value) {
                Headers_[NStringUtils::ToLower(key)].emplace_back(value);
                return *this;
            }

            TResponse& Header(const std::string& key, std::string&& value) {
                Headers_[NStringUtils::ToLower(key)].emplace_back(std::move(value));
                return *this;
            }

            TResponse& Header(std::string&& key, std::string&& value) {
                Headers_[NStringUtils::ToLower(std::move(key))].emplace_back(std::move(value));
                return *this;
            }

            TResponse& SetCookie(const TSetCookieSpec& spec) {
                Header("Set-Cookie", spec.Dump());

                return *this;
            }

            template<typename... TArgs>
            TResponse& Reserve(TArgs&&... args) {
                Body->Reserve(std::forward<TArgs>(args)...);
                return *this;
            }

            template<typename... TArgs>
            TResponse& Write(TArgs&&... args) {
                Body->Append(std::forward<TArgs>(args)...);
                return *this;
            }

            template<typename... TArgs>
            TResponse& Wrap(TArgs&&... args) {
                Body->Wrap(std::forward<TArgs>(args)...);
                return *this;
            }

            const std::string& FirstLine() const {
                return FirstLine_;
            }

            const NHTTPLikeParser::THeaders& Headers() const {
                return Headers_;
            }

            size_t ContentLength() const {
                return Body->Size();
            }

            const char* Content() const {
                return Body->Data();
            }

            TBlob Preamble() const;
            explicit operator TBlobSequence() const;

            void DoNotAddContentLength() {
                AddContentLength_ = false;
            }

        private:
            TBlobSequence DumpSimple() const;
            TBlobSequence DumpMultipart() const;
            TBlob Preamble(const NHTTPLikeParser::THeaders&) const;

        private:
            std::string FirstLine_;
            NHTTPLikeParser::THeaders Headers_;
            std::shared_ptr<TBlob> Body;
            bool AddContentLength_ = true;
            std::vector<TResponse> Parts;
        };
    }
}
