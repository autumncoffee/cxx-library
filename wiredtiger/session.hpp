#pragma once

#include <string>
#include "model.hpp"
#include "iterator.hpp"

namespace NAC {
    class TWiredTigerSessionImpl;

    class TWiredTigerSession {
    public:
        TWiredTigerSession() = delete;
        TWiredTigerSession(void*);

        TWiredTigerSession(const TWiredTigerSession&) = delete;
        TWiredTigerSession(TWiredTigerSession&& right) {
            Impl = right.Impl;
            right.Impl = nullptr;
        }

        ~TWiredTigerSession();

    private:
        void CreateTable(const std::string&, const std::string&, const std::string&, const std::string&);
        void CreateIndex(const std::string&, const std::string&, const std::string&);
        bool Insert(const std::string&, const TWiredTigerModelBase&, const TWiredTigerModelBase&);
        bool Set(const std::string&, const TWiredTigerModelBase&, const TWiredTigerModelBase&);
        bool Append(const std::string&, TWiredTigerModelBase&, const TWiredTigerModelBase&);
        bool Get(const std::string&, const TWiredTigerModelBase&, TWiredTigerModelBase&);
        TWiredTigerIterator Search(const std::string&, const std::string&, const std::string&, const TWiredTigerModelBase&, int);
        bool Remove(const std::string&, const TWiredTigerModelBase&);

    public:
        template<class TWiredTigerModelDescr>
        void CreateTable() {
            CreateTable(
                TWiredTigerModelDescr::DBName,
                TWiredTigerModelDescr::TKey::__ACModelGetFullFormatStatic(),
                TWiredTigerModelDescr::TValue::__ACModelGetFullFormatStatic(),
                TWiredTigerModelDescr::FieldNames()
            );
        }

        template<class TWiredTigerIndexDescr>
        void CreateIndex() {
            CreateIndex(
                TWiredTigerIndexDescr::TModel::DBName,
                TWiredTigerIndexDescr::DBName,
                TWiredTigerIndexDescr::TKey::__ACModelGetFieldNameListStatic()
            );
        }

        template<class TWiredTigerModelDescr>
        bool Insert(
            const typename TWiredTigerModelDescr::TKey& key,
            const typename TWiredTigerModelDescr::TValue& data
        ) {
            return Insert(
                TWiredTigerModelDescr::DBName,
                key,
                data
            );
        }

        template<class TWiredTigerModelDescr>
        bool Set(
            const typename TWiredTigerModelDescr::TKey& key,
            const typename TWiredTigerModelDescr::TValue& data
        ) {
            return Set(
                TWiredTigerModelDescr::DBName,
                key,
                data
            );
        }

        template<class TWiredTigerModelDescr>
        bool Append(
            typename TWiredTigerModelDescr::TKey& key,
            const typename TWiredTigerModelDescr::TValue& data
        ) {
            return Append(
                TWiredTigerModelDescr::DBName,
                key,
                data
            );
        }

        template<class TWiredTigerModelDescr>
        bool Get(
            const typename TWiredTigerModelDescr::TKey& key,
            typename TWiredTigerModelDescr::TValue& out
        ) {
            return Get(
                TWiredTigerModelDescr::DBName,
                key,
                out
            );
        }

        template<class TWiredTigerIndexDescr, class TValue = typename TWiredTigerIndexDescr::TModel::TValue>
        TWiredTigerIterator Search(
            const typename TWiredTigerIndexDescr::TKey& key,
            int direction = 0
        ) {
            return Search(
                TWiredTigerIndexDescr::TModel::DBName,
                TWiredTigerIndexDescr::DBName,
                TValue::__ACModelGetFieldNameListStatic(),
                key,
                direction
            );
        }

        template<class TWiredTigerModelDescr>
        bool Remove(
            const typename TWiredTigerModelDescr::TKey& key
        ) {
            return Remove(
                TWiredTigerModelDescr::DBName,
                key
            );
        }

    private:
        TWiredTigerSessionImpl* Impl;
    };
}
