#include "ccoretypes/base_object.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const daqIntfID BASE_OBJECT_INTF_ID = { daq::IBaseObject::Id.Data1, daq::IBaseObject::Id.Data2 , daq::IBaseObject::Id.Data3, daq::IBaseObject::Id.Data4_UInt64 };

int BaseObject_addRef(daqBaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->addRef();
}

int BaseObject_releaseRef(daqBaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->releaseRef();
}

daqErrCode BaseObject_dispose(daqBaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->dispose();
}

daqErrCode BaseObject_getHashCode(daqBaseObject* self, daqSizeT* hashCode)
{
    return static_cast<daq::IBaseObject*>(self)->getHashCode(hashCode);
}

daqErrCode BaseObject_equals(daqBaseObject* self, void* other, daqBool* equal)
{
    return static_cast<daq::IBaseObject*>(self)->equals(static_cast<daq::IBaseObject*>(other), equal);
}

daqErrCode BaseObject_toString(daqBaseObject* self, daqCharPtr* str)
{
    return static_cast<daq::IBaseObject*>(self)->toString(str);
}

daqErrCode BaseObject_create(daqBaseObject** baseObject)
{
    *baseObject = reinterpret_cast<daqBaseObject*>(daq::BaseObject_Create());
    return *baseObject == nullptr ? OPENDAQ_ERR_NOMEMORY : OPENDAQ_SUCCESS;
}

daqErrCode BaseObject_queryInterface(daqBaseObject* self, daqIntfID intfId, daqBaseObject** interfacePtr)
{
    return static_cast<daq::IBaseObject*>(self)->queryInterface(copendaq::utils::toDaqIntfId(intfId), interfacePtr);
}

daqErrCode BaseObject_borrowInterface(daqBaseObject* self, daqIntfID intfId, daqBaseObject** interfacePtr)
{
    return static_cast<daq::IBaseObject*>(self)->borrowInterface(copendaq::utils::toDaqIntfId(intfId), interfacePtr);
}