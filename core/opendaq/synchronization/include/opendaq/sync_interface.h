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
#include <coretypes/stringobject.h>
#include <coretypes/listobject.h>
#include <opendaq/component_status_container.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 */


/*!
 * @ingroup opendaq_synchronization_path
 * @addtogroup opendaq_sync_interface Sync Interface
 * @{
 */

enum class SyncMode : EnumType
{
  Off = 0,  ///> Interface is disabled.
  Input,    ///> Interface can only receive a synchronization reference.
  Output,   ///> Interface can only distribute the device clock.
  Auto      ///> Interface automatically selects its active role.
};

/*!
 * @brief Interface representing a Synchronization Interface.
 */
DECLARE_OPENDAQ_INTERFACE(ISyncInterface, IBaseObject)
{
    /*!
     * @brief Gets the name of the synchronization interface.
     * @param[out] name The name of the synchronization interface.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    /*!
     * @brief Gets the reference domain ID of the synchronization interface.
     * @param[out] referenceDomainId The reference domain ID string.
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) = 0;

    virtual ErrCode INTERFACE_FUNC getMode(SyncMode* sourceMode) = 0;

    // [templateType(availableModes, IInteger, IString)]
    virtual ErrCode INTERFACE_FUNC getAvailableModes(IDict** availableModes) = 0;

    virtual ErrCode INTERFACE_FUNC setOutputOnly(Bool outputOnly) = 0;
    virtual ErrCode INTERFACE_FUNC getOutputOnly(Bool* outputOnly) = 0;

    virtual ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** syncStatus) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
