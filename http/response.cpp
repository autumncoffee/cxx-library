#include "response.hpp"


namespace NAC {
    namespace NHTTP {
        TResponse::operator std::shared_ptr<NHTTPLikeParser::TParsedData>() const {
            if (FirstLine_.size() == 0) {
                throw std::logic_error("Reponse first line is empty");
            }

            std::shared_ptr<NHTTPLikeParser::TParsedData> out;
            out.reset(new NHTTPLikeParser::TParsedData);
            auto&& response = *out;

            response.Append(FirstLine_.size(), FirstLine_.data());

            for (const auto& header : Headers_) {
                for (const auto& value : header.second) {
                    response << header.first << ": " << value << "\r\n";
                }
            }

            response
                << "Content-Length: "
                << std::to_string(ContentLength())
                << "\r\n\r\n"
            ;

            if (ContentLength() > 0) {
                response.Append(ContentLength(), Content());
            }

            return out;
        }
    }
}
