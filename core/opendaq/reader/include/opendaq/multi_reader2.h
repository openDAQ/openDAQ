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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

struct IComponent;

/*!
 * @ingroup opendaq_readers
 * @addtogroup opendaq_multi_reader Multi reader
 * @{
 */

/*!
 * @brief The kind of inputs a multi reader accepts; chosen at construction, signals and ports never mix.
 */
enum class MultiReader2InputType : EnumType
{
    Signals = 0,
    Ports
};

/*!
 * @brief Reads multiple signals at once.
 */
DECLARE_OPENDAQ_INTERFACE(IMultiReader2, IBaseObject)
{
    /*!
     * @brief Adds a signal or an input port as a reader input, matching the constructed input type.
     * @param input The signal or input port to add.
     */
    virtual ErrCode INTERFACE_FUNC addInput(IComponent* input) = 0;

    /*!
     * @brief Removes a previously added signal or input port from the reader inputs.
     * @param input The signal or input port to remove.
     */
    virtual ErrCode INTERFACE_FUNC removeInput(IComponent* input) = 0;
};

/*!@}*/

END_NAMESPACE_OPENDAQ
