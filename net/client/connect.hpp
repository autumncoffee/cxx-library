#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <string>
#include <memory>

namespace NAC {
    namespace NNetClient {
        bool Connect(
            const char* const host,
            const short peerPort,
            std::shared_ptr<sockaddr_in>& addr,
            socklen_t& addrLen,
            int& fh,
            const size_t maxRetries = 3
        );
    }
}
