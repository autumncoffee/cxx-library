#pragma once

#include <vector>
#include <utility>
#include <http/handler/handler.hpp>
#include <pcrecpp.h>

namespace NAC {
    namespace NHTTPRouter {
        class TRouter : public NHTTPHandler::THandler {
        public:
            virtual NHTTP::TResponse Handle(
                const NHTTP::TRequest& request,
                const std::vector<std::string>& args,
                const size_t prefixLen
            );

            NHTTP::TResponse Handle(
                const NHTTP::TRequest& request,
                const std::vector<std::string>& args
            ) override {
                return Handle(request, args, 0);
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
