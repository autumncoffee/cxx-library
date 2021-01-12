#pragma once

// wqcb == Write Queue CallBack

#include <functional>
#include <memory>

namespace NAC {
    namespace NNetServer {
        class TBaseClient;

        using TWQCB = std::function<void(std::shared_ptr<TBaseClient>)>;
    }
}
