#pragma once
#include <opendaq/opendaq.h>
#include <opendaq_module_template/utils.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

template <typename Type>
class ComponentTemplateBase;
class AddableComponentTemplateBase;
class FunctionBlockTemplateBase;

template <typename Type>
class ComponentTemplateBase
{
public:
    virtual ~ComponentTemplateBase() = default;

    ComponentPtr getComponent();
    
    void removeComponentWithId(const FolderConfigPtr& parentFolder, const std::string& componentId) const;
    void removeComponent(const FolderConfigPtr& parentFolder, const ComponentPtr& component) const;
    
    template <class FBImpl, class... Params>
    FunctionBlockPtr createAndAddFunctionBlock(const std::string& fbId, Params&&... params) const;

    virtual void initSignals(const FolderConfigPtr& signalsFolder);
    virtual void initFunctionBlocks(const FolderConfigPtr& fbFolder);
    virtual void handleOptions(const DictPtr<IString, IBaseObject>& options);

    virtual void initProperties();
    virtual void initTags(const TagsPrivatePtr& tags);
    virtual void initStatuses(const ComponentStatusContainerPrivatePtr& statusContainer);
    virtual void propertyChanged(const StringPtr& propertyName);
    virtual ComponentAttributeConfig getAttributeConfig();
    
    virtual void start();

    virtual BaseObjectPtr onPropertyWrite(const PropertyObjectPtr& owner, const StringPtr& propertyName, const PropertyPtr& property, const BaseObjectPtr& value);
    virtual BaseObjectPtr onPropertyRead(const PropertyObjectPtr& owner, const StringPtr& propertyName, const PropertyPtr& property, const BaseObjectPtr& value);


    LoggerComponentPtr loggerComponent;
    ContextPtr context;
    std::mutex sync;
    Type* componentImpl;
};

class AddableComponentTemplateBase
{
public:
    virtual ~AddableComponentTemplateBase() = default;
    virtual void handleConfig(const PropertyObjectPtr& config);
};

class FunctionBlockTemplateBase
{
public:
    virtual ~FunctionBlockTemplateBase() = default;
    virtual void initInputPorts(const FolderConfigPtr& inputPortsFolder);
};

template <typename Type>
ComponentPtr ComponentTemplateBase<Type>::getComponent()
{
    return componentImpl->objPtr;
}

template <typename Type>
void ComponentTemplateBase<Type>::removeComponentWithId(const FolderConfigPtr& parentFolder, const std::string& componentId) const
{
    if (parentFolder.assigned())
        throw NotAssignedException{"Parent component is not assigned."};
    if (componentId.empty())
        throw NotAssignedException{"Missing component removal target local ID."};

    if(parentFolder.hasItem(componentId))
        parentFolder.removeItemWithLocalId(componentId);
    else
        LOG_W("Component with id {} not found in parent folder", componentId);

    LOG_T("Component with id {} removed from parent folder", componentId)
}

template <typename Type>
void ComponentTemplateBase<Type>::removeComponent(const FolderConfigPtr& parentFolder, const ComponentPtr& component) const
{
    if (parentFolder.assigned())
        throw NotAssignedException{"Parent component is not assigned."};
    if (component.assigned())
        throw NotAssignedException{"Component slated for removal is not assigned."};

    if(parentFolder.hasItem(component.getLocalId()))
        parentFolder.removeItem(component);
    else
        LOG_W("Component with id {} not found in parent folder", component.getLocalId());

    LOG_T("Component with id {} removed from parent folder", component.getLocalId())
}

template <typename Type>
template <class FBImpl, class ... Params>
FunctionBlockPtr ComponentTemplateBase<Type>::createAndAddFunctionBlock(const std::string& fbId, Params&&... params) const
{
    LOG_T("Adding function block {}", fbId)
    auto fb = createWithImplementation<IFunctionBlock, FBImpl>(context, componentImpl->functionBlocks, fbId, std::forward<Params>(params)...);
    componentImpl->functionBlocks.addItem(fb);
    return fb;
}

inline void AddableComponentTemplateBase::handleConfig(const PropertyObjectPtr& /*config*/)
{
}

template <typename Type>
void ComponentTemplateBase<Type>::initSignals(const FolderConfigPtr& /*signalsFolder*/)
{
}

template <typename Type>
void ComponentTemplateBase<Type>::initFunctionBlocks(const FolderConfigPtr& /*fbFolder*/)
{
}

template <typename Type>
void ComponentTemplateBase<Type>::handleOptions(const DictPtr<IString, IBaseObject>& /*options*/)
{
}

template <typename Type>
void ComponentTemplateBase<Type>::initProperties()
{
}

template <typename Type>
void ComponentTemplateBase<Type>::initTags(const TagsPrivatePtr& /*tags*/)
{
}

template <typename Type>
void ComponentTemplateBase<Type>::initStatuses(const ComponentStatusContainerPrivatePtr& /*statusContainer*/)
{
}

template <typename Type>
void ComponentTemplateBase<Type>::propertyChanged(const StringPtr& /*propertyName*/)
{
}

template <typename Type>
ComponentAttributeConfig ComponentTemplateBase<Type>::getAttributeConfig()
{
    return {};
}

template <typename Type>
void ComponentTemplateBase<Type>::start()
{
}

template <typename Type>
BaseObjectPtr ComponentTemplateBase<Type>::onPropertyWrite(const PropertyObjectPtr& /*owner*/,
                                                           const StringPtr& /*propertyName*/,
                                                           const PropertyPtr& /*property*/,
                                                           const BaseObjectPtr& /*value*/)
{
    return nullptr;
}

template <typename Type>
BaseObjectPtr ComponentTemplateBase<Type>::onPropertyRead(const PropertyObjectPtr& /*owner*/,
                                                          const StringPtr& /*propertyName*/,
                                                          const PropertyPtr& /*property*/,
                                                          const BaseObjectPtr& /*value*/)
{
    return nullptr;
}

END_NAMESPACE_OPENDAQ_TEMPLATES
