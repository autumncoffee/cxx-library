#pragma once

#include <vector>
#include <utility>
#include <ac-library/http/handler/handler.hpp>
#include <pcrecpp.h>

namespace NAC {
    namespace NHTTPRouter {
        class TRouter : public NHTTPHandler::THandler {
        public:
            virtual void Handle(
                std::shared_ptr<NHTTP::TRequest> request,
                const std::vector<std::string>& args,
                const size_t prefixLen
            );

            void Handle(
                std::shared_ptr<NHTTP::TRequest> request,
                const std::vector<std::string>& args
            ) override {
                Handle(request, args, 0);
            }

            void Add(const std::string& path, std::shared_ptr<NHTTPHandler::THandler> handler);
            void Add(const pcrecpp::RE& path, std::shared_ptr<NHTTPHandler::THandler> handler);

            void Add(const char* path, std::shared_ptr<NHTTPHandler::THandler> handler) {
                Add(std::string(path), handler);
            }

            virtual NHTTP::TResponse RouteNotFound(const NHTTP::TRequest& request) const;

        private:
            std::vector<std::pair<pcrecpp::RE, std::shared_ptr<NHTTPHandler::THandler>>> Handlers;
        };
    }
}
