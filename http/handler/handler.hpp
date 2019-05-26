#pragma once

#include <ac-library/http/request.hpp>
#include <ac-library/http/response.hpp>


namespace NAC {
    namespace NHTTPHandler {
        class THandler {
        public:
            virtual ~THandler();

            virtual void Handle(
                std::shared_ptr<const NHTTP::TRequest> request,
                const std::vector<std::string>& args
            ) = 0;
        };
    }
}
