#include <sftp_file_transfer/libssh2/sftp_file_transfer_impl.h>

#include <sftp_file_transfer/sftp_arguments.h>
#include <opendaq/custom_log.h>

namespace daq::sftp::libssh2
{

SftpFileTransferImpl::SftpFileTransferImpl(const StringPtr& address,
                                           const AuthenticationConfigPtr& authenticationConfig,
                                           const LoggerPtr& logger,
                                           const PropertyObjectPtr& config)
{
    validateArguments(address, authenticationConfig, logger, config);
    loggerComponent = logger.getOrAddComponent("SftpFileTransferLibssh2");

    ConnectionProperties properties;
    properties.chunkSize = static_cast<std::size_t>(readIntPropertyValue(config, "ChunkSize"));
    properties.ciphers = readStringPropertyValue(config, "Ciphers");

    connection = std::make_unique<SftpConnection>(parseEndpoint(address), authenticationConfig, properties);

    if (!connection->hostKeyVerified())
        DAQLOGF_W(loggerComponent,
                  "Connected to {} WITHOUT host key verification: the server identity was not checked and the connection "
                  "may be intercepted",
                  address);
}

ErrCode SftpFileTransferImpl::upload(IString* localPath, IString* remotePath)
{
    OPENDAQ_PARAM_NOT_NULL(localPath);
    OPENDAQ_PARAM_NOT_NULL(remotePath);

    return daqTry([&]
    {
        std::scoped_lock lock(connectionSync);
        connection->upload(localPath, remotePath);
    });
}

ErrCode SftpFileTransferImpl::download(IString* remotePath, IString* localPath)
{
    OPENDAQ_PARAM_NOT_NULL(remotePath);
    OPENDAQ_PARAM_NOT_NULL(localPath);

    return daqTry([&]
    {
        std::scoped_lock lock(connectionSync);
        connection->download(remotePath, localPath);
    });
}

}

BEGIN_NAMESPACE_OPENDAQ

// The create function behind SftpFileTransferLibssh2 (sftp_file_transfer_libssh2_factory.h).
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    INTERNAL_FACTORY, sftp::libssh2::SftpFileTransferImpl, IFileTransfer, createSftpFileTransferLibssh2,
    IString*, address,
    IAuthenticationConfig*, authenticationConfig,
    ILogger*, logger,
    IPropertyObject*, config
)

END_NAMESPACE_OPENDAQ
