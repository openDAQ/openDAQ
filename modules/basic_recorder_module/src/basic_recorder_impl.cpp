#include <filesystem>
#include <functional>
#include <memory>
#include <set>
#include <string>

#include <opendaq/function_block_impl.h>
#include <opendaq/opendaq.h>

#include <basic_recorder_module/basic_recorder_impl.h>
#include <basic_recorder_module/common.h>

BEGIN_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE

daq::FunctionBlockTypePtr BasicRecorderImpl::createType()
{
    return daq::FunctionBlockType(
        TYPE_ID,
        "BasicRecorder",
        "Basic CSV recording functionality",
        daq::PropertyObject());
}

BasicRecorderImpl::BasicRecorderImpl(
    const daq::ContextPtr& context,
    const daq::ComponentPtr& parent,
    const daq::StringPtr& localId,
    const daq::PropertyObjectPtr& config
)
    : daq::FunctionBlock(createType(), context, parent, localId, nullptr)
{
    this->tags.add(Tags::RECORDER);

    addInputPort();
    addProperties();
}

void BasicRecorderImpl::startRecording()
{
    objPtr.setPropertyValue(Props::RECORDING_ACTIVE, true);
}

void BasicRecorderImpl::stopRecording()
{
    objPtr.setPropertyValue(Props::RECORDING_ACTIVE, false);
}

void BasicRecorderImpl::onConnected(const daq::InputPortPtr& port)
{
    addInputPort();
    reconfigure();
}

void BasicRecorderImpl::onDisconnected(const daq::InputPortPtr& port)
{
    while (portCount >= 2)
    {
        auto ports = objPtr.template asPtr<daq::IFunctionBlock>(true).getInputPorts();
        if (ports.getItemAt(portCount - 1).getConnection().assigned())
            break;
        if (ports.getItemAt(portCount - 2).getConnection().assigned())
            break;

        removeInputPort(ports.getItemAt(--portCount));
    }

    reconfigure();
}

void BasicRecorderImpl::onPacketReceived(const daq::InputPortPtr& port)
{
    if (auto signal = findSignal(port))
        signal->onPacketReceived(port);
}

void BasicRecorderImpl::addProperties()
{
    objPtr.addProperty(daq::StringProperty(Props::PATH, ""));
    objPtr.addProperty(daq::BoolProperty(Props::RECORDING_ACTIVE, false));

    auto reconfigure = std::bind(&BasicRecorderImpl::reconfigure, this);

    objPtr.getOnPropertyValueWrite(Props::PATH) += reconfigure;
    objPtr.getOnPropertyValueWrite(Props::RECORDING_ACTIVE) += reconfigure;
}

void BasicRecorderImpl::addInputPort()
{
    auto c = createAndAddInputPort("Value" + std::to_string(++portCount), daq::PacketReadyNotification::SameThread);
}

void BasicRecorderImpl::reconfigure()
{
    std::filesystem::path path = static_cast<std::string>(objPtr.getPropertyValue(Props::PATH));
    bool recordingActive = objPtr.getPropertyValue(Props::RECORDING_ACTIVE);

    if (recordingActive)
    {
        std::set<daq::IInputPort *> ports;

        auto inputPorts = borrowPtr<daq::FunctionBlockPtr>().getInputPorts();
        for (const auto& inputPort : inputPorts)
        {
            auto connection = inputPort.getConnection();
            if (connection.assigned())
            {
                ports.emplace(inputPort.getObject());

                daq::SignalPtr signal = connection.getSignal();

                auto it = signals.find(inputPort.getObject());
                if (it == signals.end())
                    signals.emplace(
                        inputPort.getObject(),
                        std::make_shared<BasicRecorderSignal>(path, signal));
            }
        }

        decltype(signals)::iterator it = signals.begin();
        while (it != signals.end())
        {
            if (ports.find(it->first) == ports.end())
            {
                auto jt = it;
                ++jt;
                signals.erase(it);
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

std::shared_ptr<BasicRecorderSignal> BasicRecorderImpl::findSignal(daq::IInputPort *port)
{
    std::shared_ptr<BasicRecorderSignal> signal;
    auto lock = getRecursiveConfigLock();
    auto it = signals.find(port);
    if (it != signals.end())
        signal = it->second;
    return signal;
}

END_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE
