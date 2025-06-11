#include <properties_module/properties_fb_3.h>

BEGIN_NAMESPACE_PROPERTIES_MODULE

PropertiesFb3::PropertiesFb3(const ContextPtr& ctx, const ComponentPtr& par, const StringPtr& locId)
    : FunctionBlock(CreateType(), ctx, par, locId)
{
    initProperties();
}

void PropertiesFb3::initProperties()
{
    
}

FunctionBlockTypePtr PropertiesFb3::CreateType()
{
    return FunctionBlockType("PropertiesFb3", "Properties3", "Function Block focused on Properties 3");
}

END_NAMESPACE_PROPERTIES_MODULE
