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

    GenericSignalContainerImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const StringPtr& className = nullptr);

protected:
    FolderConfigPtr signals;
    FolderConfigPtr functionBlocks;

    std::vector<ComponentPtr> components;
    std::unordered_set<std::string> defaultComponents;

    LoggerComponentPtr signalContainerLoggerComponent;

    SignalConfigPtr createAndAddSignal(const std::string& localId, const DataDescriptorPtr& descriptor = nullptr);
    void addSignal(const SignalPtr& signal);
    void removeSignal(const SignalConfigPtr& signal);

    void addNestedFunctionBlock(const FunctionBlockPtr& functionBlock);
    void removeNestedFunctionBlock(const FunctionBlockPtr& functionBlock);
    FunctionBlockPtr createAndAddNestedFunctionBlock(const StringPtr& typeId,
                                                     const StringPtr& localId,
                                                     const PropertyObjectPtr& config = nullptr);
    void validateComponentNotExists(const std::string& localId);

    template <class TItemInterface = IComponent>
    FolderConfigPtr addFolder(const std::string& localId, const FolderConfigPtr& parent = nullptr);

    ComponentPtr addComponent(const std::string& localId, const FolderConfigPtr& parent = nullptr);

    void removed() override;

    ErrCode serializeCustomValues(ISerializer* serializer) override;
    virtual void deserializeFunctionBlock(const std::string& fbId,
                                          const SerializedObjectPtr& serializedFunctionBlock);
    virtual void updateSignal(const std::string& sigId,
                              const SerializedObjectPtr& serializedSignal);

    template <class F>
    void updateFolder(const SerializedObjectPtr& obj, const std::string& folderType, const std::string& itemType, F&& f);

    void updateObject(const SerializedObjectPtr& obj) override;

    virtual bool clearFunctionBlocksOnUpdate();
};

template <class Intf = IComponent, class... Intfs>
class SignalContainerImpl : public GenericSignalContainerImpl<Intf, Intfs...>
{
public:
    using Self = SignalContainerImpl<Intf, Intfs...>;
    using Super = GenericSignalContainerImpl<Intf, Intfs...>;

    SignalContainerImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const StringPtr& className = nullptr);

    ErrCode INTERFACE_FUNC getItems(IList** items) override;
    ErrCode INTERFACE_FUNC getItem(IString* localId, IComponent** item) override;
    ErrCode INTERFACE_FUNC isEmpty(Bool* empty) override;
    ErrCode INTERFACE_FUNC hasItem(IString* localId, Bool* value) override;
};

template <class Intf, class... Intfs>
GenericSignalContainerImpl<Intf, Intfs...>::GenericSignalContainerImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const StringPtr& className)
    : Super(context, parent, localId, className)
    , signalContainerLoggerComponent(
        context.getLogger().assigned() ? context.getLogger().getOrAddComponent("GenericSignalContainerImpl")
                                       : throw ArgumentNullException{"Logger not assigned!"})
{
    signals = addFolder<ISignal>("Sig");
    functionBlocks = addFolder<IFunctionBlock>("FB");

    defaultComponents.insert("Sig");
    defaultComponents.insert("FB");
}

template <class Intf, class ... Intfs>
SignalContainerImpl<Intf, Intfs...>::SignalContainerImpl(const ContextPtr& context, const ComponentPtr& parent, const StringPtr& localId, const StringPtr& className)
    : Super(context, parent, localId, className)
{
}

template <class Intf, class ... Intfs>
ErrCode SignalContainerImpl<Intf, Intfs...>::getItems(IList** items)
{
    OPENDAQ_PARAM_NOT_NULL(items);

    auto itemList = List<IComponent>();
    for (const auto& component : this->components)
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

    return OPENDAQ_NOTFOUND;
}

template<class Intf, class ...Intfs>
SignalConfigPtr GenericSignalContainerImpl<Intf, Intfs ...>::createAndAddSignal(const std::string& localId, const DataDescriptorPtr& descriptor)
{
    auto signal = Signal(this->context, signals, localId);
    if (descriptor.assigned())
        signal.setDescriptor(descriptor);

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
template <class TItemInterface>
FolderConfigPtr GenericSignalContainerImpl<Intf, Intfs...>::addFolder(const std::string& localId, const FolderConfigPtr& parent)
{
    if (!parent.assigned())
    {
        validateComponentNotExists(localId);

        auto folder = Folder<TItemInterface>(this->context, this->template thisPtr<ComponentPtr>(), localId);
        this->components.push_back(folder);
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

        auto component = Component(this->context, this->template thisPtr<ComponentPtr>(), localId);
        this->components.push_back(component);
        return component;
    }

    auto component = Component(this->context, parent, localId);
    parent.addItem(component);
    return component;
}

template <class Intf, class ... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::removed()
{
    for (const auto& component : this->components)
        component.remove();
}

template <class Intf, class ... Intfs>
ErrCode GenericSignalContainerImpl<Intf, Intfs...>::serializeCustomValues(ISerializer* serializer)
{
    auto errCode = Super::serializeCustomValues(serializer);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    return daqTry(
        [&serializer, this]()
        {
            if (!signals.isEmpty())
            {
                serializer->key("sig");
                signals.serialize(serializer);
            }

            if (!functionBlocks.isEmpty())
            {
                serializer->key("fb");
                functionBlocks.serialize(serializer);
            }

            return OPENDAQ_SUCCESS;
        });
}

template <class Intf, class... Intfs>
void GenericSignalContainerImpl<Intf, Intfs...>::updateObject(const SerializedObjectPtr& obj)
{
    Super::updateObject(obj);

    if (obj.hasKey("fb"))
    {
        const auto fbFolder = obj.readSerializedObject("fb");
        fbFolder.checkObjectType("Folder");

        if (clearFunctionBlocksOnUpdate())
            functionBlocks.clear();

        updateFolder(fbFolder,
                     "Folder",
                     "FunctionBlock",
                     [this](const std::string& localId, const SerializedObjectPtr& obj)
                     {
                         deserializeFunctionBlock(localId, obj);
                     });
    }

    if (obj.hasKey("sig"))
    {
        const auto sigFolder = obj.readSerializedObject("sig");
        sigFolder.checkObjectType("Folder");

        updateFolder(sigFolder,
                     "Folder",
                     "Signal",
                     [this](const std::string& localId, const SerializedObjectPtr& obj)
                     { updateSignal(localId, obj); });
    }
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
void GenericSignalContainerImpl<Intf, Intfs...>::deserializeFunctionBlock(const std::string& /*fbId*/,
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
