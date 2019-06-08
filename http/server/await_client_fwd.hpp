#pragma once

#include <functional>
#include <memory>

namespace NAC {
    namespace NHTTP {
        class TIncomingResponse;
        class TRequest;
    }

    namespace NWebSocketParser {
        class TFrame;
    }

    namespace NHTTPServer {
        class TClientBase;

        template<typename TBase = TClientBase>
        class TAwaitClient;

        template<typename TBase = TClientBase>
        using TAwaitClientCallback = std::function<void(
            std::shared_ptr<NHTTP::TIncomingResponse>,
            std::shared_ptr<TBase>
        )>;

        template<typename TBase = TClientBase>
        using TAwaitClientWSCallback = std::function<void(
            std::shared_ptr<NWebSocketParser::TFrame>,
            std::shared_ptr<NHTTP::TRequest>,
            std::shared_ptr<TBase>
        )>;
    }
}
