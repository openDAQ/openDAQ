#include <opendaq/component_impl.h>

BEGIN_NAMESPACE_OPENDAQ

namespace permissions
{
    void GetDefaultComponentPermissions(IPermissions** permissions)
    {
        static auto DefaultComponentPermissions = PermissionsBuilder().inherit(true).build();
        if (permissions)
            *permissions = DefaultComponentPermissions.addRefAndReturn();
    }
}

using ComponentInternalImpl = ComponentImpl<IComponent>;

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, ComponentInternal, IComponent, createComponent,
    IContext*, context,
    IComponent*, parent,
    IString*, localId,
    IString*, className)

END_NAMESPACE_OPENDAQ
