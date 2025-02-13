#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    typedef void BaseObject;

    void EXPORTED BaseObject_addRef(void* self);
    void EXPORTED BaseObject_releaseRef(void* self);
    void EXPORTED BaseObject_dispose(void* self);
    void EXPORTED BaseObject_getHashCode(void* self, SizeT* hashCode);
    void EXPORTED BaseObject_equals(void* self, void* other, Bool* equal);
    void EXPORTED BaseObject_toString(void* self, CharPtr* str);
    void EXPORTED BaseObject_create(BaseObject** baseObject);

#ifdef __cplusplus
}
#endif