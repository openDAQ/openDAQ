/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coreobjects/property_object_impl.h>
#include <config_protocol/config_client_object_impl.h>
#include <config_protocol/config_client_function_impl.h>
#include <config_protocol/config_client_procedure_impl.h>
#include <config_protocol/config_client_object.h>
#include <opendaq/deserialize_component_ptr.h>
#include <config_protocol/config_protocol_deserialize_context.h>
#include <opendaq/custom_log.h>
#include <config_protocol/config_protocol_deserialize_context_impl.h>
#include <opendaq/context_factory.h>
#include <config_protocol/errors.h>

namespace daq::config_protocol
{

template <typename Impl>
class ConfigClientPropertyObjectBaseImpl;

template <class Impl>
class ConfigClientPropertyObjectBaseImpl : public ConfigClientObjectImpl, public Impl
{
public:
    template <class ... Args>
    explicit ConfigClientPropertyObjectBaseImpl(
        const ConfigProtocolClientCommPtr& configProtocolClientComm,
        const std::string& remoteGlobalId,
        const Args& ... args);

    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC getPropertyValue(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC getPropertySelectionValue(IString* propertyName, IBaseObject** value) override;
    ErrCode INTERFACE_FUNC clearPropertyValue(IString* propertyName) override;
    ErrCode INTERFACE_FUNC clearProtectedPropertyValue(IString* propertyName) override;
    ErrCode INTERFACE_FUNC getProperty(IString* propertyName, IProperty** value) override;
    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;
    ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IString* propertyName, IEvent** event) override;
    ErrCode INTERFACE_FUNC getOnPropertyValueRead(IString* propertyName, IEvent** event) override;
    ErrCode INTERFACE_FUNC getVisibleProperties(IList** properties) override;
    ErrCode INTERFACE_FUNC hasProperty(IString* propertyName, Bool* hasProperty) override;
    ErrCode INTERFACE_FUNC getAllProperties(IList** properties) override;
    ErrCode INTERFACE_FUNC setPropertyOrder(IList* orderedPropertyNames) override;

    ErrCode INTERFACE_FUNC beginUpdate() override;
    ErrCode INTERFACE_FUNC endUpdate() override;

    ErrCode INTERFACE_FUNC updateInternal(ISerializedObject* obj, IBaseObject* context) override;
    ErrCode INTERFACE_FUNC update(ISerializedObject* obj, IBaseObject* config) override;

    ErrCode INTERFACE_FUNC complete() override;

    ErrCode INTERFACE_FUNC getRemoteGlobalId(IString** remoteGlobalId) override;
    ErrCode INTERFACE_FUNC setRemoteGlobalId(IString* remoteGlobalId) override;
    ErrCode INTERFACE_FUNC handleRemoteCoreEvent(IComponent* sender, ICoreEventArgs* args) override;
    ErrCode INTERFACE_FUNC remoteUpdate(ISerializedObject* serialized) override;
    ErrCode INTERFACE_FUNC setRemoteUpdating(Bool remoteUpdating) override;

protected:
    bool deserializationComplete;

    virtual void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args);
    virtual void onRemoteUpdate(const SerializedObjectPtr& serialized);
    void cloneAndSetChildPropertyObject(const PropertyPtr& prop) override;

/*
    void beginApplyUpdate() override;
    void endApplyUpdate() override;
    */

    bool remoteUpdating;

private:
    BaseObjectPtr getValueFromServer(const StringPtr& propName, bool& setValue);

    void updateProperties(const SerializedObjectPtr& serObj);
    void updatePropertyValues(const SerializedObjectPtr& serObj);
    void propertyValueChanged(const CoreEventArgsPtr& args);
    void propertyObjectUpdateEnd(const CoreEventArgsPtr& args);
    void propertyAdded(const CoreEventArgsPtr& args);
    void propertyRemoved(const CoreEventArgsPtr& args);
    PropertyObjectPtr getObjectAtPath(const CoreEventArgsPtr& args);
    BaseObjectPtr getFullPropName(const std::string& propName) const;

    void checkCanSetPropertyValue(const StringPtr& propName);
    bool isBasePropertyObject(const PropertyObjectPtr& propObj);

    std::string getPath();
    void applyUpdatingPropsAndValuesProtocolVer0();
};

class ConfigClientPropertyObjectImpl : public ConfigClientPropertyObjectBaseImpl<GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>
{
public:
    using Super = ConfigClientPropertyObjectBaseImpl<GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>;

    ConfigClientPropertyObjectImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                   const std::string& remoteGlobalId,
                                   const TypeManagerPtr& manager,
                                   const StringPtr& className);

    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC clearPropertyValue(IString* propertyName) override;
    ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;
    ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) override;

    ErrCode INTERFACE_FUNC beginUpdate() override;
    ErrCode INTERFACE_FUNC endUpdate() override;

    ErrCode INTERFACE_FUNC deserializeValues(ISerializedObject* serializedObject,
                                             IBaseObject* context,
                                             IFunction* callbackFactory) override;
    ErrCode INTERFACE_FUNC complete() override;
    ErrCode INTERFACE_FUNC getDeserializedParameter(IString* parameter, IBaseObject** value) override;

    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    void unfreeze();
};

class ScopedRemoteUpdate
{
public:
    ScopedRemoteUpdate(const ScopedRemoteUpdate&) = delete;
    ScopedRemoteUpdate(ScopedRemoteUpdate&&) = delete;

    explicit ScopedRemoteUpdate(const PropertyObjectPtr& obj)
        : obj(obj)
    {
        checkErrorInfo(obj.asPtr<IConfigClientObject>(true)->setRemoteUpdating(True));
    }

    ScopedRemoteUpdate operator=(const ScopedRemoteUpdate&) = delete;
    ScopedRemoteUpdate operator=(ScopedRemoteUpdate&&) = delete;

    ~ScopedRemoteUpdate()
    {
        checkErrorInfo(obj.asPtr<IConfigClientObject>(true)->setRemoteUpdating(False));
    }

private:
    BaseObjectPtr obj;
};


