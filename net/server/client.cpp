#include "client.hpp"
#include <unistd.h>
#include <iostream>
#include <utils/socket.hpp>
#include <fcntl.h>
#include <assert.h>

namespace NAC {
    namespace NNetServer {
        TBaseClient::TBaseClient(TArgs* const args)
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

        TNetClient::TNetClient(TArgs* const args)
            : TBaseClient(args)
        {
            NSocketUtils::SetupSocket(Args->Fh, 1000); // TODO: check return value
            fcntl(Args->Fh, F_SETFL, fcntl(Args->Fh, F_GETFL, 0) | O_NONBLOCK);

            Destroyed = false;
        }

        void TNetClient::Cb(const NMuhEv::TEvSpec& event) {
            assert(event.Ident == Args->Fh);

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
                    int read = ReadFromSocket(event.Ident, buf, bufSize);

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

                    int written = 0;

                    if (item->Dummy) {
                        written = WriteToSocket(-1, nullptr, 0);

                    } else {
                        written = WriteToSocket(event.Ident, item->Data + item->Pos, item->Size - item->Pos);
                    }

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
                        if (item->Dummy) {
                            PopWriteItem();

                        } else {
                            item->Pos += written;

                            if (item->Pos >= item->Size) {
                                PopWriteItem();
                            }
                        }
                    }
                }
            }
        }

        int TNetClient::ReadFromSocket(
            const int fh,
            void* buf,
            const size_t bufSize
        ) {
            return ::recvfrom(fh, buf, bufSize, MSG_DONTWAIT, NULL, 0);
        }

        int TNetClient::WriteToSocket(
            const int fh,
            const void* buf,
            const size_t bufSize
        ) {
            return ::write(fh, buf, bufSize);
        }

        void TNetClient::PushWriteQueue(std::shared_ptr<TWriteQueueItem> data) {
            if (!data) {
                return;
            }

            {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                WriteQueue.emplace_back(data);
            }

            WakeLoop();
        }

        void TNetClient::UnshiftDummyWriteQueueItem() {
            auto data = std::make_shared<TWriteQueueItem>();
            data->Dummy = true;

            {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                WriteQueue.emplace_front(data);
            }

            WakeLoop();
        }

        void TNetClient::Destroy() {
            bool _destroyed = Destroyed.exchange(true);

            WakeLoop();

            if(_destroyed)
                return;

            shutdown(Args->Fh, SHUT_RDWR);
            close(Args->Fh);
        }
    }
}
