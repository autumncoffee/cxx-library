#include "connect.hpp"
#include <iostream>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <utils/socket.hpp>
#include <fcntl.h>
#include <unistd.h>

namespace {
    bool ConnectImpl(
        const char* const host,
        const short peerPort,
        std::shared_ptr<sockaddr_in>& addr,
        socklen_t& addrLen,
        int& fh
    ) {
        addrinfo hints;
        addrinfo* serviceInfo;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_NUMERICSERV;

        char port[6];
        sprintf(port, "%d", peerPort);
        int rv;

        if((rv = getaddrinfo(host, port, &hints, &serviceInfo)) != 0) {
            std::cerr << "getaddrinfo(" << host << ", " << peerPort << "): " << gai_strerror(rv) << std::endl;
            return false;
        }

        addr.reset(new sockaddr_in);
        bool connected = false;

        for(addrinfo* aiIt = serviceInfo; aiIt != nullptr; aiIt = aiIt->ai_next) {
            if((fh = socket(aiIt->ai_family, aiIt->ai_socktype, aiIt->ai_protocol)) == -1) {
                perror("socket");
                continue;
            }

            // TODO: discovery should be async too
            if(!NAC::NSocketUtils::SetupSocket(fh, 3000)) {
                close(fh);
                continue;
            }

            if(connect(fh, aiIt->ai_addr, aiIt->ai_addrlen) == -1) {
                if(errno != EINPROGRESS) {
                    std::string str ("connect(");
                    str += host;
                    str += ":";
                    str += port;
                    str += ")";

                    perror(str.c_str());

                    close(fh);
                    continue;
                }

                fcntl(fh, F_SETFL, fcntl(fh, F_GETFL, 0) | O_NONBLOCK);
            }

            *addr = *(sockaddr_in*)(aiIt->ai_addr);
            addrLen = aiIt->ai_addrlen;
            connected = true;

            break;
        }

        freeaddrinfo(serviceInfo);

        struct sockaddr_in addr_;
        socklen_t addrLen_ = sizeof(addr_);

        if(connected && (getpeername(fh, (sockaddr*)&addr_, &addrLen_) == -1)) {
            perror("getpeername");
            close(fh);
            // free(addr);
            connected = false;
        }

        return connected;
    }
}

namespace NAC {
    namespace NNetClient {
        bool Connect(
            const char* const host,
            const short peerPort,
            std::shared_ptr<sockaddr_in>& addr,
            socklen_t& addrLen,
            int& fh,
            const size_t maxRetries
        ) {
            for (size_t i = 0; i < maxRetries; ++i) {
                if (ConnectImpl(host, peerPort, addr, addrLen, fh)) {
                    return true;
                }
            }

            return false;
        }
    }
}
