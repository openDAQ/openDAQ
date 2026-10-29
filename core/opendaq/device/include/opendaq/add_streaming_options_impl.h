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

    ErrCode INTERFACE_FUNC getStreamingConfigs(IDict** configs) override;
    ErrCode INTERFACE_FUNC setStreamingType(IString* typeId) override;
    ErrCode INTERFACE_FUNC getStreamingConfig(IAddStreamingConfig** config, IString* typeId = nullptr) override;
    ErrCode INTERFACE_FUNC getStreamingMode(StreamingMode* mode) override;
    ErrCode INTERFACE_FUNC setStreamingMode(StreamingMode mode) override;

    ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;

private:
    ListPtr<IString> offeredTypeIds();
    bool isOffered(const StringPtr& typeId);
};

END_NAMESPACE_OPENDAQ
