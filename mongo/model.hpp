#pragma once

#include <ac-library/models/model.hpp>
#include "field.hpp"

#define AC_MONGO_MODEL_BEGIN(cls) \
    AC_MODEL_BEGIN(cls, NAC::TMongoModelBase) \
\
public: \
    using NAC::TMongoModelBase::TMongoModelBase;

#define AC_MONGO_MODEL_FIELD2(type, name, dbName) AC_MODEL_FIELD2(type, name, dbName)
#define AC_MONGO_MODEL_FIELD(type, name) AC_MODEL_FIELD(type, name)
#define AC_MONGO_MODEL_FIELD_IMPL(cls, name) AC_MODEL_FIELD_IMPL(cls, name)
#define AC_MONGO_MODEL_END() AC_MODEL_END()
#define AC_MONGO_MODEL_IMPL_START(cls) AC_MODEL_IMPL_START(cls)
#define AC_MONGO_MODEL_IMPL_END(cls) AC_MODEL_IMPL_END(cls)

namespace NAC {
    class TMongoModelBase : public TModelBase {
    public:
        void Load(const void* doc);

    protected:
        void* ACModelGet(size_t i) const override;
        void ACModelSet(size_t, std::function<void*()>&&) override;

    private:
        std::vector<std::function<void*()>> Data;
    };
}
