#include <opendaq/component_impl.h>

BEGIN_NAMESPACE_OPENDAQ

using ComponentInternalImpl = ComponentImpl<IComponent>;

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ComponentInternal, IComponent, createComponent,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    IString*, className)

#if !defined(BUILDING_STATIC_LIBRARY)
extern "C" daq::ErrCode PUBLIC_EXPORT createComponentWithDefaultPropertyMode(
    IComponent** objTmp,
    IContext* context,
    IComponent* parent,
    IString* localId,
    IString* className,
    Int propertyMode)
{
    return daq::createObject<IComponent, ComponentInternalImpl, IContext*, IComponent*, IString*, IString*, ComponentStandardProps>(
        objTmp,
        context,
        parent,
        localId,
        className,
        static_cast<ComponentStandardProps>(propertyMode));
}
#endif

END_NAMESPACE_OPENDAQ
