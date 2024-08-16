/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/impl.h>
#include <opendaq/reference_domain_info_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class ReferenceDomainInfoBuilderImpl : public ImplementationOf<IReferenceDomainInfoBuilder>
{
public:
    explicit ReferenceDomainInfoBuilderImpl();
    explicit ReferenceDomainInfoBuilderImpl(const ReferenceDomainInfoPtr& infoCopy);

    ErrCode INTERFACE_FUNC build(IReferenceDomainInfo** info) override;

    ErrCode INTERFACE_FUNC setReferenceDomainId(IString* referenceDomainId) override;
    ErrCode INTERFACE_FUNC getReferenceDomainId(IString** referenceDomainId) override;

    ErrCode INTERFACE_FUNC setReferenceDomainOffset(IInteger* referenceDomainOffset) override;
    ErrCode INTERFACE_FUNC getReferenceDomainOffset(IInteger** referenceDomainOffset) override;

    ErrCode INTERFACE_FUNC setReferenceTimeSource(TimeSource referenceTimeSource) override;
    ErrCode INTERFACE_FUNC getReferenceTimeSource(TimeSource* referenceTimeSource) override;

protected:
    StringPtr referenceDomainId;
    IntegerPtr referenceDomainOffset;
    TimeSource referenceTimeSource;
};

END_NAMESPACE_OPENDAQ
