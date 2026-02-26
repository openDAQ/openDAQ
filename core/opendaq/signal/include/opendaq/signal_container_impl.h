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
#include <opendaq/signal_config_ptr.h>
#include <opendaq/component_impl.h>
#include <opendaq/folder_factory.h>
#include <opendaq/component_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/context_ptr.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/custom_log.h>
#include <opendaq/module_manager.h>
#include <opendaq/module_manager_utils.h>

BEGIN_NAMESPACE_OPENDAQ
template <class Intf = IComponent, class ... Intfs>
class GenericSignalContainerImpl : public ComponentImpl<Intf, Intfs ...>
{
public:
    using Self = GenericSignalContainerImpl<Intf, Intfs...>;
    using Super = ComponentImpl<Intf, Intfs ...>;

    GenericSignalContainerImpl(const ContextPtr& context,
                               const ComponentPtr& parent,
                               const StringPtr& localId,
                               const StringPtr& className = nullptr,
                               const StringPtr& name = nullptr);
    
    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC enableCoreEventTrigger() override;
    ErrCode INTERFACE_FUNC disableCoreEventTrigger() override;

    // IComponentPrivate
    ErrCode INTERFACE_FUNC updateOperationMode(OperationModeType modeType) override;

protected:
    FolderConfigPtr signals;
    FolderConfigPtr functionBlocks;

    std::vector<ComponentPtr> components;
    std::unordered_set<std::string> defaultComponents;
    bool allowNonDefaultComponents;

    LoggerComponentPtr signalContainerLoggerComponent;

    SignalConfigPtr createAndAddSignal(const std::string& localId,
                                       const DataDescriptorPtr& descriptor = nullptr,
                                       bool visible = true,
                                       bool isPublic = true,
                                       const PermissionsPtr& permissions = nullptr,
                                       LockingStrategy lockingStrategy = LockingStrategy::OwnLock);

    void addSignal(const SignalPtr& signal);
    void removeSignal(const SignalConfigPtr& signal);

    void addNestedFunctionBlock(const FunctionBlockPtr& functionBlock);
    void removeNestedFunctionBlock(const FunctionBlockPtr& functionBlock);
    FunctionBlockPtr createAndAddNestedFunctionBlock(const StringPtr& typeId,
                                                     const StringPtr& localId,
                                                     const PropertyObjectPtr& config = nullptr);

    void validateComponentNotExists(const std::string& localId);
    void validateComponentIsDefault(const std::string& localId);

    template <class TItemInterface = IComponent>
    FolderConfigPtr addFolder(const std::string& localId,
                              const FolderConfigPtr& parent = nullptr,
                              LockingStrategy lockingStrategy = LockingStrategy::OwnLock);

    ComponentPtr addComponent(const std::string& localId,
                              const FolderConfigPtr& parent = nullptr,
                              LockingStrategy lockingStrategy = LockingStrategy::OwnLock);

    ComponentPtr addExistingComponent(const ComponentPtr& component, const FolderConfigPtr& parent = nullptr);
    void removeComponentById(const std::string& localId);

    void removed() override;

    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;
    virtual void updateFunctionBlock(const std::string& fbId,
                                     const SerializedObjectPtr& serializedFunctionBlock,
                                     const BaseObjectPtr& context);
    virtual void updateSignal(const std::string& sigId,
                              const SerializedObjectPtr& serializedSignal,
                              const BaseObjectPtr& context);

    template <class F>
    void updateFolder(const SerializedObjectPtr& obj, const std::string& folderType, const std::string& itemType, F&& f);

    void updateObject(const SerializedObjectPtr& obj, const BaseObjectPtr& context) override;
    void onUpdatableUpdateEnd(const BaseObjectPtr& context) override;

    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    void serializeFolder(const SerializerPtr& serializer, const FolderConfigPtr& folder, const std::string& id, bool forUpdate);

    template <class Interface, class FolderPtr>
    void deserializeDefaultFolder(const SerializedObjectPtr& serializedObject,
                                  const BaseObjectPtr& context,
                                  const FunctionPtr& factoryCallback,
                                  FolderPtr& folder,
                                  const std::string& id);

