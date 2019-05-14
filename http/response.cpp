#include "response.hpp"
#include <utility>

namespace NAC {
    namespace NHTTP {
        TBlob TResponse::Preamble() const {
            if (FirstLine_.empty()) {
                throw std::logic_error("Reponse first line is empty");
            }

            TBlob out;
            out.Append(FirstLine_.size(), FirstLine_.data());

            for (const auto& header : Headers_) {
                for (const auto& value : header.second) {
                    out << header.first << ": " << value << "\r\n";
                }
            }

            return out;
        }

        TResponse::operator TBlobSequence() const {
            auto preamble = Preamble();

            preamble
                << "Content-Length: "
                << std::to_string(ContentLength())
                << "\r\n\r\n"
            ;

            TBlobSequence out;
            out.Concat(std::move(preamble));

            if (ContentLength() > 0) {
                out.Concat(ContentLength(), Content());
            }

            out.MemorizeCopy(this);

            return out;
        }
    }
}
