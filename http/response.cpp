#include "response.hpp"
#include <utility>
#include <ac-library/http/utils/headers.hpp>
#include <ac-library/http/utils/multipart.hpp>

namespace NAC {
    namespace NHTTP {
        TBlob TResponse::Preamble() const {
            return Preamble(Headers_);
        }

        TBlob TResponse::Preamble(const NHTTPLikeParser::THeaders& headers) const {
            TBlob out;

            if (!FirstLine_.empty()) {
                out.Append(FirstLine_.size(), FirstLine_.data());
            }

            for (const auto& header : headers) {
                for (const auto& value : header.second) {
                    out << header.first << ": " << value << "\r\n";
                }
            }

            return out;
        }

        TResponse::operator TBlobSequence() const {
            if (Parts_.empty()) {
                return DumpSimple();

            } else {
                return DumpMultipart();
            }
        }

        const TResponse* TResponse::PartByName(const std::string &name) const {
            for (auto &&part : Parts()) {
                std::string ContentDisposition;
                NHTTP::THeaderParams ContentDispositionParams;
                NHTTPUtils::ParseHeader(part.Headers(), "content-disposition",
                                        ContentDisposition, ContentDispositionParams);
                for (auto[key, value]: ContentDispositionParams) {
                    if (key == std::string("filename") && value == std::string("\"") + name + std::string("\"")) {
                        return &part;
                    }
                }
            }
            return nullptr;
        }

        TBlobSequence TResponse::DumpSimple() const {
            auto preamble = Preamble();

            if (AddContentLength_) {
                preamble
                    << "Content-Length: "
                    << std::to_string(ContentLength())
                    << "\r\n"
                ;
            }

            preamble << "\r\n";

            TBlobSequence out;
            out.Concat(std::move(preamble));

            if (ContentLength() > 0) {
                out.Concat(ContentLength(), Content());
            }

            out.MemorizeCopy(this);

            return out;
        }

        TBlobSequence TResponse::DumpMultipart() const {
            decltype(Headers_) headers(Headers_);

            std::string ctype;
            THeaderParams params;
            NHTTPUtils::ParseHeader(headers, "content-type", ctype, params);
            std::string boundary;

            {
                const auto& boundary_ = params.find("boundary");

                if (boundary_ == params.end()) {
                    params["boundary"] = boundary = NHTTPUtils::Boundary();

                    for (const auto& it : params) {
                        ctype += ";" + it.first + "=" + it.second;
                    }

                    headers["content-type"] = std::vector({ctype});

                } else {
                    boundary = boundary_->second;
                }
            }

            boundary = "--" + boundary;

            TBlob end;
            end.Reserve(boundary.size() + 6);
            end << "\r\n" << boundary << "--\r\n";

            bool isFirst(true);
            TBlobSequence body;
            size_t contentLength = end.Size();

            for (const auto& node : Parts_) {
                auto part = (TBlobSequence)(node);

                for (const auto& it : part) {
                    contentLength += it.Len;
                }

                TBlob tmp;
                tmp.Reserve(boundary.size() + (isFirst ? 2 : 4));
                tmp << (isFirst ? "" : "\r\n") << boundary << "\r\n";

                contentLength += tmp.Size();

                body.Concat(std::move(tmp));
                body.Concat(std::move(part));

                isFirst = false;
            }

            TBlobSequence out;

            {
                auto preamble = Preamble(headers);

                if (AddContentLength_) {
                    preamble
                        << "Content-Length: "
                        << std::to_string(contentLength)
                        << "\r\n"
                    ;
                }

                preamble << "\r\n";

                out.Concat(std::move(preamble));
            }

            out.Concat(std::move(body));
            out.Concat(std::move(end));
            out.MemorizeCopy(this);

            return out;
        }
    }
}
