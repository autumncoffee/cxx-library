#pragma once

#include <spin_lock.hpp>
#include <queue>
#include <muhev.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace NAC {
    namespace NNetServer {
        class TBaseClient {
        public:
            struct TArgs {
                int Fh;
                int WakeupFd;
                std::shared_ptr<sockaddr_in> Addr;
            };

        public:
            explicit TBaseClient(const TArgs* const args);

            TBaseClient() = delete;
            TBaseClient(const TBaseClient&) = delete;
            TBaseClient(TBaseClient&&) = delete;

            virtual ~TBaseClient() {
            }

            virtual void Cb(const NMuhEv::TEvSpec& event) = 0;
            virtual bool IsAlive() const = 0;
            virtual bool ShouldWrite() = 0;

            void WakeLoop() const;
            void SetWeakPtr(const std::shared_ptr<TBaseClient>& ptr);

            template<typename T = TBaseClient, typename std::enable_if_t<std::is_same<T, TBaseClient>::value>* = nullptr>
            std::shared_ptr<T> GetNewSharedPtr() const {
                return std::shared_ptr<T>(SelfWeakPtr);
            }

            template<typename T = TBaseClient, typename std::enable_if_t<!std::is_same<T, TBaseClient>::value>* = nullptr>
            std::shared_ptr<T> GetNewSharedPtr() const {
                auto ptr = GetNewSharedPtr<TBaseClient>();

                return std::shared_ptr<T>(ptr, (T*)ptr.get());
            }

            int GetFh() const {
                return Args->Fh;
            }

        protected:
            const std::unique_ptr<const TArgs> Args;

        private:
            std::weak_ptr<TBaseClient> SelfWeakPtr;
        };

        class TNetClient : public TBaseClient {
        public:
            using TArgs = TBaseClient::TArgs;

            struct TWriteQueueItem {
                size_t Pos = 0;
                size_t Size = 0;
                const char* Data = nullptr;

                virtual ~TWriteQueueItem() {
                }
            };

        public:
            explicit TNetClient(const TArgs* const args);

            void Cb(const NMuhEv::TEvSpec& event) override;

            virtual void Destroy();

            virtual void Drop() {
                Destroy();
            }

            virtual void PushWriteQueue(std::shared_ptr<TWriteQueueItem> data);

            virtual std::shared_ptr<TWriteQueueItem> GetWriteItem() {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                if(WriteQueue.empty()) {
                    return nullptr;
                }

                return WriteQueue.front();
            }

            virtual void PopWriteItem() {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                WriteQueue.pop();
            }

            bool IsAlive() const override {
                return !Destroyed.load();
            }

            bool ShouldWrite() override {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                return !WriteQueue.empty();
            }

        protected:
            virtual void OnData(const size_t dataSize, char* data) = 0;

        private:
            std::atomic<bool> Destroyed;
            std::queue<std::shared_ptr<TWriteQueueItem>> WriteQueue;
            NUtils::TSpinLock WriteQueueLock;
        };
    }
}
