#include <coreobjects/util.h>

#include <coreobjects/property_object_impl.h>
#include <coretypes/errors.h>

BEGIN_NAMESPACE_OPENDAQ

extern "C"
ErrCode daqInitializeCoreObjectsTesting()
{
    auto permissions = daq::object_utils::UnrestrictedPermissions;

    auto permissionManager = PermissionManager();
    permissionManager.setPermissions(object_utils::UnrestrictedPermissions);

    return OPENDAQ_SUCCESS;
}

END_NAMESPACE_OPENDAQ
