#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

namespace permissions
{
    static const auto DefaultPropertyObjectPermissions = []
    {
        printf("DefaultPropertyObjectPermissions\n");
        return PermissionsBuilder().assign("everyone", PermissionMaskBuilder().read().write().execute()).build();
    }();

    void GetDefaultPropertyObjectPermissions(IPermissions** permissions)
    {
        if (permissions)
            *permissions = DefaultPropertyObjectPermissions.addRefAndReturn();
    }
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, PropertyObject)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyObject,
    IPropertyObject, createPropertyObjectWithClassAndManager,
    ITypeManager*, manager,
    IString*, className
)


END_NAMESPACE_OPENDAQ
