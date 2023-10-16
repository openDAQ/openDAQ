#include <opendaq/logger_thread_pool_impl.h>

#include <coretypes/impl.h>

BEGIN_NAMESPACE_OPENDAQ

LoggerThreadPoolImpl::LoggerThreadPoolImpl() :
    spdlogThreadPool(std::make_shared<ThreadPool>(8192, 1, []{}))
{
}

ErrCode LoggerThreadPoolImpl::getThreadPoolImpl(ThreadPoolPtr *impl)
{
    if (impl == nullptr)
    {
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Can not return by a null pointer.");
    }
    *impl = spdlogThreadPool;
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, LoggerThreadPool)

END_NAMESPACE_OPENDAQ
