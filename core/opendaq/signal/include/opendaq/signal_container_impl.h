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
#include <opendaq/signal_config_ptr.h>
#include <opendaq/component_impl.h>
#include <opendaq/folder_factory.h>
#include <opendaq/component_factory.h>
#include <opendaq/signal_factory.h>
#include <opendaq/context_ptr.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/custom_log.h>
#include <opendaq/module_manager.h>

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
                               const StringPtr& className = nullptr);
    
    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC enableCoreEventTrigger() override;
    ErrCode INTERFACE_FUNC disableCoreEventTrigger() override;

protected:
    FolderConfigPtr signals;
    FolderConfigPtr functionBlocks;

    std::vector<ComponentPtr> components;
    std::unordered_set<std::string> defaultComponents;
    bool allowNonDefaultComponents;

    LoggerComponentPtr signalContainerLoggerComponent;

    SignalConfigPtr createAndAddSignal(const std::string& localId, const DataDescriptorPtr& descriptor = nullptr, bool visible = true);

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
    FolderConfigPtr addFolder(const std::string& localId, const FolderConfigPtr& parent = nullptr);

    ComponentPtr addComponent(const std::string& localId, const FolderConfigPtr& parent = nullptr);
    ComponentPtr addExistingComponent(const ComponentPtr& component, const FolderConfigPtr& parent = nullptr);
    void removeComponentById(const std::string& localId);

    void removed() override;

    void serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate) override;
    virtual void updateFunctionBlock(const std::string& fbId,
                                          const SerializedObjectPtr& serializedFunctionBlock);
    virtual void updateSignal(const std::string& sigId,
                              const SerializedObjectPtr& serializedSignal);

    template <class F>
    void updateFolder(const SerializedObjectPtr& obj, const std::string& folderType, const std::string& itemType, F&& f);

    void updateObject(const SerializedObjectPtr& obj) override;

    void deserializeCustomObjectValues(const SerializedObjectPtr& serializedObject,
                                       const BaseObjectPtr& context,
                                       const FunctionPtr& factoryCallback) override;

    void serializeFolder(const SerializerPtr& serializer, const FolderConfigPtr& folder, const std::string& id, bool forUpdate);

    template <class FolderPtr>
    void deserializeDefaultFolder(const SerializedObjectPtr& serializedObject,
                                  const BaseObjectPtr& context,
                                  const FunctionPtr& factoryCallback,
                                  FolderPtr& folder,
                                  const std::string& id);

    virtual bool clearFunctionBlocksOnUpdate();

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
                        const StringPtr& className = nullptr);

    virtual ErrCode INTERFACE_FUNC getItems(IList** items, ISearchFilter* searchFilter) override;
    ErrCode INTERFACE_FUNC getItem(IString* localId, IComponent** item) override;
    ErrCode INTERFACE_FUNC isEmpty(Bool* empty) override;
    ErrCode INTERFACE_FUNC hasItem(IString* localId, Bool* value) override;
};

template <class Intf, class... Intfs>
GenericSignalContainerImpl<Intf, Intfs...>::GenericSignalContainerImpl(const ContextPtr& context,
                                                                       const ComponentPtr& parent,
                                                                       const StringPtr& localId,
                                                                       const StringPtr& className)
    : Super(context, parent, localId, className)
    , allowNonDefaultComponents(false)
    , signalContainerLoggerComponent(
        context.getLogger().assigned() ? context.getLogger().getOrAddComponent("GenericSignalContainerImpl")
            : throw ArgumentNullException{"Logger not assigned!"})
{
    defaultComponents.insert("Sig");
    defaultComponents.insert("FB");

    signals = addFolder<ISignal>("Sig", nullptr);
    functionBlocks = addFolder<IFunctionBlock>("FB", nullptr);

    signals.asPtr<IComponentPrivate>().lockAllAttributes();
    functionBlocks.asPtr<IComponentPrivate>().lockAllAttributes();
}

template <class Intf, class ... Intfs>
ErrCode GenericSignalContainerImpl<Intf, Intfs...>::enableCoreEventTrigger()
{
    for (const auto& component : this->components)
    {
        const ErrCode err = component.template asPtr<IPropertyObjectInternal>()->enableCoreEventTrigger();
        if (OPENDAQ_FAILED(err))
            return err;
    }

    return ComponentImpl<Intf, Intfs...>::enableCoreEventTrigger();
}

