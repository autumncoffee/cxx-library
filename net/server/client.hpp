#pragma once

#include <ac-common/spin_lock.hpp>
#include <queue>
#include <ac-common/muhev.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "add_client.hpp"
#include <ac-library/net/client/connect.hpp>
#include <ac-common/string_sequence.hpp>
#include <utility>
#include <openssl/ssl.h>

namespace NAC {
    namespace NNetServer {
        class TBaseClient : public NMuhEv::TNode {
        public:
            struct TArgs {
                NMuhEv::TLoop* Loop = nullptr;
                TAddClient AddClient;

                virtual ~TArgs() {
                }
            };

        public:
            explicit TBaseClient(TArgs* const args);

            TBaseClient() = delete;
            TBaseClient(const TBaseClient&) = delete;
            TBaseClient(TBaseClient&&) = delete;

            virtual ~TBaseClient() {
            }

            NMuhEv::TLoop& Loop() const {
                return *Args->Loop;
            }

            virtual bool ShouldWrite() = 0;
            virtual void Drop() = 0;

            virtual void OnWire() {
            }

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

            template<typename T, typename... TArgs>
            std::shared_ptr<T> Connect(
                const char* const host,
                const short port,
                const bool ssl,
                const size_t maxRetries,
                TArgs&&... forwardClientArgs
            ) const {
                static_assert(std::is_base_of<TBaseClient, T>::value);
                static_assert(std::is_base_of<TBaseClient::TArgs, typename T::TArgs>::value);

                std::shared_ptr<sockaddr_in> addr;
                socklen_t addrLen;
                int fh;

                if (!NNetClient::Connect(host, port, addr, addrLen, fh, maxRetries)) {
                    return std::shared_ptr<T>();
                }

                std::unique_ptr<typename T::TArgs> clientArgs(new typename T::TArgs(std::forward<TArgs>(forwardClientArgs)...));
                clientArgs->Loop = Args->Loop;
                clientArgs->Fh = fh;
                clientArgs->Addr = addr;
                clientArgs->AddClient = Args->AddClient;

                FixUpConnectClientArgs(clientArgs.get(), ssl);

                std::shared_ptr<T> out(new T(clientArgs.release()));
                Args->AddClient(out);

                return out;
            }

        protected:
            virtual void FixUpConnectClientArgs(TArgs*, bool ssl) const = 0;

        protected:
            std::shared_ptr<TArgs> Args;

        private:
            std::weak_ptr<TBaseClient> SelfWeakPtr;
        };

        class TNetClient : public TBaseClient {
        public:
            struct TArgs : public TBaseClient::TArgs {
                int Fh = -1;
                std::shared_ptr<sockaddr_in> Addr;
                SSL_CTX* SSLCtx = nullptr;
                bool SSLIsClient = false;
                bool UseSSL = false;
            };

            struct TWriteQueueItem {
                virtual ~TWriteQueueItem() {
                }

                void CalcSize();

                template<typename... TArgs>
                void Concat(TArgs&&... args) {
                    Sequence.Concat(std::forward<TArgs>(args)...);
                }

            private:
                size_t Pos = 0;
                ssize_t Size = -1;
                size_t DataLast = 0;
                size_t DataPos = 0;
                TBlobSequence Sequence;
                bool Dummy = false;

                friend class TNetClient;
            };

        public:
            explicit TNetClient(TArgs* const args);

            ~TNetClient();

            void Cb(int, int) override;

            virtual void Destroy();

            void Drop() override {
                Destroy();
            }

            template<typename... TArgs>
            void PushWriteQueueData(TArgs&&... args) {
                auto item = std::make_shared<TWriteQueueItem>();
                item->Concat(std::forward<TArgs>(args)...);

                PushWriteQueue(std::move(item));
            }

            virtual void PushWriteQueue(std::shared_ptr<TWriteQueueItem> data);

            virtual std::shared_ptr<TWriteQueueItem> GetWriteItem() {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                if (WriteQueue.empty()) {
                    return nullptr;
                }

                return WriteQueue.front();
            }

            virtual void PopWriteItem() {
                NUtils::TSpinLockGuard guard(WriteQueueLock);

                WriteQueue.pop_front();
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

            virtual int ReadFromSocket(
                void* buf,
                const size_t bufSize
            );

            int Write(TWriteQueueItem& item);

            virtual int WriteToSocket(
                const int fh,
                const void* buf,
                const size_t bufSize
            );

            void UnshiftDummyWriteQueueItem();

            void FixUpConnectClientArgs(TBaseClient::TArgs* clientArgs_, bool ssl) const override {
                const auto& args = *(TNetClient::TArgs*)Args.get();
                auto&& clientArgs = *(TNetClient::TArgs*)clientArgs_;

                clientArgs.SSLIsClient = true;

                if (ssl && args.SSLCtx) {
                    clientArgs.SSLCtx = args.SSLCtx;
                    clientArgs.UseSSL = true;
                }
            }

        private:
            void UpdateEvent();

        private:
            std::atomic<bool> Destroyed;
            std::deque<std::shared_ptr<TWriteQueueItem>> WriteQueue;
            NUtils::TSpinLock WriteQueueLock;

            SSL* SSL_ = nullptr;
        };
    }
}
