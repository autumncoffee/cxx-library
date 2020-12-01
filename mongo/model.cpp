#include "model.hpp"
#include <bsoncxx/document/view.hpp>
#include <stdexcept>
#include <utility>

namespace NAC {
    void TMongoModelBase::Load(const void* doc_) {
        decltype(Data) newData;
        const auto& doc = *(const bsoncxx::document::view*)doc_;

        newData.reserve(GetACModelFields().size());

        for (const auto& field : GetACModelFields()) {
            const auto& value = doc[field->Name];

            if (auto&& cb = ((const TMongoFieldBase*)field.get())->Load(&value)) {
                newData.emplace_back(std::move(cb));

            } else {
                newData.emplace_back(decltype(newData)::value_type());
            }
        }

        std::swap(Data, newData);
    }

    void* TMongoModelBase::ACModelGet(size_t i) const {
        return Data.at(i)();
    }

    void TMongoModelBase::ACModelSet(size_t, std::function<void*()>&&) {
        throw std::runtime_error("MongoDB models are read-only");
    }
}
