//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.7.0) on 03.06.2025 22:05:12.
// </auto-generated>
//------------------------------------------------------------------------------

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

    typedef struct daqEvalValue daqEvalValue;
    typedef struct daqString daqString;
    typedef struct daqPropertyObject daqPropertyObject;
    typedef struct daqList daqList;
    typedef struct daqFunction daqFunction;

    EXPORTED extern const daqIntfID DAQ_EVAL_VALUE_INTF_ID;

    daqErrCode EXPORTED daqEvalValue_getEval(daqEvalValue* self, daqString** eval);
    daqErrCode EXPORTED daqEvalValue_getResult(daqEvalValue* self, daqBaseObject** obj);
    daqErrCode EXPORTED daqEvalValue_cloneWithOwner(daqEvalValue* self, daqPropertyObject* owner, daqEvalValue** clonedValue);
    daqErrCode EXPORTED daqEvalValue_getParseErrorCode(daqEvalValue* self);
    daqErrCode EXPORTED daqEvalValue_getPropertyReferences(daqEvalValue* self, daqList** propertyReferences);
    daqErrCode EXPORTED daqEvalValue_getResultNoLock(daqEvalValue* self, daqBaseObject** obj);
    daqErrCode EXPORTED daqEvalValue_createEvalValue(daqEvalValue** obj, daqString* eval);
    daqErrCode EXPORTED daqEvalValue_createEvalValueArgs(daqEvalValue** obj, daqString* eval, daqList* args);
    daqErrCode EXPORTED daqEvalValue_createEvalValueFunc(daqEvalValue** obj, daqString* eval, daqFunction* func);

#ifdef __cplusplus
}
#endif
