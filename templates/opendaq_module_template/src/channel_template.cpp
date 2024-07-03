#include <opendaq_module_template/channel_template.h>

BEGIN_NAMESPACE_OPENDAQ

TemplateChannel::TemplateChannel(const FunctionBlockTypePtr& fbType,
                                 const ContextPtr& context,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId,
                                 const StringPtr& className)
    : Channel(fbType, context, parent, localId, className)
{
}

END_NAMESPACE_OPENDAQ
