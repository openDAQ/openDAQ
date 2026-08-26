/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/multi_reader2.h>
#include <opendaq/multi_reader2_params.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*!
 * @brief Creates an empty parameter object for a MultiReader2.
 */
inline ObjectPtr<IMultiReader2Params> MultiReader2Params()
{
    ObjectPtr<IMultiReader2Params> obj(MultiReader2Params_Create());
    return obj;
}

/*!
 * @brief Creates a MultiReader2 configured by the given parameters.
 * @param params The parameters holding the input list and read settings.
 */
inline ObjectPtr<IMultiReader2> MultiReader2(const ObjectPtr<IMultiReader2Params>& params)
{
    ObjectPtr<IMultiReader2> obj(MultiReader2_Create(params));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
