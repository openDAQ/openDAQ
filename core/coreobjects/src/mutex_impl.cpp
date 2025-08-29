#include <coreobjects/mutex_impl.h>
#include <fmt/format.h>
#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

ErrCode MutexImpl::lock()
{
    try
    {
        this->mutex.lock();
    }
    catch (std::exception& e)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to lock mutex: {}", e.what()));
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MutexImpl::tryLock(Bool* succeeded)
{
    try
    {
        *succeeded = this->mutex.try_lock();
    }
    catch (std::exception& e)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to try_lock mutex: {}", e.what()));
    }

    return OPENDAQ_SUCCESS;
}

ErrCode MutexImpl::unlock()
{
    try
    {
        this->mutex.unlock();
    }
    catch (std::exception& e)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_GENERALERROR, fmt::format("Failed to unlock mutex: {}", e.what()));
    }

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Mutex)

END_NAMESPACE_OPENDAQ
