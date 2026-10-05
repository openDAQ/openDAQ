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
#include <coretypes/listptr.h>
#include <opendaq/component_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/multi_reader2_input_status_ptr.h>
#include <opendaq/multi_reader2_status.h>

#include <vector>

BEGIN_NAMESPACE_OPENDAQ

class MultiReader2InputStatusImpl : public ImplementationOf<IMultiReader2InputStatus>
{
public:
    MultiReader2InputStatusImpl(const ComponentPtr& input,
                                bool used,
                                MultiReader2InputError error,
                                bool descriptorChanged,
                                const DataDescriptorPtr& descriptor);

    // IMultiReader2InputStatus
    ErrCode INTERFACE_FUNC getInput(IComponent** input) override;
    ErrCode INTERFACE_FUNC getUsed(Bool* used) override;
    ErrCode INTERFACE_FUNC getError(MultiReader2InputError* error) override;
    ErrCode INTERFACE_FUNC getDescriptorChanged(Bool* changed) override;
    ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) override;

private:
    ComponentPtr input;
    bool used;
    MultiReader2InputError error;
    bool descriptorChanged;
    DataDescriptorPtr descriptor;
};

class MultiReader2StatusImpl : public ImplementationOf<IMultiReader2Status>
{
public:
    MultiReader2StatusImpl(bool hasChanges,
                           bool valid,
                           bool domainDescriptorChanged,
                           const DataDescriptorPtr& domainDescriptor,
                           bool resynchronized,
                           const ListPtr<IMultiReader2InputStatus>& inputs);

    // IMultiReader2Status
    ErrCode INTERFACE_FUNC getHasChanges(Bool* hasChanges) override;
    ErrCode INTERFACE_FUNC getValid(Bool* valid) override;
    ErrCode INTERFACE_FUNC getDomainDescriptorChanged(Bool* changed) override;
    ErrCode INTERFACE_FUNC getDomainDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getResynchronized(Bool* resynchronized) override;
    ErrCode INTERFACE_FUNC getInputs(IList** inputs) override;
    ErrCode INTERFACE_FUNC getInputStatus(IComponent* input, IMultiReader2InputStatus** status) override;

private:
    bool hasChanges;
    bool valid;
    bool domainDescriptorChanged;
    DataDescriptorPtr domainDescriptor;
    bool resynchronized;
    ListPtr<IMultiReader2InputStatus> inputs;
};

END_NAMESPACE_OPENDAQ
