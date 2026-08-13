/*
 * Copyright 2022-2026 openDAQ d.o.o.
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

#include <opendaq/sync_interface_ptr.h>
#include <opendaq/sync_interface_internal.h>
#include <coreobjects/property_object_impl.h>
#include <coretypes/string_ptr.h>
#include <coreobjects/eval_value_factory.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/objectptr.h>
#include <coretypes/deserializer.h>

BEGIN_NAMESPACE_OPENDAQ

inline ObjectPtr<IInteger> Integer(SyncMode mode)
{
   return Integer(static_cast<Int>(mode));
}

template <typename TInterface = IPropertyObject, typename... Interfaces>
class SyncInterfaceBaseImpl;

using SyncInterfaceBase = SyncInterfaceBaseImpl<>;

template <typename TInterface, typename... Interfaces>
class SyncInterfaceBaseImpl : public GenericPropertyObjectImpl<TInterface, ISyncInterface, ISyncInterfaceInternal, Interfaces...>
{
public:
    using Super = GenericPropertyObjectImpl<TInterface, ISyncInterface, ISyncInterfaceInternal, Interfaces...>;
    using Self = SyncInterfaceBaseImpl<TInterface, Interfaces...>;

    SyncInterfaceBaseImpl();

    // ISyncInterface
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) override;
    ErrCode INTERFACE_FUNC getMode(SyncMode* sourceMode) override;
    ErrCode INTERFACE_FUNC getAvailableModes(IDict** availableModes) override;
    ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** syncStatus) override;
    ErrCode INTERFACE_FUNC getConfiguration(IPropertyObject** configuration) override;

    // ISyncInterfaceInternal
    ErrCode INTERFACE_FUNC setAsSource(Bool source) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

protected:
    explicit SyncInterfaceBaseImpl(const StringPtr& name, const std::vector<SyncMode> & availableModes);

private:
    DictPtr<IInteger, IString> sourceModes;
    DictPtr<IInteger, IString> outputModes;
};

template <typename TInterface, typename... Interfaces>
SyncInterfaceBaseImpl<TInterface, Interfaces...>::SyncInterfaceBaseImpl()
    : Super()
{   
}

template <typename TInterface, typename... Interfaces>
SyncInterfaceBaseImpl<TInterface, Interfaces...>::SyncInterfaceBaseImpl(const StringPtr& name, const std::vector<SyncMode> & availableModes)
    : SyncInterfaceBaseImpl()
{
    sourceModes = Dict<IInteger, IString>();
    outputModes = Dict<IInteger, IString>({{static_cast<Int>(SyncMode::Off), "Off"}});

    for (const auto& mode : availableModes)
    {
        switch (mode)
        {
            case SyncMode::Off:
                outputModes.set(static_cast<Int>(mode), "Off");
                break;
            case SyncMode::Input:
                sourceModes.set(static_cast<Int>(mode), "Input");
                break;
            case SyncMode::Output:
                outputModes.set(static_cast<Int>(mode), "Output");
                break;
            case SyncMode::Auto:
                sourceModes.set(static_cast<Int>(mode), "Auto");
                break;
        }
    }

    this->objPtr.addProperty(StringPropertyBuilder("Name", name).setReadOnly(true).build());
    this->objPtr.addProperty(DictPropertyBuilder("ModeOptions", outputModes).setReadOnly(true).setVisible(false).build());
    this->objPtr.addProperty(SparseSelectionProperty("Mode", EvalValue("$ModeOptions"), Integer(SyncMode::Off)));
    this->objPtr.setPropertyOrder(List<IString>("ModeOptions"));

    auto statusProperty = PropertyObject();
    statusProperty.addProperty(BoolPropertyBuilder("Synchronized", False).setReadOnly(true).build());
    statusProperty.addProperty(StringPropertyBuilder("ReferenceDomainId", "").setReadOnly(true).build());
    this->objPtr.addProperty(ObjectPropertyBuilder("Status", statusProperty).setReadOnly(true).build());
    this->objPtr.addProperty(ObjectProperty("Configuration", PropertyObject()));
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);
    return daqTry([&]
    {
        *name = this->objPtr.getPropertyValue("Name").template as<IString>();
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getMode(SyncMode* sourceMode)
{
    OPENDAQ_PARAM_NOT_NULL(sourceMode);
    *sourceMode = static_cast<SyncMode>(this->objPtr.getPropertyValue("Mode").template asPtr<IInteger>());
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::setAsSource(Bool source)
{
    if (source)
    {
        if (sourceModes.getCount() == 0)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALID_OPERATION, "Current sync interface can not be chossen as source");

        OPENDAQ_RETURN_IF_FAILED(this->setProtectedPropertyValue(String("ModeOptions"), sourceModes));
        
        if (sourceModes.hasKey(static_cast<Int>(SyncMode::Auto)))
            OPENDAQ_RETURN_IF_FAILED(this->setPropertyValue(String("Mode"), Integer(SyncMode::Auto)));
        else
            OPENDAQ_RETURN_IF_FAILED(this->setPropertyValue(String("Mode"), Integer(SyncMode::Input)));

        return OPENDAQ_SUCCESS;
    }

    OPENDAQ_RETURN_IF_FAILED(this->setProtectedPropertyValue(String("ModeOptions"), outputModes));
    OPENDAQ_RETURN_IF_FAILED(this->setPropertyValue(String("Mode"), Integer(SyncMode::Off)));
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getAvailableModes(IDict** availableModes)
{
    OPENDAQ_PARAM_NOT_NULL(availableModes);
    *availableModes = this->objPtr.getPropertyValue("ModeOptions").template as<IDict>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getStatusContainer(IComponentStatusContainer** syncStatus)
{
    return OPENDAQ_IGNORED;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);
    *referenceDomainId = this->objPtr.getPropertyValue("Status.ReferenceDomainId").template as<IString>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getConfiguration(IPropertyObject** configuration)
{
    OPENDAQ_PARAM_NOT_NULL(configuration);
    BaseObjectPtr objPtr;
    OPENDAQ_RETURN_IF_FAILED(this->getPropertyValue(String("Configuration"), &objPtr));
    *configuration = objPtr.asOrNull<IPropertyObject>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ConstCharPtr SyncInterfaceBaseImpl<TInterface, Interfaces...>::SerializeId()
{
    return "SyncInterface";
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::Deserialize(ISerializedObject* serialized,
                                                                       IBaseObject* context,
                                                                       IFunction* factoryCallback,
                                                                       IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(obj);
    return daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = Super::DeserializePropertyObject(
            serialized,
            context,
            factoryCallback,
            [](const SerializedObjectPtr& /*serialized*/, const BaseObjectPtr& /*context*/, const StringPtr& /*className*/)
            {
                return createWithImplementation<TInterface, SyncInterfaceBaseImpl<TInterface, Interfaces...>>();
            }).detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_CLONEABLE);
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncInterfaceBase)

END_NAMESPACE_OPENDAQ
