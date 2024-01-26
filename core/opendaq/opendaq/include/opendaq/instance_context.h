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
#include <coretypes/dictobject_factory.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_devices
 * @addtogroup opendaq_instance IInstanceContext
 * @{
 */
DECLARE_OPENDAQ_INTERFACE(IInstanceContext, IBaseObject)
{
    // [templateType(options, IString, IBaseObject)]
    /*!
     * @brief Gets the dictionary of parsed variables which can be used for configuring Instance object
     * @param[out] options The dictionary of parsed variables which can be used for configuring Instance object
     */
    virtual ErrCode INTERFACE_FUNC getOptions(IDict** options) = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, InstanceContext, IInstanceContext)

END_NAMESPACE_OPENDAQ
