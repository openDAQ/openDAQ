/*
 * Copyright 2022-2023 Blueberry d.o.o.
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
#include <ref_fb_module/common.h>
#include <opendaq/function_block_ptr.h>
#include <ref_fb_module/averager_context.h>
#include <opendaq/function_block_impl.h>
#include <opendaq/input_port_config_ptr.h>

BEGIN_NAMESPACE_REF_FB_MODULE

namespace Averager
{

class AveragerFbImpl final : public FunctionBlock
{
public:
    AveragerFbImpl(const ContextPtr& ctx, const ComponentPtr& parent, const StringPtr& localId);

    ErrCode INTERFACE_FUNC getInputPorts(IList** inputPorts) override;
    ErrCode INTERFACE_FUNC getSignals(IList** signals) override;
    ErrCode INTERFACE_FUNC getSignalsRecursive(IList** signals) override;

    void onConnected(const InputPortPtr& port) override;
    void onDisconnected(const InputPortPtr& port) override;

    static FunctionBlockTypePtr CreateType();

    static AveragerContext& getAveragerContext(const InputPortConfigPtr& inputPort);
protected:
    void removed() override;
private:
    std::vector<GenericFunctionBlockPtr<IAveragerContext>> averagerContexts;
    size_t blockSize;
    DomainSignalType domainSignalType;
    size_t avgCtxIndex;

    void initProperties();
    void propertyChanged();
    void configureAveragerContexts();
    void configureAveragerContext(AveragerContext& avgContext);
    void readProperties();
    void updateAveragerContexts();
};

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    INTERNAL_FACTORY,
    AveragerFb,
    IFunctionBlock,
    IContext*, context,
    IString*, parentGlobalId,
    IString*, localId
    );

}

END_NAMESPACE_REF_FB_MODULE
