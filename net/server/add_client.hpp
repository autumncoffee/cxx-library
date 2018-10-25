#pragma once

#include <functional>
#include <memory>

namespace NAC {
    namespace NNetServer {
        class TBaseClient;

        using TAddClient = std::function<void(std::shared_ptr<TBaseClient>)>;
    }
}
