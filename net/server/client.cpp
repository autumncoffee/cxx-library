#include "client.hpp"
#include <unistd.h>
#include <iostream>
#include <ac-common/utils/socket.hpp>
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

            if (args->UseSSL && args->SSLCtx) {
                SSL_ = SSL_new(args->SSLCtx);

                if (!SSL_) {
                    std::cerr << "SSL_new() failed" << std::endl;
                    abort();
                }

                if (SSL_set_fd(SSL_, Args->Fh) != 1) {
                    std::cerr << "SSL_set_fd() failed" << std::endl;
                    abort();
                }

                if (args->SSLIsClient) {
                    if (SSL_set_cipher_list(SSL_, "ALL") != 1) {
                        std::cerr << "SSL_set_cipher_list() failed" << std::endl;
                        abort();
                    }

                    SSL_set_connect_state(SSL_);

                } else {
                    SSL_set_accept_state(SSL_);
                }
            }

            Destroyed = false;
        }

        void TNetClient::Cb(const NMuhEv::TEvSpec& event) {
            assert(event.Ident == Args->Fh);

            if (event.Flags & NMuhEv::MUHEV_FLAG_ERROR) {
                std::cerr << "EV_ERROR" << std::endl;
                Drop();
                return;
            }

            if (event.Flags & NMuhEv::MUHEV_FLAG_EOF) {
                // NUtils::cluck(1, "EV_EOF");
                Drop();
                return;
            }

            if (event.Filter & NMuhEv::MUHEV_FILTER_READ) {
                while (true) {
                    static const size_t bufSize = 8192;
                    char buf[bufSize];
                    int read = ReadFromSocket(event.Ident, buf, bufSize);

                    if (read > 0) {
                        OnData(read, buf);

                    } else if (read < 0) {
                        if ((errno != EAGAIN) && (errno != EINTR)) {
                            Drop();
                            return;

                        } else if (errno == EAGAIN) {
                            break;
                        }

                    } else {
                        Drop();
                        return;
                    }
                }
            }

            if (event.Filter & NMuhEv::MUHEV_FILTER_WRITE) {
                while (true) {
                    auto item = GetWriteItem();

                    if (!item) {
                        break;
                    }

                    const int written = Write(*item, event.Ident);

                    if (written < 0) {
                        auto e = errno;

                        if ((e != EAGAIN) && (e != EINTR)) {
                            perror("write");
                            Drop(); // TODO?
                        }

                        if (e != EINTR) {
                            break;
                        }

                    } else if (item->Dummy || (item->Pos >= item->Size)) {
                        PopWriteItem();
                    }
                }
            }
        }

        int TNetClient::ReadFromSocket(
            const int fh,
            void* buf,
            const size_t bufSize
        ) {
            if (SSL_) {
                int rv = SSL_read(SSL_, buf, bufSize);

                if (rv < 0) {
                    auto err = SSL_get_error(SSL_, rv);

                    switch (err) {
                        case SSL_ERROR_WANT_WRITE:
                            UnshiftDummyWriteQueueItem();

                        case SSL_ERROR_WANT_READ:
                            errno = EAGAIN;
                            break;

                        default:
                            errno = ECONNRESET;
                            break;
                    }
                }

                return rv;

            } else {
                return ::recvfrom(fh, buf, bufSize, MSG_DONTWAIT, NULL, 0);
            }
        }

        int TNetClient::WriteToSocket(
            const int fh,
            const void* buf,
            const size_t bufSize
        ) {
            if (SSL_) {
                int rv;

                if (fh == -1) {
                    rv = SSL_do_handshake(SSL_);

                } else {
                    rv = SSL_write(SSL_, buf, bufSize);
                }

                if (rv < 0) {
                    auto err = SSL_get_error(SSL_, rv);

                    switch (err) {
                        case SSL_ERROR_WANT_WRITE: {
                            if (fh == -1) {
                                UnshiftDummyWriteQueueItem();
                            }
                        }

                        case SSL_ERROR_WANT_READ:
                            errno = EAGAIN;
                            break;

                        default:
                            errno = ECONNRESET;
                            break;
                    }
                }

                return rv;

            } else {
                return ::write(fh, buf, bufSize);
            }
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

        void TNetClient::TWriteQueueItem::CalcSize() {
            Size = 0;

            for (const auto& node : Sequence) {
                Size += node.Len;
            }
        }

        int TNetClient::Write(TWriteQueueItem& item, const int fh) {
            if (item.Dummy) {
                return WriteToSocket(-1, nullptr, 0);
            }

            if (item.Size < 0) {
                item.CalcSize();
            }

            int written = 0;

            while (item.Pos < item.Size) {
                size_t next = 0;
                size_t len = 0;

                const char* str = item.Sequence.Read(item.DataLast, item.DataPos, &len, &next);
                written = WriteToSocket(fh, str, len);

                if (written < 0) {
                    break;

                } else {
                    if (next != item.DataLast) {
                        item.DataLast = next;
                        item.DataPos = 0;
                    }

                    item.Pos += written;
                    item.DataPos += written;
                }
            }

            return written;
        }
    }
}
