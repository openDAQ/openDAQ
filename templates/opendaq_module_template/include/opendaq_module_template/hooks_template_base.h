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
#include <opendaq_module_template/utils.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

template <typename Impl>
class TemplateHooksBase
{
public:
    TemplateHooksBase(std::shared_ptr<Impl> impl)
        : templateImpl(std::move(impl))
    {
    }

    void registerCallbacks(const PropertyObjectPtr& objPtr);
    std::shared_ptr<Impl> templateImpl;
};

template <typename Impl>
void TemplateHooksBase<Impl>::registerCallbacks(const PropertyObjectPtr& objPtr)
{
    objPtr.getOnAnyPropertyValueWrite() +=
        [&](const PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
        {
            PropertyEventArgs propArgs{obj, args.getProperty(), args.getProperty().getName(), args.getValue(), args.getIsUpdating()};
            if (args.getPropertyEventType() == PropertyEventType::Clear)
                propArgs.value = args.getProperty().getDefaultValue();

            const auto val = templateImpl->onPropertyWrite(propArgs);
            if (val.assigned() && args.getValue() != val)
                args.setValue(val);
        };

    objPtr.getOnAnyPropertyValueRead() +=
        [&](const PropertyObjectPtr& obj, const PropertyValueEventArgsPtr& args)
        {
        PropertyEventArgs propArgs{obj, args.getProperty(), args.getProperty().getName(), args.getValue(), args.getIsUpdating()};
            const auto val = templateImpl->onPropertyRead(propArgs);
            if (val.assigned() && args.getValue() != val)
                args.setValue(val);
        };

    objPtr.getOnEndUpdate() +=
        [&](const PropertyObjectPtr& obj, const EndUpdateEventArgsPtr& args)
        {
            std::set<StringPtr> changedProperties;
            for (const auto& prop : args.getProperties())
                changedProperties.insert(prop);

            UpdateEndArgs updateArgs{obj, changedProperties, args.getIsParentUpdating()};
            templateImpl->onEndUpdate(updateArgs);
        };

    // TODO: Is this the correct way of handling nested property objects?
    for (const auto& prop : objPtr.getAllProperties())
    {
        if (prop.getValueType() == ctObject)
            registerCallbacks(objPtr.getPropertyValue(prop.getName()));
    }
}

END_NAMESPACE_OPENDAQ_TEMPLATES
