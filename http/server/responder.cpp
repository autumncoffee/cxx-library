#include "responder.hpp"
#include "client.hpp"

namespace NAC {
    namespace NHTTPServer {
        TResponder::TResponder(std::shared_ptr<TClient> client)
            : Client(client)
        {
        }

        void TResponder::Respond(const NHTTP::TResponse& response) const {
            Client->PushWriteQueue((std::shared_ptr<NHTTPLikeParser::TParsedData>)response);
        }
    }
}
