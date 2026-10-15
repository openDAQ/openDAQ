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
#include <coreobjects/property_object.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IInteger, IntegerPtr, "<coretypes/integer.h>")]
 * [interfaceLibrary(IPropertyObject, CoreObjects)]
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

enum class SyncSourceStatus : EnumType
{
    Off = 0,
    Listening,
    Calibrating,
    Synced,
    Error,
    Unknown
};

enum class SyncRoleStatus : EnumType
{
    Off = 0,
    Input,
    Output,
    Unknown
};

/*!
 * @brief Interface representing a Synchronization Interface.
 *
 * A property object. Use the methods below; the property tree is for special cases only.
 */
DECLARE_OPENDAQ_INTERFACE(ISyncInterface, IBaseObject)
{
    /*!
     * @brief Gets the ID of the synchronization interface.
     * @param[out] id The ID of the synchronization interface.
     *
     * Unique and immutable. It is the key in ISynchronization::getInterfaces and the argument of
     * ISynchronization::setSource.
     */
    virtual ErrCode INTERFACE_FUNC getId(IString** id) = 0;

    /*!
     * @brief Gets the reference type of the interface's reference domain ID.
     * @param[out] syncType The synchronization type string.
     *
     * Lowercase. Known types are "local", "ptp", "ntp", "gps" and "irig"; others are allowed.
     * The reference domain ID is this value, or starts with it followed by ':'.
     */
    virtual ErrCode INTERFACE_FUNC getSyncType(IString** syncType) = 0;

    /*!
     * @brief Gets the reference domain ID of the synchronization interface.
     * @param[out] referenceDomainId The reference domain ID string.
     *
     * The ID observed in the input role, or produced in the output role. Empty when none is
     * available, and always empty when the mode is `Off`.
     */
    virtual ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) = 0;

    /*!
     * @brief Gets whether the synchronization interface can be selected as the synchronization source.
     * @param[out] canBeSource True if the interface supports `Input` or `Auto`; False otherwise.
     */
    virtual ErrCode INTERFACE_FUNC getSourceSupported(Bool* canBeSource) = 0;

    /*!
     * @brief Sets the mode of the synchronization interface.
     * @param mode The mode to set the synchronization interface to. Must be one of the modes
     * returned by `getAvailableModes`.
     */
    virtual ErrCode INTERFACE_FUNC setMode(SyncMode mode) = 0;

    /*!
     * @brief Gets the current mode of the synchronization interface.
     * @param[out] sourceMode The current mode of the synchronization interface.
     */
    virtual ErrCode INTERFACE_FUNC getMode(SyncMode* sourceMode) = 0;

    /*!
     * @brief Gets the modes available to the synchronization interface, depending on whether
     * it is currently selected as the synchronization source.
     * @param[out] availableModes A dictionary mapping available `SyncMode` values to their names.
     *
     * `Auto` and `Input` while the interface is the source, `Off` and `Output` otherwise, limited
     * to the modes the interface supports.
     */
    // [templateType(availableModes, IInteger, IString)]
    virtual ErrCode INTERFACE_FUNC getAvailableModes(IDict** availableModes) = 0;

    /*!
     * @brief Gets the configuration property object of the synchronization interface.
     * @param[out] configuration The configuration property object. Always holds `Mode`.
     */
    virtual ErrCode INTERFACE_FUNC getConfiguration(IPropertyObject** configuration) = 0;

    /*!
     * @brief Gets the status container of the synchronization interface.
     * @param[out] syncStatus The status container.
     *
     * Holds two standard statuses; implementations may add more.
     *
     * SynchronizationSourceStatus: Off, Listening, Calibrating, Synced, Error, Unknown.
     * - Off: the mode is `Off`.
     * - Listening: waiting for a source or for role negotiation.
     * - Calibrating: a source is found but not yet applied to the device clock.
     * - Synced: the input is applied to the clock, or the output is actively distributing.
     * - Error: a misconfiguration, or a protocol, network or device error; the message describes it.
     * - Unknown: the state cannot be determined.
     *
     * SynchronizationRoleStatus: Off, Input, Output, Unknown.
     * Matches the mode for `Off`, `Input` and `Output`. For `Auto` it is the negotiated role, and
     * `Unknown` while negotiating.
     */
    virtual ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** syncStatus) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
