#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    EXPORTED extern const IntfID BASE_OBJECT_INTF_ID;

    ErrCode EXPORTED BaseObject_addRef(BaseObject* self);
    ErrCode EXPORTED BaseObject_releaseRef(BaseObject* self);
    ErrCode EXPORTED BaseObject_dispose(BaseObject* self);
    ErrCode EXPORTED BaseObject_getHashCode(BaseObject* self, SizeT* hashCode);
    ErrCode EXPORTED BaseObject_equals(BaseObject* self, BaseObject* other, Bool* equal);
    ErrCode EXPORTED BaseObject_toString(BaseObject* self, CharPtr* str);
    ErrCode EXPORTED BaseObject_create(BaseObject** baseObject);
    ErrCode EXPORTED BaseObject_queryInterface(BaseObject* self, IntfID intfId, BaseObject** interfacePtr);
    ErrCode EXPORTED BaseObject_borrowInterface(BaseObject* self, IntfID intfId, BaseObject** interfacePtr);

#ifdef __cplusplus
}
#endif