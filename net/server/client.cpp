#include "client.hpp"
#include <unistd.h>
#include <iostream>
#include <utils/socket.hpp>
#include <fcntl.h>

namespace NAC {
    namespace NNetServer {
        TBaseClient::TBaseClient(const TArgs* const args)
            : Args(args)
        {
        }

        void TBaseClient::WakeLoop() const {
            ::write(Args->WakeupFd, "1", 1);
        }

        void TBaseClient::SetWeakPtr(const std::shared_ptr<TBaseClient>& ptr) {
            if(!SelfWeakPtr.expired()) {
                // Utils::cluck(1, "SelfWeakPtr is already set");
                abort();
            }

            if(ptr.get() != this) {
                // Utils::cluck(1, "Tried to set SelfWeakPtr to another client");
                abort();
            }

            SelfWeakPtr = ptr;
        }

        TNetClient::TNetClient(const TArgs* const args)
            : TBaseClient(args)
        {
            NSocketUtils::SetupSocket(Args->Fh, 1000); // TODO: check return value
            fcntl(Args->Fh, F_SETFL, fcntl(Args->Fh, F_GETFL, 0) | O_NONBLOCK);

            Destroyed = false;
        }

        void TNetClient::Cb(const NMuhEv::TEvSpec& event) {
            if(event.Flags & NMuhEv::MUHEV_FLAG_ERROR) {
                std::cerr << "EV_ERROR" << std::endl;
                Drop();
                return;
            }

            if(event.Flags & NMuhEv::MUHEV_FLAG_EOF) {
                // NUtils::cluck(1, "EV_EOF");
                Drop();
                return;
            }

            if(event.Filter & NMuhEv::MUHEV_FILTER_READ) {
                while(true) {
                    static const size_t bufSize = 8192;
                    char buf[bufSize];
                    int read = recvfrom(
                        event.Ident,
                        buf,
                        bufSize,
                        MSG_DONTWAIT,
                        NULL,
                        0
                    );

                    if(read > 0) {
                        OnData(read, buf);

                    } else if(read < 0) {
                        if((errno != EAGAIN) && (errno != EINTR)) {
                            Drop();
                            return;

                        } else if(errno == EAGAIN) {
                            break;
                        }

                    } else {
                        Drop();
                        return;
                    }
                }
            }

            if(event.Filter & NMuhEv::MUHEV_FILTER_WRITE) {
                while(true) {
                    auto item = GetWriteItem();

                    if (!item) {
                        break;
                    }

                    if (item->Pos >= item->Size) {
                        PopWriteItem();
                        continue;
                    }

                    int written = ::write(event.Ident, item->Data + item->Pos, item->Size - item->Pos);

                    if(written < 0) {
                        auto e = errno;

                        if((e != EAGAIN) && (e != EINTR)) {
                            perror("write");
                            Drop(); // TODO?
                        }

                        if(e != EINTR) {
                            break;
                        }

                    } else {
                        item->Pos += written;
                    }
                }
            }
        }

        void TNetClient::PushWriteQueue(std::shared_ptr<TWriteQueueItem> data) {
            if (!data) {
                return;
            }

            {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                WriteQueue.push(data);
            }

            WakeLoop();
        }

        void TNetClient::Destroy() {
            bool _destroyed = Destroyed.exchange(true);

            if(_destroyed)
                return;

            shutdown(Args->Fh, SHUT_RDWR);
            close(Args->Fh);
        }
    }
}
