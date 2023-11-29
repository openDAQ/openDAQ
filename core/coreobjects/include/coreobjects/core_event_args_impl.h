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
#include <coreobjects/core_event_args.h>
#include <coretypes/event_args_impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ
namespace core_event_args_impl
{
    static std::string getCoreEventName(const Int id)
    {
        switch(id)
        {
            case core_event_ids::PropertyValueChanged:
                return "PropertyValueChanged";
            case core_event_ids::PropertyObjectUpdateEnd:
                return "PropertyObjectUpdateEnd";
            case core_event_ids::PropertyAdded:
                return "PropertyAdded";
            case core_event_ids::PropertyRemoved:
                return "PropertyRemoved";
            case core_event_ids::ComponentAdded:
                return "ComponentAdded";
            case core_event_ids::ComponentRemoved:
                return "ComponentRemoved";
            case core_event_ids::SignalConnected:
                return "SignalConnected";
            case core_event_ids::SignalDisconnected:
                return "SignalDisconnected";
            case core_event_ids::DataDescriptorChanged:
                return "DataDescriptorChanged";
            case core_event_ids::ComponentUpdateEnd:
                return "ComponentUpdateEnd";
            case core_event_ids::AttributeChanged:
                return "AttributeChanged";
            default:
                break;
        }

        return "Unknown";
    }
}


class CoreEventArgsImpl : public EventArgsBase<ICoreEventArgs>
{
public:
    explicit CoreEventArgsImpl (Int id, const DictPtr<IString, IBaseObject>& parameters);

    ErrCode INTERFACE_FUNC getParameters(IDict** parameters) override;
private:
    DictPtr<IString, IBaseObject> parameters;
    bool validateParameters() const;
};


inline CoreEventArgsImpl::CoreEventArgsImpl(Int id, const DictPtr<IString, IBaseObject>& parameters)
    : EventArgsImplTemplate<ICoreEventArgs>(id, core_event_args_impl::getCoreEventName(id))
    , parameters(parameters)
{
    if (!validateParameters())
        throw InvalidParameterException{"Core event parameters for event type \"{}\" are invalid", this->eventName};
}

inline ErrCode CoreEventArgsImpl::getParameters(IDict** parameters)
{
    OPENDAQ_PARAM_NOT_NULL(parameters);

    *parameters = this->parameters.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

inline bool CoreEventArgsImpl::validateParameters() const
{
    switch(eventId)
    {
        case core_event_ids::PropertyValueChanged:
            return parameters.hasKey("Name") && parameters.hasKey("Value") && parameters.hasKey("Owner");
        case core_event_ids::PropertyObjectUpdateEnd:
            return parameters.hasKey("UpdatedProperties") && parameters.get("UpdatedProperties").asPtrOrNull<IDict>().assigned() && parameters.hasKey("Owner");
        case core_event_ids::PropertyAdded:
            return parameters.hasKey("Property") && parameters.hasKey("Owner");
        case core_event_ids::PropertyRemoved:
            return parameters.hasKey("Name") && parameters.hasKey("Owner");
        case core_event_ids::ComponentAdded:
            return parameters.hasKey("Component");
        case core_event_ids::ComponentRemoved:
            return parameters.hasKey("Id");
        case core_event_ids::SignalConnected:
            return parameters.hasKey("Signal");
        case core_event_ids::DataDescriptorChanged:
            return parameters.hasKey("DataDescriptor");
        case core_event_ids::AttributeChanged:
            return parameters.hasKey("AttributeName");
        default:
            break;
    }

    return true;
}

END_NAMESPACE_OPENDAQ
