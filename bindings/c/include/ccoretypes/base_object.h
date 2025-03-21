#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    ErrCode EXPORTED BaseObject_addRef(BaseObject* self);
    ErrCode EXPORTED BaseObject_releaseRef(BaseObject* self);
    ErrCode EXPORTED BaseObject_dispose(BaseObject* self);
    ErrCode EXPORTED BaseObject_getHashCode(BaseObject* self, SizeT* hashCode);
    ErrCode EXPORTED BaseObject_equals(BaseObject* self, BaseObject* other, Bool* equal);
    ErrCode EXPORTED BaseObject_toString(BaseObject* self, CharPtr* str);
    ErrCode EXPORTED BaseObject_create(BaseObject** baseObject);

#ifdef __cplusplus
}
#endif