#include <opendaq_module_template/function_block_template.h>

BEGIN_NAMESPACE_OPENDAQ

TemplateFunctionBlock::TemplateFunctionBlock(const FunctionBlockTypePtr& devType,
                                             const ContextPtr& context,
                                             const ComponentPtr& parent,
                                             const StringPtr& localId,
                                             const StringPtr& className)
    : FunctionBlock(devType, context, parent, localId, className)
{
}

END_NAMESPACE_OPENDAQ
