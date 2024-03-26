/*
 * Copyright 2022-2024 Blueberry d.o.o.
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
#include <opendaq/streaming_info_config.h>
#include <coretypes/intfs.h>
#include <coretypes/string_ptr.h>
#include <opendaq/component_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class StreamingInfoConfigImpl : public GenericPropertyObjectImpl<IStreamingInfoConfig>
{
public:
    using Super = GenericPropertyObjectImpl<IStreamingInfoConfig>;

    explicit StreamingInfoConfigImpl(const StringPtr& protocolId);

    // IStreamingInfo
    ErrCode INTERFACE_FUNC getPrimaryAddress(IString** address) override;
    ErrCode INTERFACE_FUNC getProtocolId(IString** protocolId) override;

    // IStreamingInfoConfig
    ErrCode INTERFACE_FUNC setPrimaryAddress(IString* address) override;

private:
    void setStringProperty(const StringPtr& name, const BaseObjectPtr& value);
    StringPtr getStringProperty(const StringPtr& name);
};

END_NAMESPACE_OPENDAQ
