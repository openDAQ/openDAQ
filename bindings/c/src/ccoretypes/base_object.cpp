#include "ccoretypes/base_object.h"

#include <opendaq/opendaq.h>

#include "copendaq_private.h"

const daqIntfID DAQ_BASE_OBJECT_INTF_ID = { daq::IBaseObject::Id.Data1, daq::IBaseObject::Id.Data2 , daq::IBaseObject::Id.Data3, daq::IBaseObject::Id.Data4_UInt64 };

int daqBaseObject_addRef(daqBaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->addRef();
}

int daqBaseObject_releaseRef(daqBaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->releaseRef();
}

daqErrCode daqBaseObject_dispose(daqBaseObject* self)
{
    return static_cast<daq::IBaseObject*>(self)->dispose();
}

daqErrCode daqBaseObject_getHashCode(daqBaseObject* self, daqSizeT* hashCode)
{
    return static_cast<daq::IBaseObject*>(self)->getHashCode(hashCode);
}

daqErrCode daqBaseObject_equals(daqBaseObject* self, void* other, daqBool* equal)
{
    return static_cast<daq::IBaseObject*>(self)->equals(static_cast<daq::IBaseObject*>(other), equal);
}

daqErrCode daqBaseObject_toString(daqBaseObject* self, daqCharPtr* str)
{
    return static_cast<daq::IBaseObject*>(self)->toString(str);
}

daqErrCode daqBaseObject_create(daqBaseObject** baseObject)
{
    *baseObject = reinterpret_cast<daqBaseObject*>(daq::BaseObject_Create());
    return *baseObject == nullptr ? OPENDAQ_ERR_NOMEMORY : OPENDAQ_SUCCESS;
}

daqErrCode daqBaseObject_queryInterface(daqBaseObject* self, daqIntfID intfId, daqBaseObject** interfacePtr)
{
    return static_cast<daq::IBaseObject*>(self)->queryInterface(copendaq::utils::toDaqIntfId(intfId), interfacePtr);
}

daqErrCode daqBaseObject_borrowInterface(daqBaseObject* self, daqIntfID intfId, daqBaseObject** interfacePtr)
{
    return static_cast<daq::IBaseObject*>(self)->borrowInterface(copendaq::utils::toDaqIntfId(intfId), interfacePtr);
}