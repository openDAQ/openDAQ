#include <coreobjects/errors.h>
#include <opendaq/module_authenticator_impl.h>
#include <iostream>

BEGIN_NAMESPACE_OPENDAQ

ModuleAuthenticatorImpl::ModuleAuthenticatorImpl(IString* certPath)
    : certificatePath(certPath)
{
}

ErrCode INTERFACE_FUNC ModuleAuthenticatorImpl::authenticateModuleBinary(Bool* binaryValid, IString* binaryPath)
{
    OPENDAQ_PARAM_NOT_NULL(binaryValid);
    OPENDAQ_PARAM_NOT_NULL(binaryPath);

    Bool valid;
    const ErrCode errCode = wrapHandlerReturn(this, &ModuleAuthenticatorImpl::onAuthenticateModuleBinary, valid, binaryPath);
    OPENDAQ_RETURN_IF_FAILED(errCode);

    *binaryValid = valid;
    return errCode;
}

Bool ModuleAuthenticatorImpl::onAuthenticateModuleBinary(IString* binaryPath)
{
    std::cerr << std::endl << "ModuleAuthenticator::onAuthenticateModuleBinary" << std::endl;

    return true;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, ModuleAuthenticator, IString*, certPath)

END_NAMESPACE_OPENDAQ
