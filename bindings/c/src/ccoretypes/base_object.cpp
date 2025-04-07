#include "ccoretypes/base_object.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const IntfID BASE_OBJECT_INTF_ID = { daq::IBaseObject::Id.Data1, daq::IBaseObject::Id.Data2 , daq::IBaseObject::Id.Data3, daq::IBaseObject::Id.Data4_UInt64 };

int BaseObject_addRef(BaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->addRef();
}

int BaseObject_releaseRef(BaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->releaseRef();
}

ErrCode BaseObject_dispose(BaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->dispose();
}

ErrCode BaseObject_getHashCode(BaseObject* self, SizeT* hashCode)
{
    return static_cast<daq::IBaseObject*>(self)->getHashCode(hashCode);
}

ErrCode BaseObject_equals(BaseObject* self, void* other, Bool* equal)
{
    return static_cast<daq::IBaseObject*>(self)->equals(static_cast<daq::IBaseObject*>(other), equal);
}

ErrCode BaseObject_toString(BaseObject* self, CharPtr* str)
{
    return static_cast<daq::IBaseObject*>(self)->toString(str);
}

ErrCode BaseObject_create(BaseObject** baseObject)
{
    *baseObject = reinterpret_cast<BaseObject*>(daq::BaseObject_Create());
    return *baseObject == nullptr ? OPENDAQ_ERR_NOMEMORY : OPENDAQ_SUCCESS;
}

ErrCode BaseObject_queryInterface(BaseObject* self, IntfID intfId, BaseObject** interfacePtr)
{
    return static_cast<daq::IBaseObject*>(self)->queryInterface(copendaq::utils::toDaqIntfId(intfId), interfacePtr);
}

ErrCode BaseObject_borrowInterface(BaseObject* self, IntfID intfId, BaseObject** interfacePtr)
{
    return static_cast<daq::IBaseObject*>(self)->borrowInterface(copendaq::utils::toDaqIntfId(intfId), interfacePtr);
}