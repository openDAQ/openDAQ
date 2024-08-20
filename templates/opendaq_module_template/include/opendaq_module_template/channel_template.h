#pragma once
#include <opendaq/channel_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class ChannelTemplateHooks;

class ChannelTemplate : public ComponentTemplateBase<ChannelTemplateHooks>,
                        public FunctionBlockTemplateBase
{
public:

    ChannelPtr getChannel() const;

private:

    friend class ChannelTemplateHooks;
};

class ChannelTemplateHooks : public FunctionBlockParamsValidation, public Channel
{
public:

    ChannelTemplateHooks(std::shared_ptr<ChannelTemplate> channel, const FunctionBlockParams& params, const StringPtr& className = "")
        : FunctionBlockParamsValidation(params)
        , Channel(params.type, params.context, params.parent, params.localId, className)
        , channel(std::move(channel))
    {
        this->channel->componentImpl = this; // TODO: Figure out safe ptr operations for this
        this->channel->loggerComponent = this->context.getLogger().getOrAddComponent(params.logName);
        this->channel->context = this->context;

        std::scoped_lock lock(sync);

        this->channel->handleOptions(params.options);
        this->channel->initProperties();
        registerCallbacks<ChannelTemplate>(objPtr, this->channel);
        this->channel->initSignals(signals);
        this->channel->initFunctionBlocks(functionBlocks);
        this->channel->initInputPorts(inputPorts);
              
        this->channel->initTags(tags);
        this->channel->initStatuses(statusContainer);

        this->channel->start();
    }   

private:
    
    friend class ChannelTemplate;
    std::shared_ptr<ChannelTemplate> channel;
};

END_NAMESPACE_OPENDAQ
