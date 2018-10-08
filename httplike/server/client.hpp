#pragma once

#include <spin_lock.hpp>
#include <queue>
#include <muhev.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <httplike/parser/parser.hpp>

namespace NAC {
    namespace NHTTPLikeServer {
        class TClient {
        private:
            int Fh;
            int WakeupFd;
            std::shared_ptr<sockaddr_in> Addr;
            std::atomic<bool> Destroyed;
            NHTTPLikeParser::TParser Parser;
            std::queue<std::shared_ptr<NHTTPLikeParser::TParsedData>> WriteQueue;
            NUtils::TSpinLock WriteQueueLock;

        public:
            TClient(int fh, int wakeupFd, std::shared_ptr<sockaddr_in> addr);

            static void Cb(const NMuhEv::TEvSpec& event);
            void PushWriteQueue(std::shared_ptr<NHTTPLikeParser::TParsedData> data);
            std::vector<std::shared_ptr<NHTTPLikeParser::TParsedData>> GetData();

            bool IsAlive() const {
                return !Destroyed.load();
            }

            int GetFh() const {
                return Fh;
            }

            bool ShouldWrite() {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                return !WriteQueue.empty();
            }

        private:
            void Destroy();

            void Drop() {
                Destroy();
            }

            std::shared_ptr<NHTTPLikeParser::TParsedData> GetWriteItem() {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                if(WriteQueue.empty()) {
                    return nullptr;
                }

                return WriteQueue.front();
            }

            void PopWriteItem() {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                WriteQueue.pop();
            }
        };
    }
}
