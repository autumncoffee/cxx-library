#include "client.hpp"
#include <muhev.hpp>
#include <iostream>
// #include <ctype>
#include <unistd.h>

namespace NAC {
    namespace NHTTPLikeServer {
        TClient::TClient(int fh, int wakeupFd, std::shared_ptr<sockaddr_in> addr)
            : Fh(fh)
            , WakeupFd(wakeupFd)
            , Addr(addr)
        {
            Destroyed = false;
        }

        void TClient::Cb(const NMuhEv::TEvSpec& event) {
            TClient* client = (TClient*)event.Ctx;

            if(event.Flags & NMuhEv::MUHEV_FLAG_ERROR) {
                std::cerr << "EV_ERROR" << std::endl;
                client->Drop();
                return;
            }

            if(event.Flags & NMuhEv::MUHEV_FLAG_EOF) {
                // NUtils::cluck(1, "EV_EOF");
                client->Drop();
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
                        client->Parser.Add(read, buf);

                    } else if(read < 0) {
                        if((errno != EAGAIN) && (errno != EINTR)) {
                            client->Drop();
                            return;

                        } else if(errno == EAGAIN) {
                            break;
                        }

                    } else {
                        client->Drop();
                        return;
                    }
                }
            }

            if(event.Filter & NMuhEv::MUHEV_FILTER_WRITE) {
                while(true) {
                    auto item = client->GetWriteItem();

                    if (!item) {
                        break;
                    }

                    if (item->WriteOffset >= item->Request.Size()) {
                        client->PopWriteItem();
                        continue;
                    }

                    int written = ::write(event.Ident, item->Request.Data() + item->WriteOffset, item->Request.Size() - item->WriteOffset);

                    if(written < 0) {
                        auto e = errno;

                        if((e != EAGAIN) && (e != EINTR)) {
                            perror("write");
                            client->Drop(); // TODO?
                        }

                        if(e != EINTR) {
                            break;
                        }

                    } else {
                        item->WriteOffset += written;
                    }
                }
            }
        }

        void TClient::Destroy() {
            bool _destroyed = Destroyed.exchange(true);

            if(_destroyed)
                return;

            shutdown(Fh, SHUT_RDWR);
            close(Fh);
        }

        std::vector<std::shared_ptr<NHTTPLikeParser::TParsedData>> TClient::GetData() {
            return Parser.ExtractData();
        }

        void TClient::PushWriteQueue(std::shared_ptr<NHTTPLikeParser::TParsedData> data) {
            if (!data) {
                return;
            }

            {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                WriteQueue.push(data);
            }

            ::write(WakeupFd, "1", 1);
        }
    }
}
