#pragma once
#include <opendaq/function_block_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

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

class FunctionBlockTemplateHooks : public FunctionBlockParamsValidation, public FunctionBlock
{
public:

    FunctionBlockTemplateHooks(std::shared_ptr<FunctionBlockTemplate> functionBlock, const FunctionBlockParams& params, const StringPtr& className = "")
        : FunctionBlockParamsValidation(params)
        , FunctionBlock(params.type, params.context, params.parent, params.localId, className)
        , functionBlock(std::move(functionBlock))
    {
        this->functionBlock->componentImpl = this; // TODO: Figure out safe ptr operations for this
        this->functionBlock->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
        this->functionBlock->context = this->context;

        std::scoped_lock lock(sync);

        this->functionBlock->handleConfig(params.config);
        this->functionBlock->handleOptions(params.options);
        this->functionBlock->initProperties();
        registerCallbacks<FunctionBlockTemplate>(objPtr, this->functionBlock);
        this->functionBlock->initSignals(signals);
        this->functionBlock->initFunctionBlocks(functionBlocks);
        this->functionBlock->initInputPorts(inputPorts);
              
        this->functionBlock->initTags(tags);
        this->functionBlock->initStatuses(statusContainer);

        this->functionBlock->start();
    }   

private:
    
    friend class FunctionBlockTemplate;
    std::shared_ptr<FunctionBlockTemplate> functionBlock;
};

END_NAMESPACE_OPENDAQ_TEMPLATES
