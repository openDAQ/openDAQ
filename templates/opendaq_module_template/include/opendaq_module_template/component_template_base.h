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
    virtual ~ComponentTemplateBase(){}

    ComponentPtr getComponent();
    std::unique_ptr<RecursiveConfigLockGuard> getRecursiveConfigLock();
    std::lock_guard<std::mutex> getAcquisitionLock();
    std::unique_lock<std::mutex> getUniqueLock();
    
    void removeComponentWithId(const FolderConfigPtr& parentFolder, const std::string& componentId) const;
    void removeComponent(const FolderConfigPtr& parentFolder, const ComponentPtr& component) const;
    
    template <class FBImpl, class... Params>
    FunctionBlockPtr createAndAddFunctionBlock(const std::string& fbId, Params&&... params) const;

    SignalPtr createAndAddSignal(const SignalParams& params) const;

    virtual void removed();

    virtual void initSignals(const FolderConfigPtr& signalsFolder);
    virtual void initFunctionBlocks(const FolderConfigPtr& fbFolder);

    virtual void initProperties();
    virtual void initTags(const TagsPrivatePtr& tags);
    virtual void initStatuses(const ComponentStatusContainerPrivatePtr& statusContainer);
    virtual ComponentAttributeConfig getAttributeConfig();
    
    virtual void start();

    virtual BaseObjectPtr onPropertyWrite(const PropertyEventArgs& args);
    virtual BaseObjectPtr onPropertyRead(const PropertyEventArgs& args);
    virtual void onEndUpdate(const UpdateEndArgs& args);

    LoggerComponentPtr loggerComponent;
    ContextPtr context;
    Type* componentImpl;
    ComponentPtr objPtr;
};

class AddableComponentTemplateBase
{
public:
    virtual ~AddableComponentTemplateBase() = default;
    virtual void applyConfig(const PropertyObjectPtr& config);
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
std::unique_ptr<RecursiveConfigLockGuard> ComponentTemplateBase<Type>::getRecursiveConfigLock()
{
    return componentImpl->getRecursiveConfigLock();
}

template <typename Type>
std::lock_guard<std::mutex> ComponentTemplateBase<Type>::getAcquisitionLock()
{
    return componentImpl->getAcquisitionLock();
}

template <typename Type>
std::unique_lock<std::mutex> ComponentTemplateBase<Type>::getUniqueLock()
{
    return componentImpl->getUniqueLock();
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

template <typename Type>
SignalPtr ComponentTemplateBase<Type>::createAndAddSignal(const SignalParams& params) const
{
    LOG_T("Adding signal {}", params.localId)
    auto signal = Signal(context, componentImpl->signals, params.localId, params.className);
    
    const auto componentPrivate = signal.template asPtr<IComponentPrivate>();
    componentPrivate.unlockAllAttributes();

    const auto attributes = params.attributes;
    if (params.descriptor.assigned())
        signal.setDescriptor(params.descriptor);
    if (attributes.domainSignal.value.assigned())
        signal.setDomainSignal(attributes.domainSignal.value);
    if (attributes.relatedSignals.value.assigned())
        signal.setDomainSignal(attributes.relatedSignals.value);


    signal.setDescription(attributes.description.value);
    signal.setName(attributes.name.value);
    signal.setActive(attributes.active.value);
    signal.setVisible(attributes.visible.value);
    signal.setPublic(attributes.isPublic.value);

    ListPtr<IString> lockedAttrs = List<IString>();
    if (attributes.description.readOnly)
        lockedAttrs.pushBack("Description");
    if (attributes.name.readOnly)
        lockedAttrs.pushBack("Name");
    if (attributes.active.readOnly)
        lockedAttrs.pushBack("Active");
    if (attributes.visible.readOnly)
        lockedAttrs.pushBack("Visible");
    if (attributes.isPublic.readOnly)
        lockedAttrs.pushBack("Public");
    if (attributes.domainSignal.readOnly)
        lockedAttrs.pushBack("DomainSignal");
    if (attributes.relatedSignals.readOnly)
        lockedAttrs.pushBack("RelatedSignals");

    componentPrivate.lockAttributes(lockedAttrs);

    componentImpl->signals.addItem(signal);
    return signal;
}

template <typename Type>
void ComponentTemplateBase<Type>::removed()
{
}

inline void AddableComponentTemplateBase::applyConfig(const PropertyObjectPtr& /*config*/)
{
}

inline void FunctionBlockTemplateBase::initInputPorts(const FolderConfigPtr& /*inputPortsFolder*/)
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
ComponentAttributeConfig ComponentTemplateBase<Type>::getAttributeConfig()
{
    return {};
}

template <typename Type>
void ComponentTemplateBase<Type>::start()
{
}

template <typename Type>
BaseObjectPtr ComponentTemplateBase<Type>::onPropertyWrite(const PropertyEventArgs& /*args*/)
{
    return nullptr;
}

template <typename Type>
BaseObjectPtr ComponentTemplateBase<Type>::onPropertyRead(const PropertyEventArgs& /*args*/)
{
    return nullptr;
}

template <typename Type>
void ComponentTemplateBase<Type>::onEndUpdate(const UpdateEndArgs& /*args*/)
{
}

END_NAMESPACE_OPENDAQ_TEMPLATES
