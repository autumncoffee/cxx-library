#include "router.hpp"

#define CAPTURE_GROUP_COUNT 10

namespace NAC {
    namespace NHTTPRouter {
        void TRouter::Handle(
            std::shared_ptr<NHTTP::TRequest> request,
            const std::vector<std::string>& inputArgs,
            const size_t prefixLen
        ) {
            pcrecpp::StringPiece path(request->Path());

            if(prefixLen > 0)
                path.remove_prefix(prefixLen);

            std::string matches[CAPTURE_GROUP_COUNT];
            pcrecpp::Arg args_[CAPTURE_GROUP_COUNT];
            const pcrecpp::Arg* args[CAPTURE_GROUP_COUNT];

            for(size_t i = 0; i < CAPTURE_GROUP_COUNT; ++i) {
                args_[i] = &matches[i];
                args[i] = &args_[i];
            }

            int consumed;

            for (const auto& spec : Handlers) {
                const auto& re = std::get<0>(spec);
                const int n = re.NumberOfCapturingGroups();

                if(re.DoMatch(path, pcrecpp::RE::UNANCHORED, &consumed, args, n)) {
                    std::vector<std::string> handlerArgs(inputArgs);
                    handlerArgs.reserve(handlerArgs.size() + n);

                    for (int i = 0; i < n; ++i)
                        handlerArgs.emplace_back(matches[i]);

                    auto handler = std::get<1>(spec);

                    if(auto* router = dynamic_cast<TRouter*>(handler.get())) {
                        router->Handle(request, handlerArgs, prefixLen + consumed);

                    } else {
                        handler->Handle(request, handlerArgs);
                    }

                    return;
                }
            }

            request->Send(RouteNotFound(*request));
        }

        void TRouter::Add(const std::string& path, std::shared_ptr<NHTTPHandler::THandler> handler) {
            pcrecpp::RE_Options opt;
            opt.set_caseless(true);

            pcrecpp::RE re(path, opt);

            Add(re, handler);
        }

        void TRouter::Add(const pcrecpp::RE& path, std::shared_ptr<NHTTPHandler::THandler> handler) {
            const int n = path.NumberOfCapturingGroups();

            if(n < 0)
                throw std::runtime_error("Bad route");

            else if(n > CAPTURE_GROUP_COUNT)
                throw std::runtime_error("Too many capture groups");

            Handlers.emplace_back(path, handler);
        }

        NHTTP::TResponse TRouter::RouteNotFound(const NHTTP::TRequest& request) const {
            NHTTP::TResponse out;
            out.FirstLine(request.Protocol() + " 404 Not Found\r\n");

            return out;
        }
    }
}
