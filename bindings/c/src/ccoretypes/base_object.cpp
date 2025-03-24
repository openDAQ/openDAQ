#include "ccoretypes/base_object.h"

#include <opendaq/opendaq.h>

const IntfID BASE_OBJECT_INTF_ID = { daq::IBaseObject::Id.Data1, daq::IBaseObject::Id.Data2 , daq::IBaseObject::Id.Data3, daq::IBaseObject::Id.Data4_UInt64 };

ErrCode BaseObject_addRef(BaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->addRef();
}

ErrCode BaseObject_releaseRef(BaseObject* self)
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
    return 0;
}

ErrCode BaseObject_queryInterface(BaseObject* self, IntfID intfId, BaseObject** interfacePtr)
{
    daq::IntfID id { intfId.Data1, intfId.Data2, intfId.Data3, { .Data4_UInt64 = intfId.Data4 } };
    return static_cast<daq::IBaseObject*>(self)->queryInterface(id, interfacePtr);
}

ErrCode BaseObject_borrowInterface(BaseObject* self, IntfID intfId, BaseObject** interfacePtr)
{
    daq::IntfID id { intfId.Data1, intfId.Data2, intfId.Data3, { .Data4_UInt64 = intfId.Data4 } };
    return static_cast<daq::IBaseObject*>(self)->borrowInterface(id, interfacePtr);
}