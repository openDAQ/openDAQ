#include "base_object.h"

#include <opendaq/opendaq.h>

void BaseObject_addRef(void* self)
{
    static_cast<daq::IBaseObject*>(self)->addRef();
}

void BaseObject_releaseRef(void* self)
{
    static_cast<daq::IBaseObject*>(self)->releaseRef();
}

void BaseObject_dispose(void* self)
{
    static_cast<daq::IBaseObject*>(self)->dispose();
}

void BaseObject_getHashCode(void* self, SizeT* hashCode)
{
    static_cast<daq::IBaseObject*>(self)->getHashCode(hashCode);
}

void BaseObject_equals(void* self, void* other, Bool* equal)
{
    static_cast<daq::IBaseObject*>(self)->equals(static_cast<daq::IBaseObject*>(other), equal);
}

void BaseObject_toString(void* self, CharPtr* str)
{
    static_cast<daq::IBaseObject*>(self)->toString(str);
}

void BaseObject_create(BaseObject** baseObject)
{
    *baseObject = reinterpret_cast<BaseObject*>(daq::BaseObject_Create());
}