    ErrCode getAvailableFunctionBlockTypesInternal(IDict** functionBlockTypes);
    ErrCode addFunctionBlockInternal(IFunctionBlock** functionBlock, IString* typeId, IPropertyObject* config = nullptr);
    ErrCode removeFunctionBlockInternal(IFunctionBlock* functionBlock);

    virtual bool clearFunctionBlocksOnUpdate();
    virtual DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes();
    virtual FunctionBlockPtr onAddFunctionBlock(const StringPtr& typeId, const PropertyObjectPtr& config);
    virtual void onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock);

    void callBeginUpdateOnChildren() override;
    void callEndUpdateOnChildren() override;

private:
    template <class Component>
    void swapComponent(Component& origComponent, const Component& newComponent);
};

template <class Intf = IComponent, class... Intfs>
class SignalContainerImpl : public GenericSignalContainerImpl<Intf, Intfs...>
{
public:
    using Self = SignalContainerImpl<Intf, Intfs...>;
    using Super = GenericSignalContainerImpl<Intf, Intfs...>;

    SignalContainerImpl(const ContextPtr& context,
                        const ComponentPtr& parent,
                        const StringPtr& localId,
                        const StringPtr& className = nullptr,
                        const StringPtr& name = nullptr);
    
protected:
    void notifyActiveChanged() override;

    virtual ErrCode INTERFACE_FUNC getItems(IList** items, ISearchFilter* searchFilter) override;
    ErrCode INTERFACE_FUNC getItem(IString* localId, IComponent** item) override;
    ErrCode INTERFACE_FUNC isEmpty(Bool* empty) override;
    ErrCode INTERFACE_FUNC hasItem(IString* localId, Bool* value) override;
};

template <class Intf, class... Intfs>
GenericSignalContainerImpl<Intf, Intfs...>::GenericSignalContainerImpl(const ContextPtr& context,
                                                                       const ComponentPtr& parent,
                                                                       const StringPtr& localId,
                                                                       const StringPtr& className,
                                                                       const StringPtr& name)
    : Super(context, parent, localId, className, name)
    , allowNonDefaultComponents(false)
    , signalContainerLoggerComponent(
        context.getLogger().assigned() ? context.getLogger().getOrAddComponent("GenericSignalContainerImpl")
            : throw ArgumentNullException{"Logger not assigned!"})
{
    defaultComponents.insert("Sig");
    defaultComponents.insert("FB");

    signals = addFolder<ISignal>("Sig", nullptr, LockingStrategy::ForwardOwnerLockOwn);
    functionBlocks = addFolder<IFunctionBlock>("FB", nullptr, LockingStrategy::ForwardOwnerLockOwn);

    signals.asPtr<IComponentPrivate>().lockAllAttributes();
    functionBlocks.asPtr<IComponentPrivate>().lockAllAttributes();
    signals.asPtr<IComponentPrivate>().unlockAttributes(List<IString>("Active"));
    functionBlocks.asPtr<IComponentPrivate>().unlockAttributes(List<IString>("Active"));
}

template <class Intf, class ... Intfs>
ErrCode GenericSignalContainerImpl<Intf, Intfs...>::enableCoreEventTrigger()
{
    for (const auto& component : this->components)
    {
        const ErrCode err = component.template asPtr<IPropertyObjectInternal>()->enableCoreEventTrigger();
        OPENDAQ_RETURN_IF_FAILED(err);
    }

    return ComponentImpl<Intf, Intfs...>::enableCoreEventTrigger();
}

template <class Intf, class ... Intfs>
ErrCode GenericSignalContainerImpl<Intf, Intfs...>::disableCoreEventTrigger()
{
    for (const auto& component : this->components)
    {
        const ErrCode err = component.template asPtr<IPropertyObjectInternal>()->disableCoreEventTrigger();
        OPENDAQ_RETURN_IF_FAILED(err);
    }

    return ComponentImpl<Intf, Intfs...>::disableCoreEventTrigger();
}

template <class Intf, class ... Intfs>
SignalContainerImpl<Intf, Intfs...>::SignalContainerImpl(const ContextPtr& context,
                                                         const ComponentPtr& parent,
                                                         const StringPtr& localId,
                                                         const StringPtr& className,
                                                         const StringPtr& name)
    : Super(context, parent, localId, className, name)
{
}

