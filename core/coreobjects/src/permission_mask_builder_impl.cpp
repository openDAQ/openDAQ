#include <coreobjects/permission_mask_builder_impl.h>
#include <coretypes/impl.h>
#include <coretypes/validation.h>

BEGIN_NAMESPACE_OPENDAQ

PermissionMaskBuilderImpl::PermissionMaskBuilderImpl(Int permissionMask)
    : permissionMask(permissionMask)
{
}

PermissionMaskBuilderImpl::PermissionMaskBuilderImpl()
    : PermissionMaskBuilderImpl(0)
{
}

ErrCode INTERFACE_FUNC PermissionMaskBuilderImpl::read()
{
    permissionMask |= (Int) Permission::Read;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionMaskBuilderImpl::write()
{
    permissionMask |= (Int) Permission::Write;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionMaskBuilderImpl::execute()
{
    permissionMask |= (Int) Permission::Execute;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionMaskBuilderImpl::clear()
{
    permissionMask = 0;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC PermissionMaskBuilderImpl::build(Int* permissionMask)
{
    OPENDAQ_PARAM_NOT_NULL(permissionMask);

    *permissionMask = this->permissionMask;
    return OPENDAQ_SUCCESS;
}

// Factories

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                               PermissionMaskBuilderImpl,
                                                               IPermissionMaskBuilder,
                                                               createPermissionMaskBuilder)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(LIBRARY_FACTORY,
                                                               PermissionMaskBuilderImpl,
                                                               IPermissionMaskBuilder,
                                                               createPermissionMaskBuilderFromMask,
                                                               Int,
                                                               permissionMask)

END_NAMESPACE_OPENDAQ
