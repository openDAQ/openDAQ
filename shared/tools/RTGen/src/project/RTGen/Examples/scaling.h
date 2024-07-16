/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/common.h>
#include <corestructure/sample_transform.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

/*#
 * [templated(defaultAlias: false)]
 * [interfaceSmartPtr(ISampleTransform, GenericSampleTransformPtr)]
 */
DECLARE_TEMPLATED_RT_INTERFACE_T_U(IScaling, ISampleTransform)
{
    virtual ErrCode INTERFACE_FUNC scaleValue(T inputValue, U* outputValue) = 0;

    // [arrayArg(inputValues, count), arrayArg(outputValues, count)]
    virtual ErrCode INTERFACE_FUNC scaleValues(T* inputValues, U* outputValues, SizeT count) = 0;
};

END_NAMESPACE_DEWESOFT_RT_CORE
