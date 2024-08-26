#pragma once
#include <opendaq/channel_impl.h>
#include <opendaq_module_template/component_template_base.h>

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

class ChannelTemplateHooks : public ChannelParamsValidation, public Channel
{
public:

    ChannelTemplateHooks(std::shared_ptr<ChannelTemplate> channel, const ChannelParams& params, const StringPtr& className = "")
        : ChannelParamsValidation(params)
        , Channel(params.type, params.context, params.parent, params.localId, className)
        , channel(std::move(channel))
    {
        this->channel->componentImpl = this; // TODO: Figure out safe ptr operations for this
        this->channel->objPtr = this->borrowPtr<PropertyObjectPtr>();
        this->channel->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
        this->channel->context = this->context;

        std::scoped_lock lock(sync);

        // TODO: Check if this is needed
        // this->channel->handleOptions(params.options);
        this->channel->initProperties();
        registerCallbacks<ChannelTemplate>(objPtr, this->channel);
        this->channel->initSignals(signals);
        this->channel->initFunctionBlocks(functionBlocks);
        this->channel->initInputPorts(inputPorts);
              
        this->channel->initTags(tags);
        this->channel->initStatuses(statusContainer);

        this->channel->start();
    }

    template <class TemplateImpl>
    std::shared_ptr<TemplateImpl> getChannelTemplate();

private:
    
    friend class ChannelTemplate;
    friend class ComponentTemplateBase<ChannelTemplateHooks>;
    std::shared_ptr<ChannelTemplate> channel;
};

template <class TemplateImpl>
std::shared_ptr<TemplateImpl> ChannelTemplateHooks::getChannelTemplate()
{
    return std::shared_ptr<TemplateImpl>(channel, reinterpret_cast<TemplateImpl*>(channel.get())); 
}

END_NAMESPACE_OPENDAQ_TEMPLATES
