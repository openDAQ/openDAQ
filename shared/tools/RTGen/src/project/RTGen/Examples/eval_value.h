/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <coretypes/coretypes.h>
#include <coreobjects/property_object.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

/*#
 * [includeHeader("<coreobjects/generic_property_object_ptr.h>")]
 */
DECLARE_RT_INTERFACE(IEvalValue, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getEval(IString** eval) = 0;

    virtual ErrCode INTERFACE_FUNC getResult(IBaseObject** obj) = 0;

    // [templateType(owner, IPropertyObject)]
    virtual ErrCode INTERFACE_FUNC cloneWithOwner(IPropertyObject* owner, IEvalValue** clonedValue) = 0;
    virtual ErrCode INTERFACE_FUNC getParseErrorCode() = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, EvalValue, IString*, eval)
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, EvalValueArgs, IEvalValue, IString*, eval, IList*, args)

END_NAMESPACE_DEWESOFT_RT_CORE
