#include <opendaq/awaitable_impl.h>
#include <opendaq/scheduler_errors.h>
#include <coretypes/objectptr.h>

#include <chrono>
#include <future>

BEGIN_NAMESPACE_OPENDAQ

namespace
{
    // Sets the flag once the guarded scope is left, including during stack unwinding.
    struct CompletionGuard
    {
        explicit CompletionGuard(std::atomic<bool>& completed)
            : completed(completed)
        {
        }

        ~CompletionGuard()
        {
            completed = true;
        }

        std::atomic<bool>& completed;
    };
}

template <typename TReturn>
AwaitableImpl<TReturn>::AwaitableImpl(Future future)
    : future(std::move(future))
    , completed(false)
{
}

template <typename TReturn>
ErrCode AwaitableImpl<TReturn>::cancel(Bool* canceled)
{
    *canceled = future.cancel();
    return OPENDAQ_SUCCESS;
}

template <typename TReturn>
ErrCode AwaitableImpl<TReturn>::wait()
{
    if (completed)
    {
        return OPENDAQ_IGNORED;
    }

    if (!future.valid())
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_EMPTY_AWAITABLE);
    }

    OPENDAQ_TRY(
        future.wait();
        completed = true; 
    )

    return OPENDAQ_SUCCESS;
}

template <typename TReturn>
ErrCode AwaitableImpl<TReturn>::getResult(daq::IBaseObject** result)
{
    OPENDAQ_PARAM_NOT_NULL(result);

    if (!completed && !future.valid())
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_EMPTY_AWAITABLE);
    }

    if constexpr (std::is_void_v<TReturn>)
    {
        try
        {
            CompletionGuard guard(completed);
            future.get();
        }
        catch (...)
        {
            // Mask exceptions from taskflow - don't rethrow
            // This matches the behavior expected by ScheduleGraphMasksExceptions test
        }

        *result = nullptr;
    }
    else
    {
        std::optional<BaseObjectPtr> optional;
        OPENDAQ_TRY(
            CompletionGuard guard(completed);
            optional = future.get();
        )

        if (!optional.has_value())
        {
            *result = nullptr;
        }
        else
        {
            *result = optional->addRefAndReturn();
        }
    }
    return OPENDAQ_SUCCESS;
}

template <typename TReturn>
ErrCode AwaitableImpl<TReturn>::hasCompleted(Bool* finished)
{
    OPENDAQ_PARAM_NOT_NULL(finished);

    if (!future.valid())
        *finished = this->completed.load();
    else
        *finished = future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    return OPENDAQ_SUCCESS;
}

template class AwaitableImpl<void>;
template class AwaitableImpl<std::optional<ObjectPtr<IBaseObject>>>;

END_NAMESPACE_OPENDAQ
