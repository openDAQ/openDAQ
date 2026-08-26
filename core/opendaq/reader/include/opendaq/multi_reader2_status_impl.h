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
#include <coretypes/intfs.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/multi_reader2_status.h>

BEGIN_NAMESPACE_OPENDAQ

class MultiReader2StatusImpl : public ImplementationOf<IMultiReader2Status>
{
public:
    MultiReader2StatusImpl(const DataDescriptorPtr& domainDescriptor,
                           MultiReader2StatusType status,
                           const DictPtr<IString, IDataDescriptor>& descriptors,
                           const DictPtr<IString, IInteger>& errors,
                           const DictPtr<IString, IInteger>& dividers);

    // IMultiReader2Status
    ErrCode INTERFACE_FUNC getStatus(MultiReader2StatusType* status) override;
    ErrCode INTERFACE_FUNC getDomainDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getDescriptors(IDict** descriptors) override;
    ErrCode INTERFACE_FUNC getDividers(IDict** dividers) override;
    ErrCode INTERFACE_FUNC getErrors(IDict** errors) override;

private:
    DataDescriptorPtr domainDescriptor;
    MultiReader2StatusType status;
    DictPtr<IString, IDataDescriptor> descriptors;
    DictPtr<IString, IInteger> errors;
    DictPtr<IString, IInteger> dividers;
};

END_NAMESPACE_OPENDAQ
