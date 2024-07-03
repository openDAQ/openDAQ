#pragma once
#include <opendaq/channel_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class TemplateChannel : public Channel, ComponentTemplateBase
{
    TemplateChannel(const FunctionBlockTypePtr& fbType,
                    const ContextPtr& context,
                    const ComponentPtr& parent,
                    const StringPtr& localId,
                    const StringPtr& className = nullptr);

};

END_NAMESPACE_OPENDAQ
