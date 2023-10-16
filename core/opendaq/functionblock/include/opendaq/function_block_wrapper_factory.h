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
#include <opendaq/function_block_wrapper_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_function_block
 * @addtogroup opendaq_function_block_factories Factories
 * @{
 */

/*!
 * @brief Creates a FunctionBlockWrapper from an existing function block instance.
 * @param functionBlock The existing function block.
 * @param includeInputPortsByDefault If true, input ports will be included in the list of input ports by default.
 * @param includeSignalsByDefault If true, signals will be included in the list of signals by default.
 * @param includePropertiesByDefault If true, properties will be included in the list of properties by default.
 * @param includeFunctionBlocksByDefault If true, function blocks will be included in the list of sub-function blocks by default.
 */
inline FunctionBlockPtr FunctionBlockWrapper(
    const FunctionBlockPtr& functionBlock,
    Bool includeInputPortsByDefault,
    Bool includeSignalsByDefault,
    Bool includePropertiesByDefault,
    Bool includeFunctionBlocksByDefault)
{
    FunctionBlockPtr obj(FunctionBlockWrapper_Create(functionBlock, includeInputPortsByDefault, includeSignalsByDefault, includePropertiesByDefault, includeFunctionBlocksByDefault));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