template <class Intf, class ... Intfs>
void SignalContainerImpl<Intf, Intfs...>::notifyActiveChanged()
{
    this->notifyItemsActiveChanged(this->components);
}

template <class Intf, class ... Intfs>
ErrCode SignalContainerImpl<Intf, Intfs...>::getItems(IList** items, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(items);

    if (searchFilter)
    {
        const ErrCode errCode = daqTry([&]
        {
            *items = this->searchItems(searchFilter, this->components).detach();
            return OPENDAQ_SUCCESS;
        });
        OPENDAQ_RETURN_IF_FAILED(errCode);
        return errCode;
    }

    auto itemList = List<IComponent>();
    for (const auto& component : this->components)
        if (component.getVisible())
            itemList.pushBack(component);

    *items = itemList.detach();
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode SignalContainerImpl<Intf, Intfs...>::isEmpty(Bool* empty)
{
    OPENDAQ_PARAM_NOT_NULL(empty);

    *empty = False;

    return OPENDAQ_SUCCESS;
}

template <class Intf, class ... Intfs>
ErrCode SignalContainerImpl<Intf, Intfs...>::hasItem(IString* localId, Bool* value)
{
    OPENDAQ_PARAM_NOT_NULL(localId);
    OPENDAQ_PARAM_NOT_NULL(value);

    auto localIdStr = StringPtr::Borrow(localId).toStdString();
    for (const auto& component : this->components)
    {
        if (component.getLocalId().toStdString() == localIdStr)
        {
            *value = True;
            return OPENDAQ_SUCCESS;
        }
    }

    *value = False;
    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
ErrCode SignalContainerImpl<Intf, Intfs...>::getItem(IString* localId, IComponent** item)
{
    OPENDAQ_PARAM_NOT_NULL(localId);
    OPENDAQ_PARAM_NOT_NULL(item);

    auto localIdStr = StringPtr::Borrow(localId).toStdString();
    for (const auto& component : this->components)
    {
        if (component.getLocalId().toStdString() == localIdStr)
        {
            *item = component.addRefAndReturn();
            return OPENDAQ_SUCCESS;
        }
    }

    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND);
}

template<class Intf, class ...Intfs>
SignalConfigPtr GenericSignalContainerImpl<Intf, Intfs...>::createAndAddSignal(const std::string& localId,
                                                                               const DataDescriptorPtr& descriptor,
                                                                               bool visible,
                                                                               bool isPublic,
                                                                               const PermissionsPtr& permissions,
                                                                               LockingStrategy lockingStrategy)
{
    SignalConfigPtr signal = Signal(this->context, signals, localId);
    if (lockingStrategy != LockingStrategy::OwnLock)
        signal.asPtr<IPropertyObjectInternal>().setLockingStrategy(lockingStrategy);

    if (descriptor.assigned())
        signal.setDescriptor(descriptor);

    if (!visible)
    {
        signal.template asPtr<IComponentPrivate>().unlockAttributes(List<IString>("Visible"));
        signal.setVisible(visible);
        signal.template asPtr<IComponentPrivate>().lockAttributes(List<IString>("Visible"));
    }

    signal.setPublic(isPublic);

    if (permissions.assigned())
        signal.getPermissionManager().setPermissions(permissions);

    addSignal(signal);
    return signal;
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::addSignal(const SignalPtr& signal)
{
    if (signal.getParent() != this->signals)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid parent of signal");

    try
    {
        this->signals.addItem(signal);
    }
    catch (DuplicateItemException&)
    {
        DAQ_THROW_EXCEPTION(DuplicateItemException, "Signal with the same local ID already exists.");
    }
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs ...>::removeSignal(const SignalConfigPtr& signal)
{
    signals.removeItem(signal);
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::addNestedFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    if (functionBlock.getParent() != functionBlocks)
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Invalid parent of function block");

    try
    {
        functionBlocks.addItem(functionBlock);
    }
    catch (DuplicateItemException&)
    {
        DAQ_THROW_EXCEPTION(DuplicateItemException, "Function block with the same local ID already exists.");
    }
}

template <class Intf, class... Intfs>
FunctionBlockPtr GenericSignalContainerImpl<Intf, Intfs...>::createAndAddNestedFunctionBlock(const StringPtr& typeId,
                                                                                             const StringPtr& localId,
                                                                                             const PropertyObjectPtr& config)
{
    ObjectPtr<IBaseObject> obj;
    this->context->getModuleManager(&obj);

    if (obj == nullptr)
        DAQ_THROW_EXCEPTION(NotAssignedException, "Module Manager is not available in the Context.");

    IModuleManagerUtils* managerUtils;
    obj->borrowInterface(IModuleManagerUtils::Id, reinterpret_cast<void**>(&managerUtils));
    
    FunctionBlockPtr fb;

    checkErrorInfo(managerUtils->createFunctionBlock(&fb, typeId, functionBlocks, config, localId));

    if (fb.assigned())
    {
        addNestedFunctionBlock(fb);
    }
    return fb;
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::removeNestedFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    functionBlocks.removeItem(functionBlock);
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::validateComponentNotExists(const std::string& localId)
{
    auto it = std::find_if(this->components.begin(),
                           this->components.end(),
                           [&localId](const ComponentPtr& component) { return component.getLocalId().toStdString() == localId; });

    if (it != this->components.end())
        DAQ_THROW_EXCEPTION(DuplicateItemException, "Duplicate component");
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::validateComponentIsDefault(const std::string& localId)
{
    if (!this->defaultComponents.count(localId))
        DAQ_THROW_EXCEPTION(InvalidParameterException, "Non-default component cannot be added as child!");
}

template <class Intf, class... Intfs>
template <class TItemInterface>
FolderConfigPtr GenericSignalContainerImpl<Intf, Intfs...>::addFolder(const std::string& localId,
                                                                      const FolderConfigPtr& parent,
                                                                      LockingStrategy lockingStrategy)
{
    if (!parent.assigned())
    {
        validateComponentNotExists(localId);
        if (!allowNonDefaultComponents)
            validateComponentIsDefault(localId);

        auto folder = Folder<TItemInterface>(this->context, this->template thisPtr<ComponentPtr>(), localId);
        if (lockingStrategy != LockingStrategy::OwnLock)
            folder.template asPtr<IPropertyObjectInternal>().setLockingStrategy(lockingStrategy);

        this->components.push_back(folder);

        if (!this->coreEventMuted && this->coreEvent.assigned())
        {
            const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                    CoreEventId::ComponentAdded,
                    Dict<IString, IBaseObject>({{"Component", folder}}));
            
            this->triggerCoreEvent(args);
            folder.template asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        }

        return folder;
    }

    auto folder = Folder(this->context, parent, localId);
    if (lockingStrategy != LockingStrategy::OwnLock)
        folder.template asPtr<IPropertyObjectInternal>().setLockingStrategy(lockingStrategy);

    parent.addItem(folder);
    return folder;
}

template <class Intf, class... Intfs>
ComponentPtr GenericSignalContainerImpl<Intf, Intfs...>::addComponent(const std::string& localId,
                                                                      const FolderConfigPtr& parent,
                                                                      LockingStrategy lockingStrategy)
{
    if (!parent.assigned())
    {
        validateComponentNotExists(localId);
        if (!allowNonDefaultComponents)
            validateComponentIsDefault(localId);

        auto component = Component(this->context, this->template thisPtr<ComponentPtr>(), localId);
        if (lockingStrategy != LockingStrategy::OwnLock)
            component.template asPtr<IPropertyObjectInternal>().setLockingStrategy(lockingStrategy);

        this->components.push_back(component);

        if (!this->coreEventMuted && this->coreEvent.assigned())
        {
            const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                    CoreEventId::ComponentAdded,
                    Dict<IString, IBaseObject>({{"Component", component}}));
            
            this->triggerCoreEvent(args);
            component.template asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        }

        return component;
    }

    auto component = Component(this->context, parent, localId);
    if (lockingStrategy != LockingStrategy::OwnLock)
        component.template asPtr<IPropertyObjectInternal>().setLockingStrategy(lockingStrategy);

    parent.addItem(component);
    return component;
}

template <class Intf, class ... Intfs>
ComponentPtr GenericSignalContainerImpl<Intf, Intfs...>::addExistingComponent(const ComponentPtr& component, const FolderConfigPtr& parent)
{
    if (!parent.assigned())
    {
        validateComponentNotExists(component.getLocalId());
        if (!allowNonDefaultComponents)
            validateComponentIsDefault(component.getLocalId());

        this->components.push_back(component);

        if (!this->coreEventMuted && this->coreEvent.assigned())
        {
            const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                    CoreEventId::ComponentAdded,
                    Dict<IString, IBaseObject>({{"Component", component}}));
            
            this->triggerCoreEvent(args);
            component.template asPtr<IPropertyObjectInternal>().enableCoreEventTrigger();
        }

        return component;
    }

    parent.addItem(component);
    return component;
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::removeComponentById(const std::string& localId)
{
    auto it = std::find_if(this->components.begin(), this->components.end(), [&localId](const ComponentPtr& comp) { return comp.getLocalId() == localId; });
    if (it != this->components.end())
    {
        (*it).template asPtr<IPropertyObjectInternal>().disableCoreEventTrigger();
        (*it).remove();
        this->components.erase(it);

        if (!this->coreEventMuted && this->coreEvent.assigned())
        {
            const auto args = createWithImplementation<ICoreEventArgs, CoreEventArgsImpl>(
                    CoreEventId::ComponentRemoved,
                    Dict<IString, IBaseObject>({{"Id", localId}}));
            
            this->triggerCoreEvent(args);
        }
    }
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::removed()
{
    for (const auto& component : this->components)
        component.remove();
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::serializeFolder(const SerializerPtr& serializer,
                                                                 const FolderConfigPtr& folder,
                                                                 const std::string& id,
                                                                 bool forUpdate)
{
    if (forUpdate)
    {
        if (!folder.isEmpty())
        {
            serializer.key(id);
            folder.asPtr<IUpdatable>(true).serializeForUpdate(serializer);
        }
    }
    else
    {
        serializer.key(id);
        folder.serialize(serializer);
    }
}

template <class Intf, class... Intfs>
template <class Component>
void GenericSignalContainerImpl<Intf, Intfs...>::swapComponent(Component& origComponent, const Component& newComponent)
{
    const auto it = std::find(components.begin(), components.end(), origComponent.template asPtr<IComponent>(false));
    *it = newComponent;
    origComponent = newComponent;
}

template <class Intf, class... Intfs>
template <class Interface, class FolderPtr>
void GenericSignalContainerImpl<Intf, Intfs...>::deserializeDefaultFolder(const SerializedObjectPtr& serializedObject,
                                                                          const BaseObjectPtr& context,
                                                                          const FunctionPtr& factoryCallback,
                                                                          FolderPtr& folder,
                                                                          const std::string& id)
{
    if (serializedObject.hasKey(id))
    {
        const auto deserializeContext = context.asPtr<IComponentDeserializeContext>(true);
        IntfID intfID = Interface::Id;
        const auto newDeserializeContext = deserializeContext.clone(this->template borrowPtr<ComponentPtr>(), id, &intfID);
        const FolderPtr newFolder = serializedObject.readObject(id, newDeserializeContext, factoryCallback);
        swapComponent(folder, newFolder);
    }
}

template <class Intf, class ... Intfs>
ErrCode GenericSignalContainerImpl<Intf, Intfs...>::getAvailableFunctionBlockTypesInternal(IDict** functionBlockTypes)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlockTypes);
    if (this->isComponentRemoved)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_COMPONENT_REMOVED);
    
    DictPtr<IString, IFunctionBlockType> dict;
    const ErrCode errCode = wrapHandlerReturn(this, &GenericSignalContainerImpl::onGetAvailableFunctionBlockTypes, dict);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *functionBlockTypes = dict.detach();
    return errCode;
}

template <class Intf, class... Intfs>
ErrCode GenericSignalContainerImpl<Intf, Intfs...>::addFunctionBlockInternal(IFunctionBlock** functionBlock,
                                                                             IString* typeId,
                                                                             IPropertyObject* config)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlock);
    OPENDAQ_PARAM_NOT_NULL(typeId);
    
    if (this->isComponentRemoved)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_COMPONENT_REMOVED);
    
    FunctionBlockPtr functionBlockPtr;
    const ErrCode errCode = wrapHandlerReturn(this, &Self::onAddFunctionBlock, functionBlockPtr, typeId, config);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *functionBlock = functionBlockPtr.detach();
    return errCode;
}

