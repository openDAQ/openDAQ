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
#include <coretypes/listobject_factory.h>
#include <opendaq/component_ptr.h>
#include <opendaq/data_descriptor_ptr.h>
#include <opendaq/multi_reader2_input_status_ptr.h>
#include <opendaq/multi_reader2_status.h>

BEGIN_NAMESPACE_OPENDAQ

class MultiReader2InputStatusImpl : public ImplementationOf<IMultiReader2InputStatus>
{
public:
    MultiReader2InputStatusImpl(
        ComponentPtr input, MultiReader2InputError error, bool descriptorChanged, DataDescriptorPtr descriptor, SizeT sampleSize);

    ErrCode INTERFACE_FUNC getInput(IComponent** input) override;
    ErrCode INTERFACE_FUNC getError(MultiReader2InputError* error) override;
    ErrCode INTERFACE_FUNC getDescriptorChanged(Bool* changed) override;
    ErrCode INTERFACE_FUNC getDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getSampleSize(SizeT* size) override;

private:
    const ComponentPtr input;
    const MultiReader2InputError error;
    const bool descriptorChanged;
    const DataDescriptorPtr descriptor;
    const SizeT sampleSize;
};

// Immutable result of one read
class MultiReader2StatusImpl : public ImplementationOf<IMultiReader2Status>
{
public:
    struct Fields
    {
        bool hasChanges = false;
        ComponentStatus status = ComponentStatus::Ok;
        std::string message;
        bool domainDescriptorChanged = false;
        DataDescriptorPtr domainDescriptor;
        ComponentPtr mainInput;
        bool resynchronized = false;
        bool synchronizing = false;
        ListPtr<IMultiReader2InputStatus> inputs = List<IMultiReader2InputStatus>();
    };

    explicit MultiReader2StatusImpl(Fields fields);

    ErrCode INTERFACE_FUNC getHasChanges(Bool* hasChanges) override;
    ErrCode INTERFACE_FUNC getStatus(ComponentStatus* status) override;
    ErrCode INTERFACE_FUNC getStatusMessage(IString** message) override;
    ErrCode INTERFACE_FUNC getDomainDescriptorChanged(Bool* changed) override;
    ErrCode INTERFACE_FUNC getDomainDescriptor(IDataDescriptor** descriptor) override;
    ErrCode INTERFACE_FUNC getMainInput(IComponent** input) override;
    ErrCode INTERFACE_FUNC getResynchronized(Bool* resynchronized) override;
    ErrCode INTERFACE_FUNC getSynchronizing(Bool* synchronizing) override;
    ErrCode INTERFACE_FUNC getInputs(IList** inputs) override;
    ErrCode INTERFACE_FUNC getInputStatus(IComponent* input, IMultiReader2InputStatus** status) override;

private:
    const Fields fields;
};

END_NAMESPACE_OPENDAQ
