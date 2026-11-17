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
#include <opendaq/add_streaming_config_ptr.h>
#include <opendaq/streaming_type_ptr.h>
#include <opendaq/transport_option_ptr.h>
#include <opendaq/transport_option_impl.h>
#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class AddStreamingConfigImpl : public GenericPropertyObjectImpl<IPropertyObject, IAddStreamingConfig>
{
    class Partial
    {
        Partial()
        {
        }
        friend class AddStreamingConfigImpl;
    };

public:
    using Super = GenericPropertyObjectImpl<IPropertyObject, IAddStreamingConfig>;

    explicit AddStreamingConfigImpl(const StreamingTypePtr& streamingType);

    // Lists the transport options the server of a device announces instead of the ones of the type, unless they are
    // not assigned
    AddStreamingConfigImpl(const StreamingTypePtr& streamingType, const ListPtr<ITransportOption>& transportOptions);
    AddStreamingConfigImpl(const StreamingTypePtr& streamingType, const ListPtr<ITransportOption>& transportOptions, Partial);

    ErrCode INTERFACE_FUNC getStreamingType(IStreamingType** type) override;
    ErrCode INTERFACE_FUNC getTypeConfig(IPropertyObject** config) override;
    ErrCode INTERFACE_FUNC getAuthenticationConfig(IAuthenticationConfig** config) override;
    ErrCode INTERFACE_FUNC getTransportOptions(IList** transportOptions) override;
    ErrCode INTERFACE_FUNC setTransportOption(IString* name) override;
    ErrCode INTERFACE_FUNC getSelectedTransportOption(ITransportOption** transportOption) override;
    ErrCode INTERFACE_FUNC setPort(IInteger* port) override;
    ErrCode INTERFACE_FUNC getPort(IInteger** port) override;

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    ErrCode serializeCustomValues(ISerializer* serializer, bool forUpdate) override;

private:
    StreamingTypePtr streamingType;
    // The options the server of the device announces, else the Streaming type's
    ListPtr<ITransportOption> transportOptions;
};

END_NAMESPACE_OPENDAQ
