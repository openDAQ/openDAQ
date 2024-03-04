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
#include <coreobjects/property_object_impl.h>
#include <config_protocol/config_client_object_impl.h>
#include <config_protocol/config_client_function_impl.h>
#include <config_protocol/config_client_procedure_impl.h>
#include <config_protocol/config_client_object.h>

#include "opendaq/custom_log.h"

namespace daq::config_protocol
{

template <typename Impl>
class ConfigClientPropertyObjectBaseImpl;

using ConfigClientPropertyObjectImpl = ConfigClientPropertyObjectBaseImpl<GenericPropertyObjectImpl<IPropertyObject, IConfigClientObject>>;

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

    ErrCode INTERFACE_FUNC complete() override;

    ErrCode INTERFACE_FUNC getRemoteGlobalId(IString** remoteGlobalId) override;
    ErrCode INTERFACE_FUNC setRemoteGlobalId(IString* remoteGlobalId) override;
    ErrCode INTERFACE_FUNC handleRemoteCoreEvent(IComponent* sender, ICoreEventArgs* args) override;

protected:
    bool deserializationComplete;

    virtual void handleRemoteCoreObjectInternal(const ComponentPtr& sender, const CoreEventArgsPtr& args);
    virtual void remoteUpdate(const SerializedObjectPtr& serialized);

private:
    BaseObjectPtr getValueFromServer(const StringPtr& propName, bool& setValue);

    void updateProperties(const SerializedObjectPtr& serObj);
    void updatePropertyValues(const SerializedObjectPtr& serObj);
    void propertyValueChanged(const CoreEventArgsPtr& args);
    void propertyObjectUpdateEnd(const CoreEventArgsPtr& args);
    void propertyAdded(const CoreEventArgsPtr& args);
    void propertyRemoved(const CoreEventArgsPtr& args);
    PropertyObjectPtr getObjectAtPath(const CoreEventArgsPtr& args);
};

template <class Impl>
template <class ... Args>
ConfigClientPropertyObjectBaseImpl<Impl>::ConfigClientPropertyObjectBaseImpl(const ConfigProtocolClientCommPtr& configProtocolClientComm,
                                                                             const std::string& remoteGlobalId,
                                                                             const Args& ... args)
    : ConfigClientObjectImpl(configProtocolClientComm, remoteGlobalId)
    , Impl(args ...)
    , deserializationComplete(false)
{
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    const auto propertyNamePtr = StringPtr::Borrow(propertyName);
    const auto valuePtr = BaseObjectPtr::Borrow(value);
    return daqTry(
        [this, &propertyNamePtr, &valuePtr]()
        {
            clientComm->setPropertyValue(remoteGlobalId, propertyNamePtr, valuePtr);
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
        clientComm->setProtectedPropertyValue(remoteGlobalId, propertyNamePtr, valuePtr);
    });
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);
    OPENDAQ_PARAM_NOT_NULL(value);

    const auto propertyNamePtr = StringPtr::Borrow(propertyName);

    return daqTry(
        [this, &propertyNamePtr, &value]()
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
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getProperty(IString* propertyName, IProperty** value)
{
    return Impl::getProperty(propertyName, value);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::addProperty(IProperty* property)
{
    if (!deserializationComplete)
        return Impl::addProperty(property);

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
    return Impl::getOnPropertyValueWrite(propertyName, event);
}

template <class Impl>
ErrCode ConfigClientPropertyObjectBaseImpl<Impl>::getOnPropertyValueRead(IString* propertyName, IEvent** event)
{
    return Impl::getOnPropertyValueRead(propertyName, event);
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
            clientComm->sendComponentCommand(remoteGlobalId, "BeginUpdate");
        });
}

