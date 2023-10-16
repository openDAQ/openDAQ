#include "opcuaclient/opcuatimertaskhelper.h"
#include <opendaq/utils/finally.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

using namespace daq::utils;

OpcUaTimerTaskHelper::OpcUaTimerTaskHelper(OpcUaClient& client, int intervalMs, const CallbackFunction& callback)
    : intervalMs(intervalMs)
    , terminated{}
    , client(client)
    , callback(callback)
{
    assert(callback);
}

OpcUaTimerTaskHelper::~OpcUaTimerTaskHelper()
{
    stop();
}

void OpcUaTimerTaskHelper::start()
{
    if (!getStarted())
    {
        terminated = false;

        using namespace std::placeholders;
        callbackIdent = client.scheduleTimerTask(intervalMs, std::bind(&OpcUaTimerTaskHelper::execute, this, _1, _2));
    }
}

void OpcUaTimerTaskHelper::stop()
{
    if (getStarted())
    {
        terminated = true;
        client.removeTimerTask(callbackIdent.value());
    }
    callbackIdent.reset();
}

double OpcUaTimerTaskHelper::getIntervalMs() const
{
    return intervalMs;
}

void OpcUaTimerTaskHelper::setIntervalMs(const double value)
{
    intervalMs = value;
}

void OpcUaTimerTaskHelper::terminate()
{
    terminated = true;
}

const std::atomic<bool>& OpcUaTimerTaskHelper::getTerminated() const
{
    return terminated;
}

bool OpcUaTimerTaskHelper::getStarted() const
{
    return callbackIdent.has_value();
}

void OpcUaTimerTaskHelper::execute(OpcUaClient& client, TimerTaskControl& control)
{
    daq::utils::Finally checkTerminate(
        [this, &control]()
        {
            if (this->terminated)
                control.terminateTimerTask();
        });

    callback(client);
}

END_NAMESPACE_OPENDAQ_OPCUA
