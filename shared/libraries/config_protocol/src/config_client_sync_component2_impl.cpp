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

#include <config_protocol/config_client_sync_component2_impl.h>
#include <opendaq/sync_component2.h>
#include <opendaq/component_deserialize_context_ptr.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/objectptr.h>
#include <opendaq/component.h>

namespace daq::config_protocol
{

template <class Impl>
ErrCode ConfigClientBaseSyncComponent2Impl<Impl>::getSelectedSource(ISyncInterface** selectedSource)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSource);
    return daqTry([&]
    {
        StringPtr sourceName = this->objPtr.getPropertySelectionValue("Source");
        PropertyObjectPtr interfaces = this->objPtr.getPropertyValue("Interfaces");
        *selectedSource = interfaces.getPropertyValue(sourceName).template as<ISyncInterface>();
        return OPENDAQ_SUCCESS;
    });
}

template <class Impl>
ErrCode ConfigClientBaseSyncComponent2Impl<Impl>::setSelectedSource(IString* selectedSourceName)
{
    OPENDAQ_PARAM_NOT_NULL(selectedSourceName);
    return daqTry([&]
    {
        const StringPtr selectedSourceNamePtr = StringPtr::Borrow(selectedSourceName);
        PropertyObjectPtr interfaces = this->objPtr.getPropertyValue("Interfaces");
        if (!interfaces.hasProperty(selectedSourceNamePtr))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format("Interface '{}' not found in interfaces", selectedSourceNamePtr));
        this->objPtr.setPropertySelectionValue("Source", selectedSourceNamePtr);
        return OPENDAQ_SUCCESS;
    });
}

template <class Impl>
ErrCode ConfigClientBaseSyncComponent2Impl<Impl>::addInterface(ISyncInterface* syncInterface)
{
    return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SUPPORTED, "Adding interfaces is not supported on the client side");
}

template <class Impl>
ErrCode ConfigClientBaseSyncComponent2Impl<Impl>::Deserialize(ISerializedObject* serialized,
                                                              IBaseObject* context,
                                                              IFunction* factoryCallback,
                                                              IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(context);

    const ErrCode errCode = daqTry([&obj, &serialized, &context, &factoryCallback]
    {
        *obj = DeserializeSyncComponent2<ISyncComponent2, ConfigClientSyncComponent2Impl>(serialized, context, factoryCallback).detach();
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}

template <class Impl>
template <class Interface, class Implementation>
BaseObjectPtr ConfigClientBaseSyncComponent2Impl<Impl>::DeserializeSyncComponent2(const SerializedObjectPtr& serialized,
                                                                                 const BaseObjectPtr& context,
                                                                                 const FunctionPtr& factoryCallback)
{
    return Impl::DeserializeComponent(
        serialized,
        context,
        factoryCallback,
        [](const SerializedObjectPtr& serialized, const ComponentDeserializeContextPtr& deserializeContext, const StringPtr& className)
        {
            const auto ctx = deserializeContext.asPtr<IConfigProtocolDeserializeContext>();
            return createWithImplementation<Interface, Implementation>(ctx->getClientComm(),
                                                                     ctx->getRemoteGlobalId(),
                                                                     deserializeContext.getContext(),
                                                                     deserializeContext.getParent(),
                                                                     deserializeContext.getLocalId(),
                                                                     className);
        });
}

// Explicit template instantiation
template class ConfigClientBaseSyncComponent2Impl<SyncComponent2Impl<IComponent, IConfigClientObject>>;

} // namespace daq::config_protocol
