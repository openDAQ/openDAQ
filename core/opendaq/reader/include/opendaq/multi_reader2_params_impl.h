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
#include <opendaq/component_ptr.h>
#include <opendaq/multi_reader2_params.h>

#include <mutex>

BEGIN_NAMESPACE_OPENDAQ

class MultiReader2ParamsImpl : public ImplementationOf<IMultiReader2Params>
{
public:
    MultiReader2ParamsImpl();

    // IMultiReader2Params
    ErrCode INTERFACE_FUNC getInputs(IList** inputs) override;
    ErrCode INTERFACE_FUNC setInputs(IList* inputs) override;
    ErrCode INTERFACE_FUNC getMainInput(IComponent** input) override;
    ErrCode INTERFACE_FUNC setMainInput(IComponent* input) override;
    ErrCode INTERFACE_FUNC getUnusedInputs(IList** inputs) override;
    ErrCode INTERFACE_FUNC setUnusedInputs(IList* inputs) override;
    ErrCode INTERFACE_FUNC getValueReadType(SampleType* valueReadType) override;
    ErrCode INTERFACE_FUNC setValueReadType(SampleType valueReadType) override;
    ErrCode INTERFACE_FUNC getMinReadCount(SizeT* count) override;
    ErrCode INTERFACE_FUNC setMinReadCount(SizeT count) override;
    ErrCode INTERFACE_FUNC getRequireSameRates(Bool* requireSameRates) override;
    ErrCode INTERFACE_FUNC setRequireSameRates(Bool requireSameRates) override;

private:
    std::mutex mutex;
    ListPtr<IComponent> inputs;
    ListPtr<IComponent> unusedInputs;
    ComponentPtr mainInput;
    SizeT minReadCount = 1;
    Bool requireSameRates = False;
    SampleType valueReadType = SampleType::Invalid;
    bool valueReadTypeAssigned = false;
};

END_NAMESPACE_OPENDAQ
