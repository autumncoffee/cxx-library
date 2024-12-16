#include "field.hpp"
#include <bsoncxx/document/element.hpp>
#include <bsoncxx/types.hpp>
#include <cstdint>
#include <utility>

namespace NAC {
    std::function<void*()> TMongoInt64Field::Load(const void* doc_) const {
        const auto& doc = *(const bsoncxx::document::element*)doc_;
        int64_t val;

        if (doc.type() == bsoncxx::type::k_int64) {
            val = doc.get_int64();

            return [val](){
                return (void*)&val;
            };
        }

        if (doc.type() == bsoncxx::type::k_int32) {
            val = (int32_t)doc.get_int32();

            return [val](){
                return (void*)&val;
            };
        }

        return std::function<void*()>();
    }

    std::function<void*()> TMongoStringField::Load(const void* doc_) const {
        const auto& doc = *(const bsoncxx::document::element*)doc_;

        if (doc.type() == bsoncxx::type::k_utf8) {
            std::string val_(doc.get_utf8().value.to_string());

            return [val = std::move(val_)](){
                return (void*)&val;
            };
        }

        return std::function<void*()>();
    }

    std::function<void*()> TMongoBoolField::Load(const void* doc_) const {
        const auto& doc = *(const bsoncxx::document::element*)doc_;

        if (doc.type() == bsoncxx::type::k_bool) {
            return [val = (bool)doc.get_bool()](){
                return (void*)&val;
            };
        }

        return std::function<void*()>();
    }

    std::function<void*()> TMongoOIDField::Load(const void* doc_) const {
        const auto& doc = *(const bsoncxx::document::element*)doc_;

        if (doc.type() == bsoncxx::type::k_oid) {
            std::string val_(doc.get_oid().value.to_string());

            return [val = std::move(val_)](){
                return (void*)&val;
            };
        }

        return std::function<void*()>();
    }
}
