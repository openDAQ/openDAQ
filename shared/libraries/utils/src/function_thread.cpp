#include <opendaq/utils/function_thread.h>
#include <utility>
#include <algorithm>

BEGIN_NAMESPACE_UTILS

FunctionThread::FunctionThread(CallbackFunction callback)
    : callback{std::move(callback)}
{
}

const FunctionThread::CallbackFunction& FunctionThread::getCallback() const
{
    return callback;
}

void FunctionThread::setCallback(const CallbackFunction& value)
{
    callback = value;
}

void FunctionThread::execute()
{
    if (callback)
        callback();
}

END_NAMESPACE_UTILS
