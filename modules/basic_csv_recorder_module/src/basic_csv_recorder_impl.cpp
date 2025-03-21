#include <filesystem>
#include <functional>
#include <memory>
#include <set>
#include <string>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <basic_csv_recorder_module/basic_csv_recorder_impl.h>
#include <basic_csv_recorder_module/basic_csv_recorder_thread.h>
#include <basic_csv_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE

FunctionBlockTypePtr BasicCsvRecorderImpl::createType()
{
    return FunctionBlockType(
        TYPE_ID,
        "BasicCsvRecorder",
        "Basic CSV recording functionality",
        PropertyObject());
}

BasicCsvRecorderImpl::BasicCsvRecorderImpl(
    const ContextPtr& context,
    const ComponentPtr& parent,
    const StringPtr& localId,
    const PropertyObjectPtr& config
)
    : FunctionBlockImpl<IFunctionBlock, IRecorder>(createType(), context, parent, localId, nullptr)
{
    this->tags.add(Tags::RECORDER);

    addInputPort();
    addProperties();
}

ErrCode BasicCsvRecorderImpl::startRecording()
{
    recordingActive = true;
    reconfigure();
    return OPENDAQ_SUCCESS;
}

ErrCode BasicCsvRecorderImpl::stopRecording()
{
    recordingActive = false;
    reconfigure();
    return OPENDAQ_SUCCESS;
}

ErrCode BasicCsvRecorderImpl::getIsRecording(Bool *isRecording)
{
    OPENDAQ_PARAM_NOT_NULL(isRecording);

    *isRecording = recordingActive;

    return OPENDAQ_SUCCESS;
}

void BasicCsvRecorderImpl::onConnected(const InputPortPtr& port)
{
    addInputPort();
    reconfigure();
}

void BasicCsvRecorderImpl::onDisconnected(const InputPortPtr& port)
{
    while (portCount >= 2)
    {
        auto ports = objPtr.template asPtr<IFunctionBlock>(true).getInputPorts();
        if (ports.getItemAt(portCount - 1).getConnection().assigned())
            break;
        if (ports.getItemAt(portCount - 2).getConnection().assigned())
            break;

        removeInputPort(ports.getItemAt(--portCount));
    }

    reconfigure();
}

void BasicCsvRecorderImpl::activeChanged()
{
    if (!active)
        stopRecording();
}

void BasicCsvRecorderImpl::onPacketReceived(const InputPortPtr& port)
{
    auto thread = findThreadForSignal(port);

    PacketPtr packet;
    while ((packet = port.getConnection().dequeue()).assigned())
        if (thread)
            thread->post(packet);
}

void BasicCsvRecorderImpl::addProperties()
{
    objPtr.addProperty(StringProperty(Props::PATH, ""));
    objPtr.getOnPropertyValueWrite(Props::PATH) += std::bind(&BasicCsvRecorderImpl::reconfigure, this);
}

void BasicCsvRecorderImpl::addInputPort()
{
    auto c = createAndAddInputPort("Value" + std::to_string(++portCount), PacketReadyNotification::SameThread);
}

void BasicCsvRecorderImpl::reconfigure()
{
    std::filesystem::path path = static_cast<std::string>(objPtr.getPropertyValue(Props::PATH));

    if (recordingActive)
    {
        // We will update the 'signals' map by emplacing new BasicCsvRecorderSignal objects for
        // newly-connected input ports, and destroying BasicCsvRecorderSignal objects for ports
        // that are gone or no longer connected. Notably, we will not disturb
        // BasicCsvRecorderSignal objects for input ports that have not changed.

        // Keep track of the currently-connected input ports we have seen, so we can later destroy
        // BasicCsvRecorderSignal objects for ports that are gone or no longer connected.
        std::set<IInputPort *> ports;

        auto inputPorts = borrowPtr<FunctionBlockPtr>().getInputPorts();
        for (const auto& inputPort : inputPorts)
        {
            auto connection = inputPort.getConnection();
            if (connection.assigned())
            {
                ports.emplace(inputPort.getObject());

                SignalPtr signal = connection.getSignal();

                // If we don't yet have a BasicCsvRecorderSignal object for this port, create one.
                auto it = threads.find(inputPort.getObject());
                if (it == threads.end())
                    threads.emplace(
                        inputPort.getObject(),
                        std::make_shared<BasicCsvRecorderThread>(path, signal, loggerComponent));
            }
        }

        // Now make another pass, and destroy BasicCsvRecorderSignal
        // objects for ports that are gone or no longer connected.
        decltype(threads)::iterator it = threads.begin();
        while (it != threads.end())
        {
            if (ports.find(it->first) == ports.end())
            {
                auto jt = it;
                ++jt;
                threads.erase(it);
                it = jt;
            }

            else
            {
                ++it;
            }
        }
    }

    else
    {
        signals.clear();
    }
}

std::shared_ptr<BasicCsvRecorderThread> BasicCsvRecorderImpl::findThreadForSignal(IInputPort *port)
{
    std::shared_ptr<BasicCsvRecorderThread> thread;
    auto lock = getRecursiveConfigLock();
    auto it = threads.find(port);
    if (it != threads.end())
        thread = it->second;
    return thread;
}

END_NAMESPACE_OPENDAQ_BASIC_CSV_RECORDER_MODULE
