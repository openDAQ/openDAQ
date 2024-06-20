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
#include <opendaq/streaming_type.h>
#include <coreobjects/component_type_impl.h>
#include <opendaq/component_type_builder_ptr.h>
#include <opendaq/streaming_type_factory.h>

BEGIN_NAMESPACE_OPENDAQ

class StreamingTypeImpl : public GenericComponentTypeImpl<IStreamingType>
{
public:
    using Self = StreamingTypeImpl;
    using Super = GenericComponentTypeImpl<IStreamingType>;

    explicit StreamingTypeImpl(const StringPtr& id,
                               const StringPtr& name,
                               const StringPtr& description,
                               const StringPtr& prefix,
                               const PropertyObjectPtr& defaultConfig);

    explicit StreamingTypeImpl(const ComponentTypeBuilderPtr& builder);
    
    ErrCode getConnectionStringPrefix(IString** prefix) override;
};

END_NAMESPACE_OPENDAQ
