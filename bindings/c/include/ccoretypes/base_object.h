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

    EXPORTED extern const daqIntfID DAQ_BASE_OBJECT_INTF_ID;

    int EXPORTED daqBaseObject_addRef(daqBaseObject* self);
    int EXPORTED daqBaseObject_releaseRef(daqBaseObject* self);
    daqErrCode EXPORTED daqBaseObject_dispose(daqBaseObject* self);
    daqErrCode EXPORTED daqBaseObject_getHashCode(daqBaseObject* self, daqSizeT* hashCode);
    daqErrCode EXPORTED daqBaseObject_equals(daqBaseObject* self, daqBaseObject* other, daqBool* equal);
    daqErrCode EXPORTED daqBaseObject_toString(daqBaseObject* self, daqCharPtr* str);
    daqErrCode EXPORTED daqBaseObject_create(daqBaseObject** baseObject);
    daqErrCode EXPORTED daqBaseObject_queryInterface(daqBaseObject* self, daqIntfID intfId, daqBaseObject** interfacePtr);
    daqErrCode EXPORTED daqBaseObject_borrowInterface(daqBaseObject* self, daqIntfID intfId, daqBaseObject** interfacePtr);

#ifdef __cplusplus
}
#endif