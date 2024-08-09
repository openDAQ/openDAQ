#pragma once
#include <opendaq/function_block_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class FunctionBlockTemplate : public FunctionBlock, ComponentTemplateBase
{
    FunctionBlockTemplate(const FunctionBlockTypePtr& devType,
                          const ContextPtr& context,
                          const ComponentPtr& parent,
                          const StringPtr& localId,
                          const StringPtr& className = nullptr)
        : FunctionBlock(devType, context, parent, localId, className)
    {
    }


};

END_NAMESPACE_OPENDAQ
