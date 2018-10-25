#include "client.hpp"
#include "responder.hpp"
#include <iostream>

namespace NAC {
    namespace NHTTPServer {
        void TClient::OnData(std::shared_ptr<NHTTPLikeParser::TParsedData> request) {
            try {
                HandleRequest(request);

            } catch(...) {
                std::cerr << "[disaster]" << std::endl;
                Drop();
            }
        }

        void TClient::HandleRequest(std::shared_ptr<NHTTPLikeParser::TParsedData> data) {
            TResponder responder(GetNewSharedPtr<TClient>());

            try {
                try {
                    NHTTP::TRequest request(data, responder);

                    try {
                        HandleRequestImpl(request);

                        if (!request.IsResponseSent()) {
                            request.Send(InternalServerError());
                        }

                    } catch(const std::exception& e) {
                        HandleException(request, e);
                    }

                } catch(const std::exception& e) {
                    responder.Respond(InternalServerError(e));
                }

            } catch(...) {
                responder.Respond(InternalServerError());
            }
        }

        void TClient::HandleRequestImpl(const NHTTP::TRequest& request) {
            std::cerr
                << "[access] "
                << request.FirstLine()
                << std::endl
            ;

            ((TArgs*)Args.get())->Handler.Handle(
                request,
                std::vector<std::string>()
            );
        }

        void TClient::HandleException(const NHTTP::TRequest& request, const std::exception& e) {
            std::cerr
                << "[error] "
                << request.FirstLine()
                << ": "
                << e.what()
                << std::endl
            ;

            request.Send(InternalServerErrorResponse());
        }

        NHTTP::TResponse TClient::InternalServerError(const std::exception& e) const {
            std::cerr << "[unhandled exception] " << e.what() << std::endl;

            return InternalServerErrorResponse();
        }

        NHTTP::TResponse TClient::InternalServerError() const {
            std::cerr << "[unknown exception] " << std::endl;

            return InternalServerErrorResponse();
        }

        NHTTP::TResponse TClient::InternalServerErrorResponse() const {
            NHTTP::TResponse response;

            response.FirstLine("HTTP/1.1 500 Internal Server Error\r\n");

            return response;
        }
    }
}
