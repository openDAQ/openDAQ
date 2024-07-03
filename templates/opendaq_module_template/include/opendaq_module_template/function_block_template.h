#pragma once
#include <opendaq/function_block_impl.h>
#include <opendaq_module_template/component_template_base.h>

BEGIN_NAMESPACE_OPENDAQ

class TemplateFunctionBlock : public FunctionBlock, ComponentTemplateBase
{
    TemplateFunctionBlock(const FunctionBlockTypePtr& devType,
                          const ContextPtr& context,
                          const ComponentPtr& parent,
                          const StringPtr& localId,
                          const StringPtr& className = nullptr);

};

END_NAMESPACE_OPENDAQ
