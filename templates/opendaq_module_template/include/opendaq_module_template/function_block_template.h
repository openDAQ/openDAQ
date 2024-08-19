#pragma once
#include <opendaq/function_block_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class FunctionBlockTemplateHooks;

class FunctionBlockTemplate : public ComponentTemplateBase<FunctionBlockTemplateHooks>
{
public:
    FunctionBlockPtr getFunctionBlock() const;

protected:

    virtual void handleConfig(const PropertyObjectPtr& config);
    virtual void handleOptions(const DictPtr<IString, IBaseObject>& options);
    virtual void initSignals(const FolderConfigPtr& signalsFolder);
    virtual void initFunctionBlocks(const FolderConfigPtr& fbFolder);

    virtual void start();

    virtual BaseObjectPtr onPropertyWrite(const StringPtr& propertyName, const PropertyPtr& property, const BaseObjectPtr& value);
    virtual BaseObjectPtr onPropertyRead(const StringPtr& propertyName, const PropertyPtr& property, const BaseObjectPtr& value);


private:
    friend class FunctionBlockTemplateHooks;
};

class FunctionBlockTemplateHooks : public FunctionBlockParamsValidation, public FunctionBlock
{
public:

    FunctionBlockTemplateHooks(std::unique_ptr<FunctionBlockTemplate> functionBlock, const FunctionBlockParams& params, const StringPtr& className = "")
        : FunctionBlockParamsValidation(params)
        , FunctionBlock(params.type, params.context, params.parent, params.localId, className)
        , functionBlock(std::move(functionBlock))
        , initialized(false)
    {
        this->functionBlock->componentImpl = this; // TODO: Figure out safe ptr operations for this
        this->functionBlock->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
        this->functionBlock->context = this->context;

        std::scoped_lock lock(sync);

        this->functionBlock->handleConfig(params.config);
        this->functionBlock->handleOptions(params.options);
        this->functionBlock->initProperties();
        registerCallbacks(objPtr);
    }   

private:
    
    void onObjectReady() override;
    void registerCallbacks(const PropertyObjectPtr& obj);
    
    friend class FunctionBlockTemplate;
    std::unique_ptr<FunctionBlockTemplate> functionBlock;
    bool initialized;
};

END_NAMESPACE_OPENDAQ
