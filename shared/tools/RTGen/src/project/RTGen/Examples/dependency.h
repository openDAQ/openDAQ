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
#include <coretypes/common.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [templated]
 */
DECLARE_OPENDAQ_LEGACY_INTERFACE(IDependency, IBaseObject)
{
    // [templateType(output, IDependency)]
    virtual ErrCode INTERFACE_FUNC addOutput(IDependency* output) = 0;

    // [elementType(outputs, IDependency)]
    virtual ErrCode INTERFACE_FUNC setOutputs(IList* outputs) = 0;

    // [elementType(outputs, IDependency)]
    virtual ErrCode INTERFACE_FUNC getOutputs(IList** outputs) = 0;

    virtual ErrCode INTERFACE_FUNC hasOutputs(Bool* outputs) = 0;

    // [templateType(input, IDependency)]
    virtual ErrCode INTERFACE_FUNC addInput(IDependency* input) = 0;

    // [elementType(inputs, IDependency)]
    virtual ErrCode INTERFACE_FUNC setInputs(IList* inputs) = 0;

    // [elementType(inputs, IDependency)]
    virtual ErrCode INTERFACE_FUNC getInputs(IList** inputs) = 0;

    virtual ErrCode INTERFACE_FUNC hasInputs(Bool* inputs) = 0;

    virtual ErrCode INTERFACE_FUNC isTask(Bool* task) = 0;
};

END_NAMESPACE_OPENDAQ
