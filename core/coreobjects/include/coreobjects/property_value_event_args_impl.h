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
#include <coretypes/common.h>
#include <coretypes/intfs.h>
#include <coreobjects/property_value_event_args.h>
#include <coreobjects/property_ptr.h>
#include <coretypes/event_args_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class PropertyValueEventArgsImpl : public EventArgsBase<IPropertyValueEventArgs>
{
public:
    explicit PropertyValueEventArgsImpl(const PropertyPtr& property,
                                        const ObjectPtr<IBaseObject>& value,
                                        PropertyEventType type,
                                        Bool isUpdating)
        : EventArgsImplTemplate<IPropertyValueEventArgs>(0, "PropertyValueEvent")
        , property(property)
        , newValue(value)
        , type(type)
        , updating(isUpdating)
    {
    }

    ErrCode INTERFACE_FUNC getProperty(IProperty** prop) override;
    ErrCode INTERFACE_FUNC getValue(IBaseObject** value) override;
    ErrCode INTERFACE_FUNC setValue(IBaseObject* value) override;
    ErrCode INTERFACE_FUNC getPropertyEventType(PropertyEventType* changeType) override;
    ErrCode INTERFACE_FUNC getIsUpdating(Bool* isUpdating) override;

private:
    PropertyPtr property;
    BaseObjectPtr newValue;
    PropertyEventType type;
    Bool updating;
};

END_NAMESPACE_OPENDAQ
