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
#include <opendaq/add_streaming_options_ptr.h>
#include <opendaq/add_streaming_config_ptr.h>
#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class AddStreamingOptionsImpl : public GenericPropertyObjectImpl<IPropertyObject, IAddStreamingOptions>
{
    class Partial
    {
        Partial()
        {
        }
        friend class AddStreamingOptionsImpl;
    };

public:
    using Super = GenericPropertyObjectImpl<IPropertyObject, IAddStreamingOptions>;

    explicit AddStreamingOptionsImpl(const DictPtr<IString, IAddStreamingConfig>& streamingConfigs);
    explicit AddStreamingOptionsImpl(Partial);

    ErrCode INTERFACE_FUNC getAvailableStreamingConfigs(IDict** configs) override;
    ErrCode INTERFACE_FUNC getStreamingType(IString** typeId) override;
    ErrCode INTERFACE_FUNC setStreamingType(IString* typeId) override;
    ErrCode INTERFACE_FUNC getStreamingConfig(IAddStreamingConfig** config) override;
    ErrCode INTERFACE_FUNC getStreamingMode(StreamingMode* mode) override;
    ErrCode INTERFACE_FUNC setStreamingMode(StreamingMode mode) override;

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

    // The base update fails on a saved streaming type that is not offered and skips the values after it.
    // This one selects no streaming type instead and applies the rest.
    ErrCode INTERFACE_FUNC updateInternal(ISerializedObject* obj, IBaseObject* context) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    void removeUnselected();

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);
    static StringPtr SavedStreamingType(const SerializedObjectPtr& saved);

protected:
    ErrCode serializeCustomValues(ISerializer* serializer, bool forUpdate) override;
    ErrCode serializePropertyValue(const StringPtr& name,
                                   const ObjectPtr<IBaseObject>& value,
                                   ISerializer* serializer,
                                   bool forUpdate) override;

private:
    ListPtr<IString> offeredTypeIds();
    bool isOffered(const StringPtr& typeId);

    // In case of remote device adds, the config is created on the server, and sent to the client
    // with the current impl, it makes sense for the client to drop unselected streaming configs when sending back
    // for forward compatibility, newer servers can send non default TrimData to disable trimming on client side
    // zero cost, as they are serialized only when not set to defaults
    struct TrimData
    {
        bool trimUnselected = true;
        ListPtr<IString> exceptions = List<IString>();
    } trimData;
};

END_NAMESPACE_OPENDAQ