template <class Impl>
template <class ... Args>
ConfigClientPropertyObjectBaseImpl<Impl>::ConfigClientPropertyObjectBaseImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                             const std::string& remoteGlobalId,
                                                                             const Args& ... args)
    : ConfigClientObjectImpl(configProtocolClientComm, remoteGlobalId)
    , Impl(args ...)
    , deserializationComplete(false)
    , remoteUpdating(false)
{
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

//    if (this->updateCount > 0)
//        return Impl::setPropertyValue(propertyName, value);

    const auto propertyNamePtr = StringPtr::Borrow(propertyName);
    const auto valuePtr = BaseObjectPtr::Borrow(value);
    return daqTry(
        [this, &propertyNamePtr, &valuePtr]()
        {
            checkCanSetPropertyValue(propertyNamePtr);
            const auto fullPropName = getFullPropName(propertyNamePtr);
            clientComm->setPropertyValue(remoteGlobalId, fullPropName, valuePtr);
        });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    if (!deserializationComplete)
        return Impl::setProtectedPropertyValue(propertyName, value);

    const auto propertyNamePtr = StringPtr::Borrow(propertyName);
    const auto valuePtr = BaseObjectPtr::Borrow(value);
    return daqTry([this, &propertyNamePtr, &valuePtr]()
    {
        checkCanSetPropertyValue(propertyNamePtr);
        const auto fullPropName = getFullPropName(propertyNamePtr);
        clientComm->setProtectedPropertyValue(remoteGlobalId, fullPropName, valuePtr);
    });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);
    OPENDAQ_PARAM_NOT_NULL(value);

    const auto propertyNamePtr = StringPtr::Borrow(propertyName);

    return daqTry([this, &propertyNamePtr, &value]
    {
        // TODO: Refactor this
        PropertyPtr prop;
        checkErrorInfo(Impl::getProperty(propertyNamePtr, &prop));
        if (clientComm->getConnected() && (prop.getValueType() == ctFunc || prop.getValueType() == ctProc))
        {
            bool setValue;
            auto v = getValueFromServer(propertyNamePtr, setValue);

            if (setValue)
                Impl::setPropertyValue(propertyNamePtr, v);
            *value = v.detach();
            return OPENDAQ_SUCCESS;
        }

        return Impl::getPropertyValue(propertyNamePtr, value);
    });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getPropertySelectionValue(IString* propertyName, IBaseObject** value)
{
    return Impl::getPropertySelectionValue(propertyName, value);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::clearPropertyValue(IString* propertyName)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    const auto propertyNamePtr = StringPtr::Borrow(propertyName);
    return daqTry([this, &propertyNamePtr]()
    {
        clientComm->clearPropertyValue(remoteGlobalId, propertyNamePtr);
    });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::clearProtectedPropertyValue(IString* propertyName)
{
    if (!deserializationComplete)
        return Impl::clearProtectedPropertyValue(propertyName);

    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getProperty(IString* propertyName, IProperty** value)
{
    return Impl::getProperty(propertyName, value);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::addProperty(IProperty* property)
{
    if (!deserializationComplete)
    {
        return Impl::addProperty(property);
    }

    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::removeProperty(IString* propertyName)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueWrite(IString* propertyName, IEvent** event)
{
    return OPENDAQ_ERR_NATIVE_CLIENT_CALL_NOT_AVAILABLE;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueRead(IString* propertyName, IEvent** event)
{
    return OPENDAQ_ERR_NATIVE_CLIENT_CALL_NOT_AVAILABLE;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getVisibleProperties(IList** properties)
{
    return Impl::getVisibleProperties(properties);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::hasProperty(IString* propertyName, Bool* hasProperty)
{
    return Impl::hasProperty(propertyName, hasProperty);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getAllProperties(IList** properties)
{
    return Impl::getAllProperties(properties);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setPropertyOrder(IList* orderedPropertyNames)
{
    return OPENDAQ_ERR_INVALID_OPERATION;
}

template <class Impl>
ErrCode INTERFACE_FUNC ConfigClientPropertyObjectBaseImpl<Impl>::beginUpdate()
{
    return daqTry([this]()
        {
            std::string path{};
            if (this->path.assigned())
                path = this->path.toStdString();
            clientComm->beginUpdate(remoteGlobalId, path);
        });
}


template <class Impl>
ErrCode INTERFACE_FUNC ConfigClientPropertyObjectBaseImpl<Impl>::endUpdate()
{
    return daqTry([this]()
        {
            std::string path{};
            if (this->path.assigned())
                path = this->path.toStdString();
            clientComm->endUpdate(remoteGlobalId, path);
        });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::updateInternal(ISerializedObject* obj, IBaseObject* /* context */)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([this, &obj]()
    {
        StringPtr serialized;
        checkErrorInfo(obj->toJson(&serialized));
        clientComm->update(remoteGlobalId, serialized, this->path);
    });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::update(ISerializedObject* obj, IBaseObject* config)
{
   return updateInternal(obj, nullptr);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::complete()
{
    deserializationComplete = true;
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getRemoteGlobalId(IString** remoteGlobalId)
{
    OPENDAQ_PARAM_NOT_NULL(remoteGlobalId);

    *remoteGlobalId = String(this->remoteGlobalId).detach();
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setRemoteGlobalId(IString* remoteGlobalId)
{
    OPENDAQ_PARAM_NOT_NULL(remoteGlobalId);

    this->remoteGlobalId = StringPtr::Borrow(remoteGlobalId).toStdString();
    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::handleRemoteCoreEvent(IComponent* sender, ICoreEventArgs* args)
{
    OPENDAQ_PARAM_NOT_NULL(sender);
    OPENDAQ_PARAM_NOT_NULL(args);

    try
    {
        handleRemoteCoreObjectInternal(sender, args);
    }
    catch ([[maybe_unused]] const std::exception& e)
    {
        const auto loggerComponent = this->clientComm->getDaqContext().getLogger().getOrAddComponent("ConfigClient");
        StringPtr globalId;
        const auto argsPtr = CoreEventArgsPtr::Borrow(args);
        sender->getGlobalId(&globalId);
        LOG_D("Component {} failed to handle core event {}: {}", globalId, argsPtr.getEventName(), e.what());
    }
    catch (...)
    {
        const auto loggerComponent = this->clientComm->getDaqContext().getLogger().getOrAddComponent("ConfigClient");
        StringPtr globalId;
        const auto argsPtr = CoreEventArgsPtr::Borrow(args);
        sender->getGlobalId(&globalId);
        LOG_D("Component {} failed to handle core event {}", globalId, argsPtr.getEventName());
    }

    return OPENDAQ_SUCCESS;
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::remoteUpdate(ISerializedObject* serialized)
{
    return daqTry(
        [&serialized, this]
        {
            onRemoteUpdate(serialized);
            return OPENDAQ_SUCCESS;
        });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setRemoteUpdating(Bool remoteUpdating)
{
    this->remoteUpdating = remoteUpdating;
    return OPENDAQ_SUCCESS;
}

template <class Impl>
BaseObjectPtr ConfigClientPropertyObjectBaseImpl<Impl>::getValueFromServer(const StringPtr& propName, bool& setValue)
{
    PropertyPtr prop;
    Impl::getProperty(propName, &prop);
    setValue = false;
    switch (const auto vt = prop.getValueType())
    {
        case ctProc:
            return createWithImplementation<IProcedure, ConfigClientProcedureImpl>(clientComm, remoteGlobalId, this->path, propName);
        case ctFunc:
            return createWithImplementation<IFunction, ConfigClientFunctionImpl>(clientComm, remoteGlobalId, this->path, propName);
        case ctObject:
        {
            BaseObjectPtr obj;
            checkErrorInfo(Impl::getPropertyValue(propName, &obj));
            return obj;
        }
        default:
        {
            setValue = true;
            return clientComm->getPropertyValue(remoteGlobalId, this->getFullPropName(propName));
        }
    }
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::updateProperties(const SerializedObjectPtr& serObj)
{
    // Add/remove properties where needed

    const auto keyStr = String("properties");
    const auto hasKey = serObj.hasKey(keyStr);
    const PropertyObjectPtr thisPtr = this->template borrowPtr<PropertyObjectPtr>();

    if (!IsTrue(hasKey))
    {
        for (const auto& prop : thisPtr.getAllProperties())
            Impl::removeProperty(prop.getName());
        return;
    }
    
    const auto propertyList = serObj.readSerializedList(keyStr);
    const TypeManagerPtr typeManager = this->manager.getRef();
    std::unordered_set<std::string> serializedProps{};

    for (SizeT i = 0; i < propertyList.getCount(); i++)
    {
        const PropertyPtr prop = propertyList.readObject(typeManager);

        const auto propName = prop.getName();
        serializedProps.insert(propName);

        if (!thisPtr.hasProperty(propName))
            thisPtr.addProperty(prop);
    }

    for (const auto& prop : thisPtr.getAllProperties())
    {
        const auto propName = prop.getName();
        if (!serializedProps.count(propName))
            Impl::removeProperty(propName);
    }
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::updatePropertyValues(const SerializedObjectPtr& serObj)
{
    const auto hasKeyStr = String("propValues");
    const PropertyObjectPtr thisPtr = this->template borrowPtr<PropertyObjectPtr>();

    if (!serObj.hasKey(hasKeyStr))
    {
        for (const auto& prop : thisPtr.getAllProperties())
        {
            const auto propInternal = prop.asPtrOrNull<IPropertyInternal>(true);
            if (propInternal.assigned())
            {
                const auto valueTypeUnresolved = propInternal.getValueTypeUnresolved();
                if (propInternal.getReferencedPropertyUnresolved().assigned())
                    continue;
                if (valueTypeUnresolved == ctFunc || valueTypeUnresolved == ctProc)
                    continue;
            }

            checkErrorInfo(Impl::clearProtectedPropertyValue(prop.getName()));
        }

        return;
    }
    
    const TypeManagerPtr typeManager = this->manager.getRef();
    const auto propValues = serObj.readSerializedObject("propValues");
    const auto protectedPropObjPtr = thisPtr.asPtr<IPropertyObjectProtected>();

    for (const auto& prop : thisPtr.getAllProperties())
    {
        const auto propName = prop.getName();
        const auto propInternal = prop.asPtrOrNull<IPropertyInternal>(true);
        auto valueTypeUnresolved = ctUndefined;
        if (propInternal.assigned())
        {
            valueTypeUnresolved = propInternal.getValueTypeUnresolved();
            if (propInternal.getReferencedPropertyUnresolved().assigned())
                continue;
            if (valueTypeUnresolved == ctFunc || valueTypeUnresolved == ctProc)
                continue;
        }

        if (!propValues.hasKey(propName))
        {
            checkErrorInfo(Impl::clearProtectedPropertyValue(propName));
            continue;
        }

        if (valueTypeUnresolved == ctObject)
        {
            const ObjectPtr<IConfigClientObject> clientObj = thisPtr.getPropertyValue(propName);
            const auto childSerObj = propValues.readSerializedObject(propName);
            checkErrorInfo(clientObj->remoteUpdate(childSerObj));
        }
        else
        {
            const auto propValue = propValues.readObject(propName, typeManager);
            checkErrorInfo(Impl::setProtectedPropertyValue(propName, propValue));
        }
    }
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args)
{
    switch (static_cast<CoreEventId>(args.getEventId()))
    {
        case CoreEventId::PropertyValueChanged:
            propertyValueChanged(args);
            break;
        case CoreEventId::PropertyObjectUpdateEnd:
            propertyObjectUpdateEnd(args);
            break;
        case CoreEventId::PropertyAdded:
            propertyAdded(args);
            break;
        case CoreEventId::PropertyRemoved:
            propertyRemoved(args);
            break;
        default:
            break;
    }
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::onRemoteUpdate(const SerializedObjectPtr& serialized)
{
    updateProperties(serialized);
    updatePropertyValues(serialized);
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::cloneAndSetChildPropertyObject(const PropertyPtr& prop)
{
    const auto propPtrInternal = prop.asPtr<IPropertyInternal>();
    if (propPtrInternal.assigned() && propPtrInternal.getValueTypeUnresolved() == ctObject && prop.getDefaultValue().assigned())
    {
        const auto propName = prop.getName();
        const auto defaultValueObj = prop.getDefaultValue().asPtrOrNull<IPropertyObject>();
        if (!defaultValueObj.assigned())
            return;

        if (!isBasePropertyObject(defaultValueObj))
        {
            auto clonedValue = defaultValueObj.asPtr<IPropertyObjectInternal>(true).clone();
            this->writeLocalValue(propName, clonedValue);
            this->configureClonedObj(propName, clonedValue);
            return;
        }

        // This feels hacky...
        const auto serializer = JsonSerializer();
        defaultValueObj.serialize(serializer);

        const auto deserializer = JsonDeserializer();

        const auto deserializeContext = createWithImplementation<IComponentDeserializeContext, ConfigProtocolDeserializeContextImpl>(
            this->clientComm, this->remoteGlobalId, nullptr, nullptr, nullptr, nullptr, nullptr, this->manager.getRef());

        const PropertyObjectPtr clientPropObj =
            deserializer.deserialize(serializer.getOutput(),
                                     deserializeContext,
                                     [this](const StringPtr& typeId,
                                            const SerializedObjectPtr& object,
                                            const BaseObjectPtr& context,
                                            const FunctionPtr& factoryCallback)
                                     {
                                         return clientComm->deserializeConfigComponent(typeId, object, context, factoryCallback, nullptr);
                                     });
        
        const auto impl = dynamic_cast<ConfigClientPropertyObjectImpl*>(clientPropObj.getObject());
        if (impl == nullptr)
            throw InvalidStateException("Failed to cast to ConfigClientPropertyObjectImpl");
        impl->unfreeze();
        this->writeLocalValue(propName, clientPropObj);
        this->configureClonedObj(propName, clientPropObj);
    }
}

/*
template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::beginApplyUpdate()
{
    if (remoteUpdating)
        return Impl::beginApplyUpdate();

    clientComm->beginUpdate(remoteGlobalId, getPath());
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::endApplyUpdate()
{
    if (remoteUpdating)
        return Impl::endApplyUpdate();

    ListPtr<IDict> propsAndValuesEx;

    if (clientComm->getProtocolVersion() >= 1)
    {
        propsAndValuesEx = List<IDict>();

        auto ignoredProps = List<IString>();
        for (auto& item : this->updatingPropsAndValues)
        {
            auto itemEx = Dict<IString, IBaseObject>();
            itemEx.set("Name", String(item.first));
            itemEx.set("SetValue", Boolean(item.second.setValue));
            itemEx.set("ProtectedAccess", Boolean(item.second.protectedAccess));
            itemEx.set("Value", item.second.value);
            propsAndValuesEx.pushBack(itemEx);
        }
    }
    else
        applyUpdatingPropsAndValuesProtocolVer0();

    this->updatingPropsAndValues.clear();

    clientComm->endUpdate(remoteGlobalId, getPath(), propsAndValuesEx);
}*/

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::applyUpdatingPropsAndValuesProtocolVer0()
{
    for (const auto& item : this->updatingPropsAndValues)
    {
        if (item.second.setValue)
        {
            if (item.second.protectedAccess)
                clientComm->setProtectedPropertyValue(remoteGlobalId, item.first, item.second.value);
            else
                clientComm->setPropertyValue(remoteGlobalId, item.first, item.second.value);
        }
        else
            clientComm->clearPropertyValue(remoteGlobalId, item.first);
    }
}


template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::propertyValueChanged(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    StringPtr propName = params.get("Name");
    StringPtr path = params.get("Path");
    const auto val = params.get("Value");

    if (path != "")
    {
        const PropertyObjectPtr obj = this->objPtr.getPropertyValue(path);
        ScopedRemoteUpdate update(obj);

        if (val.assigned())
            obj.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(propName, val);
        else
            obj.asPtr<IPropertyObjectProtected>(true).clearProtectedPropertyValue(propName);
    }
    else
    {
        if (val.assigned())
            checkErrorInfo(Impl::setProtectedPropertyValue(propName, val));
        else
            checkErrorInfo(Impl::clearProtectedPropertyValue(propName));
    }
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::propertyObjectUpdateEnd(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    const PropertyObjectPtr obj = getObjectAtPath(args);
    
    const DictPtr<IString, IBaseObject> updatedProperties = params.get("UpdatedProperties");

    if (params.get("Path") != "")
    {
        ScopedRemoteUpdate update(obj);

        obj.beginUpdate();

        for (const auto& val : updatedProperties)
        {
            if (val.second.assigned())
                obj.asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue(val.first, val.second);
            else
                obj.asPtr<IPropertyObjectProtected>(true).clearProtectedPropertyValue(val.first);
        }

        obj.endUpdate();

    }
    else
    {
        ScopedRemoteUpdate update(obj);

        checkErrorInfo(Impl::beginUpdateInternal(false));

        for (const auto& val : updatedProperties)
        {
            if (val.second.assigned())
                checkErrorInfo(Impl::setProtectedPropertyValue(val.first, val.second));
            else
                checkErrorInfo(Impl::clearProtectedPropertyValue(val.first));
        }

        checkErrorInfo(Impl::endUpdateInternal(false));
    }
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::propertyAdded(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    const PropertyObjectPtr obj = getObjectAtPath(args);

    PropertyPtr prop = params.get("Property");
    if (obj.hasProperty(prop.getName()))
        return;

    if (params.get("Path") != "")
    {
        ScopedRemoteUpdate update(obj);
        obj.addProperty(prop);
    }
    else
        checkErrorInfo(Impl::addProperty(prop));
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::propertyRemoved(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    const PropertyObjectPtr obj = getObjectAtPath(args);
    
    const StringPtr propName = params.get("Name");
    if (!obj.hasProperty(propName))
        return;

    if (params.get("Path") != "")
    {
        ScopedRemoteUpdate update(obj);
        obj.removeProperty(propName);
    }
    else
        checkErrorInfo(Impl::removeProperty(propName));
}

template <class Impl>
PropertyObjectPtr ConfigClientPropertyObjectBaseImpl<Impl>::getObjectAtPath(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    const StringPtr path = params.get("Path");
    PropertyObjectPtr thisPtr = this->template borrowPtr<PropertyObjectPtr>();
    if (path != "")
        return thisPtr.getPropertyValue(path);
    return thisPtr;
}
template <class Impl>
BaseObjectPtr ConfigClientPropertyObjectBaseImpl<Impl>::getFullPropName(const std::string& propName) const
{
    auto fullPropName = propName;
    if (this->path.assigned() && this->path != "")
        fullPropName = this->path.toStdString() + "." + fullPropName;
    return fullPropName;
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::checkCanSetPropertyValue(const StringPtr& propName)
{
    const auto prop = this->objPtr.getProperty(propName);
    switch (const auto vt = prop.getValueType())
    {
        case ctProc:
        case ctFunc:
            throw InvalidOperationException("Cannot set remote function property");
        default:
            return;
    }
}

template <class Impl>
bool ConfigClientPropertyObjectBaseImpl<Impl>::isBasePropertyObject(const PropertyObjectPtr& propObj)
{
    return !propObj.supportsInterface<IServerCapabilityConfig>() 
            && !propObj.supportsInterface<IAddressInfo>();
}


template <class Impl>
std::string ConfigClientPropertyObjectBaseImpl<Impl>::getPath()
{
    std::string path{};
    if (this->path.assigned())
        path = this->path.toStdString();

    return path;
}

inline ConfigClientPropertyObjectImpl::ConfigClientPropertyObjectImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                      const std::string& remoteGlobalId,
                                                                      const TypeManagerPtr& manager,
                                                                      const StringPtr& className)
    : Super(configProtocolClientComm, remoteGlobalId, manager, className)
{
}

inline ErrCode ConfigClientPropertyObjectImpl::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (remoteUpdating)
        return GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>::setPropertyValue(propertyName, value);
    return ConfigClientPropertyObjectBaseImpl<GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>::setPropertyValue(propertyName, value);
}

inline ErrCode ConfigClientPropertyObjectImpl::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    if (remoteUpdating)
        return GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>::setProtectedPropertyValue(propertyName, value);
    return ConfigClientPropertyObjectBaseImpl<GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>::setProtectedPropertyValue(
        propertyName,
        value);
}

inline ErrCode ConfigClientPropertyObjectImpl::clearPropertyValue(IString* propertyName)
{
    if (remoteUpdating)
        return GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>::clearPropertyValue(propertyName);
    return ConfigClientPropertyObjectBaseImpl<GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>::clearPropertyValue(propertyName);
}

inline ErrCode ConfigClientPropertyObjectImpl::addProperty(IProperty* property)
{
    if (remoteUpdating)
        return GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>::addProperty(property);
    return ConfigClientPropertyObjectBaseImpl<GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>::addProperty(property);
}

inline ErrCode ConfigClientPropertyObjectImpl::removeProperty(IString* propertyName)
{
    if (remoteUpdating)
        return GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>::removeProperty(propertyName);
    return ConfigClientPropertyObjectBaseImpl<GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>::removeProperty(propertyName);
}

inline ErrCode ConfigClientPropertyObjectImpl::beginUpdate()
{
    if (remoteUpdating)
        return GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>::beginUpdate();
    return ConfigClientPropertyObjectBaseImpl<
        GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>::beginUpdate();
}

inline ErrCode ConfigClientPropertyObjectImpl::endUpdate()
{
    if (remoteUpdating)
        return GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>::endUpdate();
    return ConfigClientPropertyObjectBaseImpl<
        GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject, IDeserializeComponent>>::endUpdate();
}

inline ErrCode ConfigClientPropertyObjectImpl::deserializeValues(ISerializedObject* serializedObject,
                                                                 IBaseObject* context,
                                                                 IFunction* callbackFactory)
{
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientPropertyObjectImpl::complete()
{
    return OPENDAQ_SUCCESS;
}

inline ErrCode ConfigClientPropertyObjectImpl::getDeserializedParameter(IString* parameter, IBaseObject** value)
{
    return OPENDAQ_NOTFOUND;
}

inline ErrCode ConfigClientPropertyObjectImpl::Deserialize(ISerializedObject* serialized,
    IBaseObject* context,
    IFunction* factoryCallback,
    IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry(
        [&obj, &serialized, &context, &factoryCallback]()
        {
            const auto serializedPtr = SerializedObjectPtr::Borrow(serialized);
            if (!serializedPtr.assigned())
                throw ArgumentNullException("Serialized object not assigned");

            const auto contextPtr = BaseObjectPtr::Borrow(context);
            if (!contextPtr.assigned())
                throw ArgumentNullException("Deserialization context not assigned");

            const auto componentDeserializeContext = contextPtr.asPtrOrNull<IComponentDeserializeContext>(true);
            if (!componentDeserializeContext.assigned())
                throw InvalidParameterException("Invalid deserialization context");

            const auto factoryCallbackPtr = FunctionPtr::Borrow(factoryCallback);

            PropertyObjectPtr propObj = Super::DeserializePropertyObject(
                serializedPtr,
                contextPtr,
                factoryCallbackPtr,
                [&componentDeserializeContext, &factoryCallback](
                    const SerializedObjectPtr& serialized, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
                {
                    const auto ctx = componentDeserializeContext.asPtr<IConfigProtocolDeserializeContext>();
                    PropertyObjectPtr propObj = createWithImplementation<IPropertyObject, ConfigClientPropertyObjectImpl>(
                        ctx->getClientComm(), ctx->getRemoteGlobalId(), ctx->getTypeManager(), className);

                    return propObj;
                });

            const auto deserializeComponent = propObj.asPtr<IDeserializeComponent>(true);
            deserializeComponent.complete();

            *obj = propObj.detach();
        });
}

inline void ConfigClientPropertyObjectImpl::unfreeze()
{
    this->frozen = false;
}

}
