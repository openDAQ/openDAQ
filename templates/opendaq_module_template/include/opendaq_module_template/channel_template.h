#pragma once
#include <opendaq/channel_impl.h>
#include <opendaq_module_template/component_template_base.h>
#include <opendaq_module_template/hooks_template_base.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

class ChannelTemplateHooks;

class ChannelTemplate : public ComponentTemplateBase<ChannelTemplateHooks>,
                        public FunctionBlockTemplateBase,
                        public std::enable_shared_from_this<ChannelTemplate>
{
public:

    ChannelPtr getChannel() const;

private:

    friend class ChannelTemplateHooks;
};

class ChannelTemplateHooks
    : public TemplateHooksBase<ChannelTemplate>, public ChannelParamsValidation, public Channel
{
public:

    ChannelTemplateHooks(const std::shared_ptr<ChannelTemplate>& ch, const ChannelParams& params)
        : TemplateHooksBase(ch)
        , ChannelParamsValidation(params)
        , Channel(params.type, params.context, params.parent.getRef(), params.localId, params.className)
    {
        this->templateImpl->componentImpl = this;
        this->templateImpl->objPtr = this->thisPtr<PropertyObjectPtr>();
        this->templateImpl->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
        this->templateImpl->context = this->context;

        auto lock = this->getRecursiveConfigLock();
        
        registerCallbacks(objPtr);

        this->templateImpl->initProperties();
        this->templateImpl->initSignals(signals);
        this->templateImpl->initFunctionBlocks(functionBlocks);
        this->templateImpl->initInputPorts(inputPorts);
              
        this->templateImpl->initTags(tags);
        this->templateImpl->initStatuses(statusContainer);

        this->templateImpl->start();
    }

    template <class TemplateImpl>
    std::shared_ptr<TemplateImpl> getChannelTemplate();

private:
    
    void removed() override;

    friend class ChannelTemplate;
    friend class ComponentTemplateBase<ChannelTemplateHooks>;
};

template <class TemplateImpl>
std::shared_ptr<TemplateImpl> ChannelTemplateHooks::getChannelTemplate()
{
    return std::shared_ptr<TemplateImpl>(templateImpl, reinterpret_cast<TemplateImpl*>(templateImpl.get())); 
}

END_NAMESPACE_OPENDAQ_TEMPLATES