template <class Intf, class ... Intfs>
ErrCode GenericSignalContainerImpl<Intf, Intfs...>::removeFunctionBlockInternal(IFunctionBlock* functionBlock)
{
    OPENDAQ_PARAM_NOT_NULL(functionBlock);

    if (this->isComponentRemoved)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_COMPONENT_REMOVED);
    
    const auto fbPtr = FunctionBlockPtr::Borrow(functionBlock);
    const ErrCode errCode = wrapHandler(this, &Self::onRemoveFunctionBlock, fbPtr);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    return errCode;
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    Super::serializeCustomObjectValues(serializer, forUpdate);
    serializeFolder(serializer, signals, "Sig", forUpdate);
    serializeFolder(serializer, functionBlocks, "FB", forUpdate);
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::updateObject(const SerializedObjectPtr& obj, const BaseObjectPtr& context)
{
    Super::updateObject(obj, context);
    const auto availableTypes = onGetAvailableFunctionBlockTypes();
    if (clearFunctionBlocksOnUpdate())
    {
        for (const auto& fb : functionBlocks.getItems())
        {
            const auto typeId = fb.template asPtr<IFunctionBlock>().getFunctionBlockType().getId();
            if (availableTypes.hasKey(typeId))
            {
                onRemoveFunctionBlock(fb);
            }
            else
            {
                auto loggerComponent = signalContainerLoggerComponent;
                LOG_D("Update did not remove static function block with type ID {} and local ID {}", typeId, fb.getLocalId())
            }
        }
    }

    if (obj.hasKey("FB"))
    {
        const auto fbFolder = obj.readSerializedObject("FB");
        fbFolder.checkObjectType("Folder");

        updateFolder(fbFolder,
                     "Folder",
                     "FunctionBlock",
                     [this, &context](const std::string& localId, const SerializedObjectPtr& obj)
                     {
                         updateFunctionBlock(localId, obj, context);
                     });
    }

    if (obj.hasKey("Sig"))
    {
        const auto sigFolder = obj.readSerializedObject("Sig");
        sigFolder.checkObjectType("Folder");

        updateFolder(sigFolder,
                     "Folder",
                     "Signal",
                     [this, &context](const std::string& localId, const SerializedObjectPtr& obj)
                     { updateSignal(localId, obj, context); });
    }
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::onUpdatableUpdateEnd(const BaseObjectPtr& context)
{
    for (const auto& comp : components)
    {
        const auto updatable = comp.template asPtrOrNull<IUpdatable>();
        if (updatable.assigned())
            updatable.updateEnded(context);
    }
    Super::onUpdatableUpdateEnd(context);
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::deserializeCustomObjectValues(
    const SerializedObjectPtr& serializedObject,
    const BaseObjectPtr& context,
    const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);

    deserializeDefaultFolder<ISignal>(serializedObject, context, factoryCallback, this->signals, "Sig");
    deserializeDefaultFolder<IFunctionBlock>(serializedObject, context, factoryCallback, this->functionBlocks, "FB");
}

template <class Intf, class ... Intfs>
bool GenericSignalContainerImpl<Intf, Intfs...>::clearFunctionBlocksOnUpdate()
{
    return false;
}

template <class Intf, class ... Intfs>
DictPtr<IString, IFunctionBlockType> GenericSignalContainerImpl<Intf, Intfs...>::onGetAvailableFunctionBlockTypes()
{
    return Dict<IString, IFunctionBlockType>().detach();
}

template <class Intf, class ... Intfs>
FunctionBlockPtr GenericSignalContainerImpl<Intf, Intfs...>::onAddFunctionBlock(const StringPtr& /*typeId*/, const PropertyObjectPtr& /*config*/)
{
    DAQ_THROW_EXCEPTION(NotSupportedException, "Component does not support adding nested function blocks");
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::onRemoveFunctionBlock(const FunctionBlockPtr& functionBlock)
{
    auto types = onGetAvailableFunctionBlockTypes();
    auto typeId = functionBlock.getFunctionBlockType().getId();
    if (!types.hasKey(typeId))
        DAQ_THROW_EXCEPTION(InvalidOperationException, "Function block being removed is a static-type. Its type is not in the list of available function block types.");

    auto lock = this->getRecursiveConfigLock2();
    this->functionBlocks.removeItem(functionBlock);
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::callBeginUpdateOnChildren()
{
    Super::callBeginUpdateOnChildren();

    for (const auto& comp : components)
    {
        auto freezable = comp.template asPtrOrNull<IFreezable>(true);
        if (freezable.assigned() && freezable.isFrozen())
            continue;

        comp.beginUpdate();
    }
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::callEndUpdateOnChildren()
{
    for (const auto& comp : components)
    {
        auto freezable = comp.template asPtrOrNull<IFreezable>(true);
        if (freezable.assigned() && freezable.isFrozen())
            continue;

        comp.endUpdate();
    }

    Super::callEndUpdateOnChildren();
}

template <class Intf, class... Intfs>
ErrCode GenericSignalContainerImpl<Intf, Intfs...>::updateOperationMode(OperationModeType modeType)
{
    ErrCode errCode = Super::updateOperationMode(modeType);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    for (const auto& component : components)
    {
        auto componentPrivate = component.template asPtrOrNull<IComponentPrivate>(true);
        if (!componentPrivate.assigned())
            continue;

        errCode = componentPrivate->updateOperationMode(modeType);
        OPENDAQ_RETURN_IF_FAILED(errCode);
    }

    return OPENDAQ_SUCCESS;
}

template <class Intf, class... Intfs>
template <class F>
void GenericSignalContainerImpl<Intf, Intfs...>::updateFolder(const SerializedObjectPtr& obj,
                                                              const std::string& folderType,
                                                              const std::string& itemType,
                                                              F&& f)
{
    obj.checkObjectType(folderType);

    auto serializedItems = this->getSerializedItems(obj);
    for (const auto& serializedItem : serializedItems)
    {
        serializedItem.second.checkObjectType(itemType);
        f(serializedItem.first, serializedItem.second);
    }
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::updateFunctionBlock(const std::string& fbId,
                                                                     const SerializedObjectPtr& serializedFunctionBlock,
                                                                     const BaseObjectPtr& context)
{
    const auto availableTypes = onGetAvailableFunctionBlockTypes();
    UpdatablePtr updatableFb;
    if (!this->functionBlocks.hasItem(fbId))
    {
        auto typeId = serializedFunctionBlock.readString("typeId");
        if (!availableTypes.hasKey(typeId))
        {
            auto loggerComponent = signalContainerLoggerComponent;
            LOG_W("Failed to add missing FB with ID {} while updating parent FB with ID {}", fbId, this->localId)
            return;
        }

        PropertyObjectPtr config;
        if (serializedFunctionBlock.hasKey("ComponentConfig"))
            config = serializedFunctionBlock.readObject("ComponentConfig");
        else
            config = PropertyObject();

        if (!config.hasProperty("LocalId"))
            config.addProperty(StringProperty("LocalId", fbId));
        else
            config.setPropertyValue("LocalId", fbId);

        auto fb = onAddFunctionBlock(typeId, config);
        updatableFb = fb.template asPtr<IUpdatable>(true);
    }
    else
    {
        updatableFb = this->functionBlocks.getItem(fbId).template asPtr<IUpdatable>(true);
    }

    updatableFb.updateInternal(serializedFunctionBlock, context);
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::updateSignal(const std::string& sigId,
                                                              const SerializedObjectPtr& serializedSignal,
                                                              const BaseObjectPtr& context)
{
    auto contextPtr = context.asPtr<IComponentUpdateContext>(true);
    contextPtr.setSignalDependency(signals.getGlobalId() + "/" + sigId, this->globalId);

    if (!signals.hasItem(sigId))
        return;

    const auto signal = signals.getItem(sigId);

    const auto updatableSignal = signal.template asPtr<IUpdatable>(true);

    updatableSignal.updateInternal(serializedSignal, context);
}

END_NAMESPACE_OPENDAQ