template <class Impl>
inline ErrCode INTERFACE_FUNC ConfigClientPropertyObjectBaseImpl<Impl>::endUpdate()
{
    return daqTry([this]()
        {
            clientComm->sendComponentCommand(remoteGlobalId, "EndUpdate");
        });
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
BaseObjectPtr ConfigClientPropertyObjectBaseImpl<Impl>::getValueFromServer(const StringPtr& propName, bool& setValue)
{
    const auto prop = Impl::getUnboundProperty(propName);
    setValue = false;
    switch (const auto vt = prop.getValueType())
    {
        case ctProc:
            return createWithImplementation<IProcedure, ConfigClientProcedureImpl>(clientComm, remoteGlobalId, propName);
        case ctFunc:
            return createWithImplementation<IFunction, ConfigClientFunctionImpl>(clientComm, remoteGlobalId, propName);
        default:
            setValue = true;
            return clientComm->getPropertyValue(remoteGlobalId, propName);
    }
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::updateProperties(const SerializedObjectPtr& serObj)
{
    // Add/remove properties where needed

    const auto keyStr = String("properties");
    const auto hasKey = serObj.hasKey(keyStr);

    if (!IsTrue(hasKey))
        return;
    
    const PropertyObjectPtr thisPtr = this->template borrowPtr<PropertyObjectPtr>();
    const auto propertyList = serObj.readSerializedList(keyStr);
    const TypeManagerPtr typeManager = this->context.getTypeManager();
    std::unordered_set<std::string> serializedProps{};

    for (SizeT i = 0; i < propertyList.getCount(); i++)
    {
        const PropertyPtr prop = propertyList.readObject(typeManager);

        const auto propName = prop.getName();
        serializedProps.insert(propName);

        // TODO: Override addProperty to custom handle nested objects
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
    if (!serObj.hasKey(hasKeyStr))
        return;
    
    const PropertyObjectPtr thisPtr = this->template borrowPtr<PropertyObjectPtr>();
    const TypeManagerPtr typeManager = this->context.getTypeManager();
    const auto propValues = serObj.readSerializedObject("propValues");
    const auto protectedPropObjPtr = thisPtr.asPtr<IPropertyObjectProtected>();

    for (const auto& prop : thisPtr.getAllProperties())
    {
        const auto propName = prop.getName();

        const auto propInternal = prop.asPtrOrNull<IPropertyInternal>(true);
        if (propInternal.assigned())
        {
            if (propInternal.getReferencedPropertyUnresolved().assigned())
                continue;
            const auto valueTypeUnresolved = propInternal.getValueTypeUnresolved();
            if (valueTypeUnresolved == ctFunc || valueTypeUnresolved == ctProc || valueTypeUnresolved == ctObject)
                continue;
        }

        if (!propValues.hasKey(propName))
        {
            protectedPropObjPtr.clearProtectedPropertyValue(propName);
            continue;
        }
        
        const auto propValue = propValues.readObject(propName, typeManager);
        protectedPropObjPtr.setProtectedPropertyValue(propName, propValue);
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
void ConfigClientPropertyObjectBaseImpl<Impl>::remoteUpdate(const SerializedObjectPtr& serialized)
{
    updateProperties(serialized);
    updatePropertyValues(serialized);
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::propertyValueChanged(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    StringPtr propName = params.get("Name");
    StringPtr path = params.get("Path");
    if (path != "")
        propName = path + "." + propName;
    const auto val = params.get("Value");
    if (val.assigned())
        Impl::setProtectedPropertyValue(propName, val);
    else
        Impl::clearProtectedPropertyValue(propName);
}

template <class Impl>
void ConfigClientPropertyObjectBaseImpl<Impl>::propertyObjectUpdateEnd(const CoreEventArgsPtr& args)
{
    const auto params = args.getParameters();
    const PropertyObjectPtr obj = getObjectAtPath(args);
    
    const DictPtr<IString, IBaseObject> updatedProperties = params.get("UpdatedProperties");

    if (params.get("Path") != "")
    {
        auto objImpl = dynamic_cast<PropertyObjectImpl*>(obj.getObject());

        checkErrorInfo(objImpl->beginUpdate());

        for (const auto& val : updatedProperties)
        {
            if (val.second.assigned())
                checkErrorInfo(objImpl->setProtectedPropertyValue(val.first, val.second));
            else
                checkErrorInfo(objImpl->clearProtectedPropertyValue(val.first));
        }

        checkErrorInfo(objImpl->endUpdate());
    }
    else
    {
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
        checkErrorInfo(dynamic_cast<PropertyObjectImpl*>(obj.getObject())->addProperty(prop));
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
        checkErrorInfo(dynamic_cast<PropertyObjectImpl*>(obj.getObject())->removeProperty(propName));
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
}
