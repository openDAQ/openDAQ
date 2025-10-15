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
#include <vector_extractor/common.h>

#include <opendaq/module_impl.h>

BEGIN_NAMESPACE_VECTOR_EXTRACTOR_MODULE

class VectorExtractorModule final : public Module
{
public:
    explicit VectorExtractorModule(ContextPtr context);

    DictPtr<IString, IFunctionBlockType> onGetAvailableFunctionBlockTypes() override;
    FunctionBlockPtr onCreateFunctionBlock(const StringPtr& id, const ComponentPtr& parent, const StringPtr& localId, const PropertyObjectPtr& config) override;
};

END_NAMESPACE_VECTOR_EXTRACTOR_MODULE

