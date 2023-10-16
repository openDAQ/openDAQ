#include "opcuaclient/opcuaasyncexecthread.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA

OpcUaAsyncExecThread::OpcUaAsyncExecThread()
{
}

OpcUaAsyncExecThread::OpcUaAsyncExecThread(std::thread&& thread)
    : thread(std::move(thread))
{
}

OpcUaAsyncExecThread::~OpcUaAsyncExecThread()
{
    joinThread();
}

void OpcUaAsyncExecThread::reset()
{
    joinThread();
}

void OpcUaAsyncExecThread::joinThread()
{
    if (thread.joinable())
        thread.join();
}

OpcUaAsyncExecThread& OpcUaAsyncExecThread::operator=(std::thread&& thread)
{
    joinThread();
    this->thread = std::move(thread);
    return *this;
}

END_NAMESPACE_OPENDAQ_OPCUA
