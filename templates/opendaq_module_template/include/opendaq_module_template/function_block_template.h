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
#include <opendaq/function_block_impl.h>
#include <opendaq_module_template/component_template_base.h>
#include <opendaq_module_template/hooks_template_base.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

/*
 * Function block template TODO:
 *  - Translate reference function block to template
 *  - Reevaluate template function block functionalities and extend the templates
 *      - Helpers for reader creation and data access
 *      - Helpers for multi reader creation and usage
 *      - Helpers for handling signal events
 *      - Helpers for input port creation
 */

class FunctionBlockTemplateHooks;

class FunctionBlockTemplate : public ComponentTemplateBase<FunctionBlockTemplateHooks>,
                              public AddableComponentTemplateBase,
                              public FunctionBlockTemplateBase
{
public:

    FunctionBlockPtr getFunctionBlock() const;

private:

    friend class FunctionBlockTemplateHooks;
};

class FunctionBlockTemplateHooks
    : public TemplateHooksBase<FunctionBlockTemplate>,
      public FunctionBlockParamsValidation,
      public FunctionBlock
{
public:

    FunctionBlockTemplateHooks(const std::shared_ptr<FunctionBlockTemplate>& functionBlock, const FunctionBlockParams& params, const StringPtr& className = "")
        : TemplateHooksBase(functionBlock)
        , FunctionBlockParamsValidation(params)
        , FunctionBlock(params.type, params.context, params.parent.getRef(), params.localId, className)
    {
        this->templateImpl->componentImpl = this;
        this->templateImpl->objPtr = this->thisPtr<PropertyObjectPtr>();
        this->templateImpl->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
        this->templateImpl->context = this->context;

        auto lock = this->getAcquisitionLock();
        registerCallbacks(objPtr);

        this->templateImpl->initProperties();
        this->templateImpl->applyConfig(params.config);
        this->templateImpl->initSignals(signals);
        this->templateImpl->initFunctionBlocks(functionBlocks);
        this->templateImpl->initInputPorts(inputPorts);
              
        this->templateImpl->initTags(tags);
        this->templateImpl->initStatuses(statusContainer);

        this->templateImpl->start();
    }   

private:
    
    void removed() override;

    friend class FunctionBlockTemplate;
    friend class ComponentTemplateBase<FunctionBlockTemplateHooks>;
};

END_NAMESPACE_OPENDAQ_TEMPLATES
