/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

/*!
 * @ingroup opendaq_signal_path
 * @addtogroup opendaq_input_port Input port
 * @{
 */

/*!
 * @brief Internal functions used by openDAQ core. This interface should never be used in
 * client SDK or module code.
 */
DECLARE_OPENDAQ_INTERFACE(IInputPortPrivate, IBaseObject)
{
    /*!
     * @brief Disconnects the signal without notification to the signal.
     */
    virtual ErrCode INTERFACE_FUNC disconnectWithoutSignalNotification() = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
