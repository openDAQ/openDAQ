#include <sftp_file_transfer/libssh2/sftp_file_transfer_async_impl.h>
#include <sftp_file_transfer/libssh2/sftp_file_transfer_operation_impl.h>

#include <sftp_file_transfer/file_transfer_status.h>
#include <sftp_file_transfer/sftp_arguments.h>
#include <coretypes/enumeration_factory.h>
#include <coretypes/integer_factory.h>
#include <coretypes/procedure_factory.h>
#include <opendaq/component_status_container_impl.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/custom_log.h>

namespace daq::sftp::libssh2
{

SftpFileTransferAsyncImpl::SftpFileTransferAsyncImpl(const StringPtr& address,
                                                     const AuthenticationConfigPtr& authenticationConfig,
                                                     const LoggerPtr& logger,
                                                     const PropertyObjectPtr& config)
{
    validateArguments(address, authenticationConfig, logger, config);
    loggerComponent = logger.getOrAddComponent("SftpFileTransferAsyncLibssh2");

    AsyncConnectionProperties properties;
    properties.chunkSize = static_cast<std::size_t>(readIntPropertyValue(config, "ChunkSize"));
    properties.ciphers = readStringPropertyValue(config, "Ciphers");
    properties.maxActiveTransfers = static_cast<std::size_t>(readIntPropertyValue(config, "MaxActiveTransfers"));
    properties.connectionTimeout = std::chrono::milliseconds(readIntPropertyValue(config, "ConnectionTimeout"));
    properties.keepaliveInterval = std::chrono::milliseconds(readIntPropertyValue(config, "KeepaliveInterval"));
    properties.reconnectAttempts = static_cast<std::size_t>(readIntPropertyValue(config, "ReconnectAttempts", 0));
    properties.reconnectInterval = std::chrono::milliseconds(readIntPropertyValue(config, "ReconnectInterval"));

    transferStatusType = FileTransferStatusType();
    connectionStatusType = FileTransferConnectionStatusType();

    statusContainer = createWithImplementation<IComponentStatusContainer, ComponentStatusContainerImpl>(
        Procedure([this](const CoreEventArgsPtr& args) { statusChanged(args); }));
    statusContainer.asPtr<IComponentStatusContainerPrivate>().addStatus(
        ConnectionStatusName,
        EnumerationWithIntValueAndType(connectionStatusType, Integer(static_cast<Int>(ConnectionStatus::Connected))));

    engine = SftpAsyncEngine::create(parseEndpoint(address),
                                     authenticationConfig,
                                     properties,
                                     loggerComponent,
                                     [this](ConnectionStatus status, const StringPtr& message) { connectionStatusChanged(status, message); });

    if (!engine->hostKeyVerified())
        DAQLOGF_W(loggerComponent,
                  "Connected to {} WITHOUT host key verification: the server identity was not checked and the connection "
                  "may be intercepted",
                  address);
}

SftpFileTransferAsyncImpl::~SftpFileTransferAsyncImpl()
{
    engine->shutdown();
}

ErrCode SftpFileTransferAsyncImpl::getStatusContainer(IComponentStatusContainer** statusContainer)
{
    OPENDAQ_PARAM_NOT_NULL(statusContainer);
    *statusContainer = this->statusContainer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferAsyncImpl::getOnStatusChanged(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);
    *event = onStatusChanged.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferAsyncImpl::upload(IString* localPath, IString* remotePath, IFileTransferOperation** operation)
{
    return createOperation(true, localPath, remotePath, operation);
}

ErrCode SftpFileTransferAsyncImpl::download(IString* remotePath, IString* localPath, IFileTransferOperation** operation)
{
    return createOperation(false, localPath, remotePath, operation);
}

ErrCode SftpFileTransferAsyncImpl::createOperation(bool upload,
                                                   IString* localPath,
                                                   IString* remotePath,
                                                   IFileTransferOperation** operation)
{
    OPENDAQ_PARAM_NOT_NULL(localPath);
    OPENDAQ_PARAM_NOT_NULL(remotePath);
    OPENDAQ_PARAM_NOT_NULL(operation);

    return daqTry([&]
    {
        *operation = createWithImplementation<IFileTransferOperation, SftpFileTransferOperationImpl>(
                         std::weak_ptr<SftpAsyncEngine>(engine),
                         upload,
                         StringPtr(localPath),
                         StringPtr(remotePath),
                         transferStatusType,
                         onStatusChanged,
                         loggerComponent)
                         .detach();
    });
}

void SftpFileTransferAsyncImpl::connectionStatusChanged(ConnectionStatus status, const StringPtr& message)
{
    statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatusWithMessage(
        ConnectionStatusName,
        EnumerationWithIntValueAndType(connectionStatusType, Integer(static_cast<Int>(status))),
        message);
}

void SftpFileTransferAsyncImpl::statusChanged(const CoreEventArgsPtr& args)
{
    BaseObjectPtr sender = this->template borrowPtr<BaseObjectPtr>();
    try
    {
        onStatusChanged(sender, args);
    }
    catch (const std::exception& e)
    {
        DAQLOGF_W(loggerComponent, "A status change handler of the file transfer object failed: {}", e.what());
    }
}

}

BEGIN_NAMESPACE_OPENDAQ

// The create function behind SftpFileTransferAsyncLibssh2 (sftp_file_transfer_async_libssh2_factory.h).
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    INTERNAL_FACTORY, sftp::libssh2::SftpFileTransferAsyncImpl, IFileTransferAsync, createSftpFileTransferAsyncLibssh2,
    IString*, address,
    IAuthenticationConfig*, authenticationConfig,
    ILogger*, logger,
    IPropertyObject*, config
)

END_NAMESPACE_OPENDAQ
