/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <ccommon.h>

    EXPORTED extern const IntfID BASE_OBJECT_INTF_ID;

    int EXPORTED BaseObject_addRef(BaseObject* self);
    int EXPORTED BaseObject_releaseRef(BaseObject* self);
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