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
#include <coretypes/coretypes.h>
#include <coretypes/event_args.h>
#include <coreobjects/property.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IEventArgs, EventArgsPtr, "<coretypes/event_args_ptr.h>")]
 */
    
/*!
 * @ingroup objects_property
 * @addtogroup objects_property_metadata_read_args PropertyMetadataReadArgs
 * @{
 */

/*!
 * Event arguments used in property metadata `onRead` events. Currently part
 * of the "Suggested values" and "Selection values" read events. They allow
 * listeners to override the read value of the metadata field.
 *
 * Listeners of such events are expected to always override the current value
 * via the `setValue` method.
 */
DECLARE_OPENDAQ_INTERFACE(IPropertyMetadataReadArgs, IEventArgs)
{
    /*!
     * @brief Gets the property that owns the metadata field.
     * @param property The property that owns the metadata field.
     */
    virtual ErrCode INTERFACE_FUNC getProperty(IProperty** property) = 0;

    /*!
     * @brief Gets the current value of the metadata field.
     * @param value The value of the metadata field.
     */
    virtual ErrCode INTERFACE_FUNC getValue(IBaseObject** value) = 0;
    /*!
     * @brief Sets the current value of the metadata field.
     * @param value The value of the metadata field.
     */
    virtual ErrCode INTERFACE_FUNC setValue(IBaseObject* value) = 0;
};

/*!@}*/

/*!
 * @ingroup objects_property
 * @addtogroup objects_property_metadata_read_args_factories Factories
 * @{
 */

/*!
 * @brief Creates a new PropertyMetadataReadArgs object with a given property that owns the metadata field.
 * @param prop The property that owns the metadata field.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, PropertyMetadataReadArgs,
    IProperty*, prop
)

/*!@}*/

END_NAMESPACE_OPENDAQ
