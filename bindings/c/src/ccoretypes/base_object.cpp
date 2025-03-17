#include "ccoretypes/base_object.h"

#include <opendaq/opendaq.h>

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