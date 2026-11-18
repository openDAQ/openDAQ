#include <sftp_file_transfer/libssh2/sftp_file_transfer_operation_impl.h>

#include <sftp_file_transfer/file_transfer_status.h>
#include <coretypes/enumeration_factory.h>
#include <coretypes/integer_factory.h>
#include <coretypes/procedure_factory.h>
#include <opendaq/component_status_container_impl.h>
#include <opendaq/component_status_container_private_ptr.h>
#include <opendaq/custom_log.h>

namespace daq::sftp::libssh2
{

namespace detail
{

bool isFinal(TransferStatus status)
{
    return status == TransferStatus::Completed || status == TransferStatus::Failed || status == TransferStatus::Canceled;
}

}

SftpFileTransferOperationImpl::SftpFileTransferOperationImpl(const std::weak_ptr<SftpAsyncEngine>& engine,
                                                             bool upload,
                                                             const StringPtr& localPath,
                                                             const StringPtr& remotePath,
                                                             const EnumerationTypePtr& statusType,
                                                             const EventPtr<const BaseObjectPtr, const CoreEventArgsPtr>& connectionEvent,
                                                             const LoggerComponentPtr& loggerComponent)
    : engine(engine)
    , upload(upload)
    , local(localPath)
    , remote(remotePath)
    , statusType(statusType)
    , connectionEvent(connectionEvent)
    , loggerComponent(loggerComponent)
{
    // The container calls the procedure on every change of a status or its message (as for components, which
    // forward it to the context's core event).
    statusContainer = createWithImplementation<IComponentStatusContainer, ComponentStatusContainerImpl>(
        Procedure([this](const CoreEventArgsPtr& args) { statusChanged(args); }));
    statusContainer.asPtr<IComponentStatusContainerPrivate>().addStatus(
        FileTransferStatusName, EnumerationWithIntValueAndType(statusType, Integer(static_cast<Int>(TransferStatus::Created))));
}

ErrCode SftpFileTransferOperationImpl::getStatusContainer(IComponentStatusContainer** statusContainer)
{
    OPENDAQ_PARAM_NOT_NULL(statusContainer);
    *statusContainer = this->statusContainer.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferOperationImpl::getOnStatusChanged(IEvent** event)
{
    OPENDAQ_PARAM_NOT_NULL(event);
    *event = onStatusChanged.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferOperationImpl::start()
{
    {
        std::scoped_lock lock(sync);
        if (started || currentStatus != TransferStatus::Created)
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The transfer was already started or canceled");
        started = true;
    }

    const auto owner = engine.lock();
    if (!owner || !owner->start(this))
    {
        std::scoped_lock lock(sync);
        started = false;
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The file transfer object was released");
    }
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferOperationImpl::pause()
{
    if (const auto owner = engine.lock())
        owner->pause(this);
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferOperationImpl::resume()
{
    if (const auto owner = engine.lock())
        owner->resume(this);
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferOperationImpl::cancel()
{
    return daqTry([&]
    {
        // An operation that was not started is canceled right here, on the calling thread, so a wait right after
        // returns. Marking it started keeps a concurrent start from going ahead.
        bool notStarted;
        {
            std::scoped_lock lock(sync);
            notStarted = !started;
            started = true;
        }
        if (notStarted)
        {
            setStatus(TransferStatus::Canceled);
            return;
        }

        // A started one is canceled by the engine; if it is shut down, it has canceled the transfer already.
        if (const auto owner = engine.lock())
            owner->cancel(this);
    });
}

ErrCode SftpFileTransferOperationImpl::wait()
{
    if (const auto owner = engine.lock(); owner && owner->onIoThread())
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Cannot wait for a transfer on the file transfer I/O thread");

    std::unique_lock lock(sync);
    if (!started && currentStatus == TransferStatus::Created)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "The transfer was not started");

    finishedChanged.wait(lock, [this] { return finished; });
    if (currentStatus == TransferStatus::Failed)
        return DAQ_MAKE_ERROR_INFO(errorCode, errorMessage.toStdString());
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferOperationImpl::getLocalPath(IString** path)
{
    OPENDAQ_PARAM_NOT_NULL(path);
    *path = local.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferOperationImpl::getRemotePath(IString** path)
{
    OPENDAQ_PARAM_NOT_NULL(path);
    *path = remote.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferOperationImpl::getBytesTransferred(SizeT* bytes)
{
    OPENDAQ_PARAM_NOT_NULL(bytes);
    *bytes = static_cast<SizeT>(bytesTransferred.load());
    return OPENDAQ_SUCCESS;
}

ErrCode SftpFileTransferOperationImpl::getTotalBytes(SizeT* bytes)
{
    OPENDAQ_PARAM_NOT_NULL(bytes);
    *bytes = static_cast<SizeT>(totalBytes.load());
    return OPENDAQ_SUCCESS;
}

bool SftpFileTransferOperationImpl::isUpload() const noexcept
{
    return upload;
}

StringPtr SftpFileTransferOperationImpl::localPath() const
{
    return local;
}

StringPtr SftpFileTransferOperationImpl::remotePath() const
{
    return remote;
}

TransferStatus SftpFileTransferOperationImpl::status() const
{
    std::scoped_lock lock(sync);
    return currentStatus;
}

void SftpFileTransferOperationImpl::setTotalBytes(std::uint64_t bytes) noexcept
{
    totalBytes = bytes;
}

void SftpFileTransferOperationImpl::setBytesTransferred(std::uint64_t bytes) noexcept
{
    bytesTransferred = bytes;
}

void SftpFileTransferOperationImpl::setStatus(TransferStatus status, ErrCode errorCode, const StringPtr& message)
{
    {
        std::scoped_lock lock(sync);
        if (detail::isFinal(currentStatus))
            return;
        currentStatus = status;
        this->errorCode = errorCode;
        errorMessage = message;
    }

    // Outside the lock: the handlers may use the operation.
    try
    {
        statusContainer.asPtr<IComponentStatusContainerPrivate>().setStatusWithMessage(
            FileTransferStatusName, EnumerationWithIntValueAndType(statusType, Integer(static_cast<Int>(status))), message);
    }
    catch (const std::exception& e)
    {
        DAQLOGF_W(loggerComponent, "Setting the status of the transfer of \"{}\" failed: {}", remote, e.what());
    }

    if (detail::isFinal(status))
    {
        {
            std::scoped_lock lock(sync);
            finished = true;
        }
        finishedChanged.notify_all();
    }
}

void SftpFileTransferOperationImpl::statusChanged(const CoreEventArgsPtr& args)
{
    // A failing handler is logged and does not keep the other handlers or the transfer from going on.
    FileTransferOperationPtr operation = this->template borrowPtr<FileTransferOperationPtr>();
    try
    {
        onStatusChanged(operation, args);
    }
    catch (const std::exception& e)
    {
        DAQLOGF_W(loggerComponent, "A status change handler of the transfer of \"{}\" failed: {}", remote, e.what());
    }

    if (!connectionEvent.assigned())
        return;
    BaseObjectPtr sender = operation;
    try
    {
        connectionEvent(sender, args);
    }
    catch (const std::exception& e)
    {
        DAQLOGF_W(loggerComponent, "A status change handler of the file transfer object failed: {}", e.what());
    }
}

}
