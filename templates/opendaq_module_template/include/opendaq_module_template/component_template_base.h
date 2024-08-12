#pragma once
#include <opendaq/opendaq.h>
#include <opendaq_module_template/utils.h>

BEGIN_NAMESPACE_OPENDAQ

class ComponentTemplateBase
{
public:
    virtual ~ComponentTemplateBase() = default;

protected:
    friend class DeviceTemplateHooks;

    virtual void initProperties();
    virtual void initTags(const TagsPrivatePtr& tags);
    virtual void initStatuses(const ComponentStatusContainerPrivatePtr& statusContainer);
    virtual void propertyChanged(const StringPtr& propertyName);
    virtual ComponentAttributeConfig getAttributeConfig();
};

inline void ComponentTemplateBase::initProperties()
{
}

inline void ComponentTemplateBase::initTags(const TagsPrivatePtr& /*tags*/)
{
}

inline void ComponentTemplateBase::initStatuses(const ComponentStatusContainerPrivatePtr& /*statusContainer*/)
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
