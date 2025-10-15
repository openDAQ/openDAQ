#include <licensing_example/module_authenticator_impl_linux.h>

#include <iostream>

BEGIN_NAMESPACE_OPENDAQ

ModuleAuthenticatorImpl::ModuleAuthenticatorImpl(const StringPtr& certPath)
    : logger(nullptr)
    , loggerComponent(nullptr)
{
    std::string pathStr = certPath.toStdString();
    certsPath = std::filesystem::path(pathStr);
}

Bool ModuleAuthenticatorImpl::onAuthenticateModuleBinary(StringPtr& vendorKey, const StringPtr& binaryPath)
{
    if (vendorKey != nullptr && binaryPath != nullptr)
        return true;
    return false;
}

Bool ModuleAuthenticatorImpl::onSetLogger(const LoggerPtr& logger)
{
    this->logger = logger;
    this->loggerComponent = logger.addComponent("ModuleAuthenticator");

    return true;
}

END_NAMESPACE_OPENDAQ