template <class Intf, class ... Intfs>
ErrCode GenericSignalContainerImpl<Intf, Intfs...>::disableCoreEventTrigger()
{
    for (const auto& component : this->components)
    {
        const ErrCode err = component.template asPtr<IPropertyObjectInternal>()->disableCoreEventTrigger();
        if (OPENDAQ_FAILED(err))
            return err;
    }

    return ComponentImpl<Intf, Intfs...>::disableCoreEventTrigger();
}

template <class Intf, class ... Intfs>
SignalContainerImpl<Intf, Intfs...>::SignalContainerImpl(const ContextPtr& context,
                                                         const ComponentPtr& parent,
                                                         const StringPtr& localId,
                                                         const StringPtr& className)
    : Super(context, parent, localId, className)
{
}

template <class Intf, class ... Intfs>
ErrCode SignalContainerImpl<Intf, Intfs...>::getItems(IList** items, ISearchFilter* searchFilter)
{
    OPENDAQ_PARAM_NOT_NULL(items);

    if (searchFilter)
    {
        return daqTry([&]
        {
            *items = this->searchItems(searchFilter, this->components).detach();
            return OPENDAQ_SUCCESS;
        });
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

    return OPENDAQ_ERR_NOTFOUND;
}

template<class Intf, class ...Intfs>
SignalConfigPtr GenericSignalContainerImpl<Intf, Intfs ...>::createAndAddSignal(const std::string& localId, const DataDescriptorPtr& descriptor, bool visible)
{
    auto signal = Signal(this->context, signals, localId);
    if (descriptor.assigned())
        signal.setDescriptor(descriptor);

    if (!visible)
    {
        signal.template asPtr<IComponentPrivate>().unlockAttributes(List<IString>("visible"));
        signal.setVisible(visible);
        signal.template asPtr<IComponentPrivate>().lockAttributes(List<IString>("visible"));
    }

    addSignal(signal);
    return signal;
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::addSignal(const SignalPtr& signal)
{
    try
    {
        this->signals.addItem(signal);
    }
    catch (DuplicateItemException&)
    {
        throw DuplicateItemException("Signal with the same local ID already exists.");
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
        throw InvalidParameterException("Invalid parent of function block");

    try
    {
        functionBlocks.addItem(functionBlock);
    }
    catch (DuplicateItemException&)
    {
        throw DuplicateItemException("Function block with the same local ID already exists.");
    }
}

template <class Intf, class ... Intfs>
FunctionBlockPtr GenericSignalContainerImpl<Intf, Intfs...>::createAndAddNestedFunctionBlock(const StringPtr& typeId, const StringPtr& localId, const PropertyObjectPtr& config)
{
    ObjectPtr<IBaseObject> obj;
    this->context->getModuleManager(&obj);

    if (obj == nullptr)
        throw NotAssignedException{"Module Manager is not available in the Context."};

    IModuleManager* manager;
    obj->borrowInterface(IModuleManager::Id, reinterpret_cast<void**>(&manager));

    ListPtr<IBaseObject> modules;
    manager->getModules(&modules);

    for (const BaseObjectPtr& moduleObj : modules)
    {
        IModule* module;
        moduleObj->borrowInterface(IModule::Id, reinterpret_cast<void**>(&module));

        DictPtr<IString, IFunctionBlockType> types;
        ErrCode err = module->getAvailableFunctionBlockTypes(&types);
        if (OPENDAQ_FAILED(err))
        {
            StringPtr moduleName;
            module->getName(&moduleName);
            if (err == OPENDAQ_ERR_NOTIMPLEMENTED)
            {
                DAQLOGF_I(signalContainerLoggerComponent, "{}: GetAvailableFunctionBlockTypes not implemented", moduleName);
            }
            else
            {
                DAQLOGF_W(signalContainerLoggerComponent, "{}: GetAvailableFunctionBlockTypes failed", moduleName);
            }
        }

        if (!types.assigned())
            continue;
        if (!types.hasKey(typeId))
            continue;

        FunctionBlockPtr fb;
        err = module->createFunctionBlock(&fb, typeId, functionBlocks, localId, config);
        if (OPENDAQ_FAILED(err))
        {
            StringPtr moduleName;
            module->getName(&moduleName);
            DAQLOGF_W(signalContainerLoggerComponent, "{}: Function Block creation failed", moduleName);
            continue;
        }
        
        addNestedFunctionBlock(fb);
        return fb;
    }
    
    throw NotFoundException{"Function block with given uid is not available."};
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
        throw DuplicateItemException("Duplicate component");
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::validateComponentIsDefault(const std::string& localId)
{
    if (!this->defaultComponents.count(localId))
        throw InvalidParameterException("Non-default component cannot be added as child!");
}

template <class Intf, class ... Intfs>
template <class TItemInterface>
FolderConfigPtr GenericSignalContainerImpl<Intf, Intfs...>::addFolder(const std::string& localId, const FolderConfigPtr& parent)
{
    if (!parent.assigned())
    {
        validateComponentNotExists(localId);
        if (!allowNonDefaultComponents)
            validateComponentIsDefault(localId);

        auto folder = Folder<TItemInterface>(this->context, this->template thisPtr<ComponentPtr>(), localId);
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
    parent.addItem(folder);
    return folder;
}

template <class Intf, class ... Intfs>
ComponentPtr GenericSignalContainerImpl<Intf, Intfs...>::addComponent(const std::string& localId, const FolderConfigPtr& parent)
{
    if (!parent.assigned())
    {
        validateComponentNotExists(localId);
        if (!allowNonDefaultComponents)
            validateComponentIsDefault(localId);

        auto component = Component(this->context, this->template thisPtr<ComponentPtr>(), localId);
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
template <class FolderPtr>
void GenericSignalContainerImpl<Intf, Intfs...>::deserializeDefaultFolder(const SerializedObjectPtr& serializedObject,
                                                                          const BaseObjectPtr& context,
                                                                          const FunctionPtr& factoryCallback,
                                                                          FolderPtr& folder,
                                                                          const std::string& id)
{
    if (serializedObject.hasKey(id))
    {
        const auto deserializeContext = context.asPtr<IComponentDeserializeContext>(true);
        const auto newDeserializeContext = deserializeContext.clone(this->template borrowPtr<ComponentPtr>(), id);
        const FolderPtr newFolder = serializedObject.readObject(id, newDeserializeContext, factoryCallback);
        swapComponent(folder, newFolder);
    }
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::serializeCustomObjectValues(const SerializerPtr& serializer, bool forUpdate)
{
    Super::serializeCustomObjectValues(serializer, forUpdate);
    serializeFolder(serializer, signals, "Sig", forUpdate);
    serializeFolder(serializer, functionBlocks, "FB", forUpdate);
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::updateObject(const SerializedObjectPtr& obj)
{
    Super::updateObject(obj);

    if (obj.hasKey("FB"))
    {
        const auto fbFolder = obj.readSerializedObject("FB");
        fbFolder.checkObjectType("Folder");

        if (clearFunctionBlocksOnUpdate())
            functionBlocks.clear();

        updateFolder(fbFolder,
                     "Folder",
                     "FunctionBlock",
                     [this](const std::string& localId, const SerializedObjectPtr& obj)
                     {
                         updateFunctionBlock(localId, obj);
                     });
    }

    if (obj.hasKey("Sig"))
    {
        const auto sigFolder = obj.readSerializedObject("Sig");
        sigFolder.checkObjectType("Folder");

        updateFolder(sigFolder,
                     "Folder",
                     "Signal",
                     [this](const std::string& localId, const SerializedObjectPtr& obj)
                     { updateSignal(localId, obj); });
    }
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::deserializeCustomObjectValues(
    const SerializedObjectPtr& serializedObject,
    const BaseObjectPtr& context,
    const FunctionPtr& factoryCallback)
{
    Super::deserializeCustomObjectValues(serializedObject, context, factoryCallback);

    deserializeDefaultFolder(serializedObject, context, factoryCallback, this->signals, "Sig");
    deserializeDefaultFolder(serializedObject, context, factoryCallback, this->functionBlocks, "FB");
}

template <class Intf, class ... Intfs>
bool GenericSignalContainerImpl<Intf, Intfs...>::clearFunctionBlocksOnUpdate()
{
    return false;
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
void GenericSignalContainerImpl<Intf, Intfs...>::updateFunctionBlock(const std::string& /*fbId*/,
                                                                          const SerializedObjectPtr& /* serializedFunctionBlock */)
{

}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::updateSignal(const std::string& sigId,
                                                              const SerializedObjectPtr& serializedSignal)
{
    if (!signals.hasItem(sigId))
    {
        DAQLOGF_W(signalContainerLoggerComponent,
                  "Signal "
                  "{}"
                  "not found",
                  sigId);
        return;
    }

    const auto signal = signals.getItem(sigId);

    const auto updatableSignal = signal.template asPtr<IUpdatable>(true);

    updatableSignal.update(serializedSignal);
}

END_NAMESPACE_OPENDAQ
