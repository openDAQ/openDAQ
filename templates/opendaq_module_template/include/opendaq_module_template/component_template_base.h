#pragma once
#include <opendaq/opendaq.h>
#include <opendaq_module_template/utils.h>

BEGIN_NAMESPACE_OPENDAQ

class ComponentTemplateBase
{
public:
    virtual ~ComponentTemplateBase() = default;

private:
    virtual void configureProperties();
    virtual void configureTags(const TagsPrivatePtr& tags);
    virtual void configureStatuses(const ComponentStatusContainerPrivatePtr& statusContainer);
    virtual void propertyChanged(const StringPtr& propertyName);
    virtual ComponentAttributeConfig getAttributeConfig();
};

inline void ComponentTemplateBase::configureProperties()
{
}

inline void ComponentTemplateBase::configureTags(const TagsPrivatePtr& /*tags*/)
{
}

inline void ComponentTemplateBase::configureStatuses(const ComponentStatusContainerPrivatePtr& /*statusContainer*/)
{
}

inline void ComponentTemplateBase::propertyChanged(const StringPtr& /*propertyName*/)
{
}

inline ComponentAttributeConfig ComponentTemplateBase::getAttributeConfig()
{
    return ComponentAttributeConfig();
}

END_NAMESPACE_OPENDAQ
