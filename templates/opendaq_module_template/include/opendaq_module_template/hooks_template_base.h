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

    ~TemplateHooksBase()
    {
        templateImpl->removed();
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
