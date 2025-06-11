#include <opendaq_module_template/channel_template.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

ChannelPtr ChannelTemplate::getChannel() const
{
    return componentImpl->objPtr;
}

void ChannelTemplateHooks::removed()
{
    templateImpl->removed();
    templateImpl.reset();
    ChannelImpl::removed();
}

END_NAMESPACE_OPENDAQ_TEMPLATES
