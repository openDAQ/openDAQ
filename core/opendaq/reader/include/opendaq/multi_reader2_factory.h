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
#include <opendaq/multi_reader2_params_ptr.h>
#include <opendaq/multi_reader2_ptr.h>
#include <opendaq/multi_reader2_status_ptr.h>
#include <opendaq/multi_reader2_input_status_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*!
 * @brief Creates a params object with the defaults: every input used, automatic main input, Float64 read type,
 * MinReadCount 1, Invalidate error policy.
 */
inline MultiReader2ParamsPtr MultiReader2Params()
{
    MultiReader2ParamsPtr obj(MultiReader2Params_Create());
    return obj;
}

/*!
 * @brief Creates a multi reader configured by the params.
 * @param params The parameters; validated as by `configure`.
 */
inline MultiReader2Ptr MultiReader2(const MultiReader2ParamsPtr& params)
{
    MultiReader2Ptr obj(MultiReader2_Create(params));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
