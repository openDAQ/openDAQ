#include <opendaq_module_template/function_block_template.h>

BEGIN_NAMESPACE_OPENDAQ_TEMPLATES

FunctionBlockPtr FunctionBlockTemplate::getFunctionBlock() const
{
    return componentImpl->objPtr;
}

END_NAMESPACE_OPENDAQ_TEMPLATES
