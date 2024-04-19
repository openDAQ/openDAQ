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
#include <coretypes/common.h>
#include <coretypes/coretype.h>
#include <coretypes/stringobject.h>
#include <coretypes/event.h>
#include <coretypes/funcobject.h>
#include <coretypes/updatable.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

enum class ConfigurationAction : EnumType
{
    Default,          // Use the mode's default action
    Skip,             // Do nothing
    UpdateProperties, // Update properties
    CreateUpdate,     // Create from factory and update properties
    CallbackUpdate,   // Create from callback and update properties
    Clear,            // Clear value
    Custom,           // Call IUpdatable.update()
    CheckObject,      // Check object for appropriate action
};

// [flags]
enum class PropertyState : EnumType
{
    Exists  = 0b001,  // Value is set in the object and configuration
    Missing = 0b010,  // Value is set in the in the object but NOT in configuration
    New     = 0b100,  // Value is NOT set but exists in configuration
};

static const Int AutoIndex = -1;

/*#
 * [defaultPtr(false)]
 * [include(IUpdatable)]
 */
DECLARE_RT_INTERFACE_EX(IPropertyInfo, IBaseObject)
{
    DEFINE_INTFID("IProperty")

    virtual ErrCode INTERFACE_FUNC getType(CoreType* value) const = 0;
    virtual ErrCode INTERFACE_FUNC setType(CoreType value) = 0;

    virtual ErrCode INTERFACE_FUNC getName(IString** value) = 0;
    virtual ErrCode INTERFACE_FUNC setName(IString* value) = 0;

    virtual ErrCode INTERFACE_FUNC getDescription(IString** value) = 0;
    virtual ErrCode INTERFACE_FUNC setDescription(IString* value) = 0;

    virtual ErrCode INTERFACE_FUNC getUnit(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setUnit(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getUnitVisible(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setUnitVisible(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getIsEnum(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setIsEnum(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getMinValue(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setMinValue(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getMaxValue(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setMaxValue(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getDefaultValue(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setDefaultValue(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getReadOnlyValue(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setReadOnlyValue(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getEnabled(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setEnabled(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getVisibleOnlyThroughRef(Bool* value) const = 0;
    virtual ErrCode INTERFACE_FUNC setVisibleOnlyThroughRef(Bool value) = 0;

    virtual ErrCode INTERFACE_FUNC getVisible(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setVisible(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getReadOnly(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setReadOnly(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getValues(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setValues(IBaseObject* value) = 0;

    // [ignore(Wrapper)]
    virtual ErrCode INTERFACE_FUNC getOnValueChanged(IEvent** value) = 0;

    virtual ErrCode INTERFACE_FUNC getRefProp(IBaseObject** value) = 0;
    virtual ErrCode INTERFACE_FUNC setRefProp(IBaseObject* value) = 0;

    virtual ErrCode INTERFACE_FUNC getIndex(Int* value) const = 0;
    virtual ErrCode INTERFACE_FUNC setIndex(Int value) = 0;

    virtual ErrCode INTERFACE_FUNC getWriteValidator(IFuncObject** validator) = 0;
    virtual ErrCode INTERFACE_FUNC setWriteValidator(IFuncObject* validator) = 0;

    virtual ErrCode INTERFACE_FUNC getReadValidator(IFuncObject** validator) = 0;
    virtual ErrCode INTERFACE_FUNC setReadValidator(IFuncObject* validator) = 0;

    virtual ErrCode INTERFACE_FUNC setLocal() = 0;
    virtual ErrCode INTERFACE_FUNC isLocal(Bool* isLocal) = 0;

    virtual ErrCode INTERFACE_FUNC getConfigurationAction(ConfigurationMode mode,
                                                          PropertyState state,
                                                          ConfigurationAction* action) = 0;

    virtual ErrCode INTERFACE_FUNC setConfigurationAction(ConfigurationMode mode,
                                                          PropertyState state,
                                                          ConfigurationAction action) = 0;

    virtual ErrCode INTERFACE_FUNC copyConfigurationActions(IPropertyInfo* prop) = 0;

    virtual ErrCode INTERFACE_FUNC isMutable(Bool* isMutable) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyInfo)

END_NAMESPACE_DEWESOFT_RT_CORE
