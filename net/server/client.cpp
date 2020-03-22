#include "client.hpp"
#include <unistd.h>
#include <iostream>
#include <ac-common/utils/socket.hpp>
#include <fcntl.h>

namespace NAC {
    namespace NNetServer {
        TBaseClient::TBaseClient(TArgs* const args)
            : NMuhEv::TNode(-1)
            , Args(args)
        {
        }

        void TBaseClient::WakeLoop() const {
            Loop().Wake();
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
            if (!args->Loop) {
                std::cerr << "Missing event loop" << std::endl;
                abort();
            }

            if (args->Fh == -1) {
                std::cerr << "Missing socket" << std::endl;
                abort();
            }

            NSocketUtils::SetupSocket(args->Fh, 1000); // TODO: check return value
            fcntl(args->Fh, F_SETFL, fcntl(args->Fh, F_GETFL, 0) | O_NONBLOCK);

            EvIdent = args->Fh;
            EvFilter = NMuhEv::MUHEV_FILTER_READ;
            EvFlags = NMuhEv::MUHEV_FLAG_NONE;

            Loop().AddEvent(*this, /* mod = */false);

            if (args->UseSSL && args->SSLCtx) {
                SSL_ = SSL_new(args->SSLCtx);

                if (!SSL_) {
                    std::cerr << "SSL_new() failed" << std::endl;
                    abort();
                }

                if (SSL_set_fd(SSL_, args->Fh) != 1) {
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

        TNetClient::~TNetClient() {
            if (SSL_) {
                SSL_free(SSL_);
            }
        }

        void TNetClient::Cb(int filter, int flags) {
            if (flags & NMuhEv::MUHEV_FLAG_ERROR) {
                std::cerr << "EV_ERROR" << std::endl;
                Drop();
                return;
            }

            if (flags & NMuhEv::MUHEV_FLAG_EOF) {
                // NUtils::cluck(1, "EV_EOF");
                Drop();
                return;
            }

            if (filter & NMuhEv::MUHEV_FILTER_READ) {
                while (true) {
                    static const size_t bufSize = 8192;
                    char buf[bufSize];
                    int read = ReadFromSocket(buf, bufSize);

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

            if (filter & NMuhEv::MUHEV_FILTER_WRITE) {
                while (true) {
                    auto item = GetWriteItem();

                    if (!item) {
                        UpdateEvent();
                        break;
                    }

                    const int written = Write(*item);

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
            void* buf,
            const size_t bufSize
        ) {
            if (SSL_) {
                int rv = SSL_read(SSL_, buf, bufSize);

                if (rv < 0) {
                    auto err = SSL_get_error(SSL_, rv);

                    switch (err) {
                        case SSL_ERROR_WANT_WRITE: {
                            bool push(true);

                            {
                                NUtils::TSpinLockGuard guard(WriteQueueLock);
                                push = (WriteQueue.empty() || !WriteQueue.front()->Dummy);
                            }

                            if (push) {
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
                return ::recvfrom(EvIdent, buf, bufSize, MSG_DONTWAIT, NULL, 0);
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

            UpdateEvent();
            WakeLoop();
        }

        void TNetClient::UnshiftDummyWriteQueueItem() {
            auto data = std::make_shared<TWriteQueueItem>();
            data->Dummy = true;

            {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                WriteQueue.emplace_front(data);
            }

            UpdateEvent();
            WakeLoop();
        }

        void TNetClient::Destroy() {
            bool _destroyed = Destroyed.exchange(true);

            WakeLoop();

            if (_destroyed) {
                return;
            }

            Loop().RemoveEvent(*this);

            shutdown(((TNetClient::TArgs*)Args.get())->Fh, SHUT_RDWR);
            close(((TNetClient::TArgs*)Args.get())->Fh);
        }

        void TNetClient::TWriteQueueItem::CalcSize() {
            Size = 0;

            for (const auto& node : Sequence) {
                Size += node.Len;
            }
        }

        int TNetClient::Write(TWriteQueueItem& item) {
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
                written = WriteToSocket(EvIdent, str, len);

                if (written < 0) {
                    break;

                } else {
                    if (next == item.DataLast) {
                        item.DataPos += written;

                    } else {
                        item.DataLast = next;
                        item.DataPos = written;
                    }

                    item.Pos += written;
                }
            }

            return written;
        }

        void TNetClient::UpdateEvent() {
            if (!IsAlive()) {
                return;
            }

            auto filter = (NMuhEv::MUHEV_FILTER_READ | (
                ShouldWrite() ? NMuhEv::MUHEV_FILTER_WRITE : 0
            ));

            if (filter != EvFilter) {
                EvFilter = filter;
                Loop().AddEvent(*this);
            }
        }
    }
}
