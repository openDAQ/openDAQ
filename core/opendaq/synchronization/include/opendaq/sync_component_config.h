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
#include <opendaq/context.h>
#include <opendaq/sync_component.h>
#include <opendaq/task_graph.h>
#include <coretypes/factory.h>

BEGIN_NAMESPACE_OPENDAQ


 /*!
 * @ingroup opendaq_signal_path
 * @addtogroup opendaq_input_port Input port
 * @{
 */

/*#
 * [interfaceSmartPtr(ISyncComponent, GenericSyncComponentPtr)]
 */

/*!
 * @brief To be filled.
 */
DECLARE_OPENDAQ_INTERFACE(ISyncComponentConfig, ISyncComponent)
{
    /*!
     * @brief test config.
     */
    //virtual ErrCode INTERFACE_FUNC test_config() = 0;
};
/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    SyncComponent,
    ISyncComponentConfig,
    IContext*, context,
    IComponent*, parent,
    IString*, localId
)

END_NAMESPACE_OPENDAQ
