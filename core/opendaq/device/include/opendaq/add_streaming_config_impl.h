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
    AddStreamingConfigImpl(const StreamingTypePtr& streamingType, Partial);

    ErrCode INTERFACE_FUNC getStreamingType(IStreamingType** type) override;
    ErrCode INTERFACE_FUNC getTypeConfig(IPropertyObject** config) override;

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    ErrCode serializeCustomValues(ISerializer* serializer, bool forUpdate) override;

private:
    StreamingTypePtr streamingType;
};

END_NAMESPACE_OPENDAQ
