#pragma once
#include <opendaq/channel_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class ChannelTemplate : public Channel, ComponentTemplateBase
{
    ChannelTemplate(const FunctionBlockTypePtr& fbType,
                    const ContextPtr& context,
                    const ComponentPtr& parent,
                    const StringPtr& localId,
                    const StringPtr& className = nullptr)
        : Channel(fbType, context, parent, localId, className)
    {
    }

};

END_NAMESPACE_OPENDAQ
