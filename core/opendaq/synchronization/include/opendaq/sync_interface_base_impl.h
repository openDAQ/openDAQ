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

#include <opendaq/sync_interface.h>
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

    explicit SyncInterfaceBaseImpl(const StringPtr& name, const DictPtr<IInteger, IString>& options = nullptr);

    // ISyncInterface
    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC getSynced(Bool* synced) override;
    ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) override;

    // ISyncInterfaceInternal
    ErrCode INTERFACE_FUNC setAsSource(Bool isSource) override;

    // ISerializable
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:

    virtual DictPtr<IInteger, IString> getModeOptions() const;

    void setModeOptions(const DictPtr<IInteger, IString>& options);
    void setMode(const StringPtr& mode);
    void setReferenceDomainId(const StringPtr& domainId);
    void setSynced(Bool synced);

    Bool synced;
    StringPtr referenceDomainId;
};

template <typename TInterface, typename... Interfaces>
SyncInterfaceBaseImpl<TInterface, Interfaces...>::SyncInterfaceBaseImpl()
    : Super()
    , synced(False)
    , referenceDomainId("")
{   
    this->objPtr.addProperty(StringPropertyBuilder("Name", "SyncInterfaceBase").setReadOnly(true).build());
    this->objPtr.addProperty(DictProperty("ModeOptions", Self::getModeOptions(), false));
    this->objPtr.addProperty(SelectionProperty("Mode", EvalValue("$ModeOptions"), 3));

    auto statusProperty = PropertyObject();
    statusProperty.addProperty(BoolPropertyBuilder("Synchronized", synced).setReadOnly(true).build());
    statusProperty.addProperty(StringPropertyBuilder("ReferenceDomainId", referenceDomainId).setReadOnly(true).build());
    this->objPtr.addProperty(ObjectPropertyBuilder("Status", statusProperty).setReadOnly(true).build());
}

template <typename TInterface, typename... Interfaces>
SyncInterfaceBaseImpl<TInterface, Interfaces...>::SyncInterfaceBaseImpl(const StringPtr& name, const DictPtr<IInteger, IString>& options)
    : SyncInterfaceBaseImpl()
{
    if (name.assigned())
        this->objPtr.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("Name", name);
    if (options.assigned())
        this->setModeOptions(options);
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
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getSynced(Bool* synced)
{
    OPENDAQ_PARAM_NOT_NULL(synced);
    *synced = this->synced;
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::getReferenceDomainId(IString** referenceDomainId)
{
    OPENDAQ_PARAM_NOT_NULL(referenceDomainId);
    *referenceDomainId = this->referenceDomainId.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename TInterface, typename... Interfaces>
ErrCode SyncInterfaceBaseImpl<TInterface, Interfaces...>::setAsSource(Bool isSource)
{
    return OPENDAQ_IGNORED;
}

template <typename TInterface, typename... Interfaces>
DictPtr<IInteger, IString> SyncInterfaceBaseImpl<TInterface, Interfaces...>::getModeOptions() const
{
    return Dict<IInteger, IString>({{0, "Input"}, {1, "Output"}, {2, "Auto"}, {3, "Off"}});
}

template <typename TInterface, typename... Interfaces>
void SyncInterfaceBaseImpl<TInterface, Interfaces...>::setModeOptions(const DictPtr<IInteger, IString>& options)
{
    this->objPtr.setPropertyValue("ModeOptions", options);
}

template <typename TInterface, typename... Interfaces>
void SyncInterfaceBaseImpl<TInterface, Interfaces...>::setMode(const StringPtr& mode)
{
    this->objPtr.setPropertySelectionValue("Mode", mode);
}

template <typename TInterface, typename... Interfaces>
void SyncInterfaceBaseImpl<TInterface, Interfaces...>::setReferenceDomainId(const StringPtr& domainId)
{
    this->referenceDomainId = domainId;
    this->objPtr.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("Status.ReferenceDomainId", domainId);
}

template <typename TInterface, typename... Interfaces>
void SyncInterfaceBaseImpl<TInterface, Interfaces...>::setSynced(Bool synced)
{
    this->synced = synced;
    this->objPtr.template asPtr<IPropertyObjectProtected>(true).setProtectedPropertyValue("Status.Synchronized", synced);
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

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncInterfaceBase)

END_NAMESPACE_OPENDAQ
