#include "server.hpp"
#include <iostream>

namespace NAC {
    namespace NHTTPServer {
        TServer::TServer(
            const TArgs& args,
            NHTTPHandler::THandler& handler
        )
            : Handler(handler)
            , Server(args, [this](std::shared_ptr<NHTTPLikeParser::TParsedData> data) {
                try {
                    return (std::shared_ptr<NHTTPLikeParser::TParsedData>)HandleRequest(data);

                } catch(...) {
                    std::cerr << "[disaster]" << std::endl;
                    return std::shared_ptr<NHTTPLikeParser::TParsedData>();
                }
            })
        {
        }

        NHTTP::TResponse TServer::HandleRequest(std::shared_ptr<NHTTPLikeParser::TParsedData> data) {
            try {
                try {
                    NHTTP::TRequest request(data);

                    try {
                        return HandleRequestImpl(request);

                    } catch(const std::exception& e) {
                        return HandleException(request, e);
                    }

                } catch(const std::exception& e) {
                    return InternalServerError(e);
                }

            } catch(...) {
                return InternalServerError();
            }
        }

        NHTTP::TResponse TServer::HandleRequestImpl(const NHTTP::TRequest& request) {
            std::cerr
                << "[access] "
                << request.FirstLine()
                << std::endl
            ;

            return Handler.Handle(
                request,
                std::vector<std::string>()
            );
        }

        NHTTP::TResponse TServer::HandleException(const NHTTP::TRequest& request, const std::exception& e) {
            std::cerr
                << "[error] "
                << request.FirstLine()
                << ": "
                << e.what()
                << std::endl
            ;

            return InternalServerErrorResponse();
        }

        NHTTP::TResponse TServer::InternalServerError(const std::exception& e) const {
            std::cerr << "[unhandled exception] " << e.what() << std::endl;

            return InternalServerErrorResponse();
        }

        NHTTP::TResponse TServer::InternalServerError() const {
            std::cerr << "[unknown exception] " << std::endl;

            return InternalServerErrorResponse();
        }

        NHTTP::TResponse TServer::InternalServerErrorResponse() const {
            NHTTP::TResponse response;

            response.FirstLine("HTTP/1.1 500 Internal Server Error\r\n");

            return response;
        }
    }
}
