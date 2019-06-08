#include "message.hpp"

namespace NAC {
    namespace NHTTP {
        TMessageBase::TMessageBase(std::shared_ptr<NHTTPLikeParser::TParsedData> data)
            : Data(data)
        {
            NHTTPUtils::ParseHeader(Headers(), "content-type", ContentType_, ContentTypeParams_);

            ParseParts();
        }

        void TMessageBase::ParseParts() {
            const auto& contentType = ContentType();

            if (strncmp(contentType.data(), "multipart/", 10) != 0) {
                return;
            }

            const auto& boundary_ = ContentTypeParams_.find("boundary");

            if (boundary_ == ContentTypeParams_.end()) {
                return;
            }

            TWithBodyParts::ParseParts(boundary_->second, ContentLength(), Content());
        }
    }
}
