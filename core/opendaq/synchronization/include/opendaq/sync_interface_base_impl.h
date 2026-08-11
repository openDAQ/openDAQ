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
    ErrCode INTERFACE_FUNC setOutputOnly(Bool outputOnly) override;
    ErrCode INTERFACE_FUNC getOutputOnly(Bool* outputOnly) override;
    ErrCode INTERFACE_FUNC getStatusContainer(IComponentStatusContainer** syncStatus) override;

    // ISyncInterfaceInternal
    ErrCode INTERFACE_FUNC setMode(SyncMode mode) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    // IPropertyObjectInternal
    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

protected:
    void setModeOptions(const ListPtr<IString>& options);
    explicit SyncInterfaceBaseImpl(const StringPtr& name); // probably also add available modes and remove setModeOptions
};

template <typename TInterface, typename... Interfaces>
SyncInterfaceBaseImpl<TInterface, Interfaces...>::SyncInterfaceBaseImpl()
    : Super()
{   
    const auto modeOptions = Dict<IInteger, IString>({
        {static_cast<Int>(SyncMode::Off), "Off"},
        {static_cast<Int>(SyncMode::Input), "Input"},
        {static_cast<Int>(SyncMode::Output), "Output"},
        {static_cast<Int>(SyncMode::Auto), "Auto"},
    });

    this->objPtr.addProperty(StringPropertyBuilder("Name", "SyncInterfaceBase").setReadOnly(true).build());
    this->objPtr.addProperty(DictPropertyBuilder("ModeOptions", modeOptions).setReadOnly(true).setVisible(false).build());
    this->objPtr.addProperty(SparseSelectionPropertyBuilder("Mode", EvalValue("$ModeOptions"), static_cast<Int>(SyncMode::Off)).setReadOnly(true).build());
    this->objPtr.addProperty(BoolPropertyBuilder("OutputOnly", false).setReadOnly(true).setVisible(false).build());
    this->objPtr.setPropertyOrder(List<IString>("ModeOptions"));

    auto statusProperty = PropertyObject();
    statusProperty.addProperty(BoolPropertyBuilder("Synchronized", False).setReadOnly(true).build());
    statusProperty.addProperty(StringPropertyBuilder("ReferenceDomainId", "").setReadOnly(true).build());
    this->objPtr.addProperty(ObjectPropertyBuilder("Status", statusProperty).setReadOnly(true).build());
}

template <typename TInterface, typename... Interfaces>
SyncInterfaceBaseImpl<TInterface, Interfaces...>::SyncInterfaceBaseImpl(const StringPtr& name)
    : SyncInterfaceBaseImpl()
{
    if (name.assigned())
        this->objPtr.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("Name", name);
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
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::setMode(SyncMode sourceMode)
{
    const auto converted = Integer(static_cast<Int>(sourceMode));
    const ErrCode errCode = this->setProtectedPropertyValue(String("Mode"), converted);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getAvailableModes(IDict** availableModes)
{
    OPENDAQ_PARAM_NOT_NULL(availableModes);
    *availableModes = this->objPtr.getPropertyValue("ModeOptions").template as<IDict>();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::setOutputOnly(Bool outputOnly)
{
    const ErrCode errCode = this->setProtectedPropertyValue(String("OutputOnly"), Boolean(outputOnly));
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getOutputOnly(Bool* outputOnly)
{
    OPENDAQ_PARAM_NOT_NULL(outputOnly);
    BaseObjectPtr valuePtr;
    const ErrCode errCode = this->getPropertyValue(String("OutputOnly"), &valuePtr);
    OPENDAQ_RETURN_IF_FAILED(errCode);
    OPENDAQ_RETURN_IF_FAILED(valuePtr.asPtr<IBoolean>(true)->getValue(outputOnly));
    return errCode;
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
void SyncInterfaceBaseImpl<TInterface, Interfaces...>::setModeOptions(const ListPtr<IString>& options)
{
    this->objPtr.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("ModeOptions", options);
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
