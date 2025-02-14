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

FunctionBlockTypePtr BasicRecorderImpl::createType()
{
    return FunctionBlockType(
        TYPE_ID,
        "BasicRecorder",
        "Basic CSV recording functionality",
        PropertyObject());
}

BasicRecorderImpl::BasicRecorderImpl(
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

ErrCode BasicRecorderImpl::startRecording()
{
    return setPropertyValue(
        String(Props::RECORDING_ACTIVE).getObject(),
        Boolean(true).getObject());
}

ErrCode BasicRecorderImpl::stopRecording()
{
    return setPropertyValue(
        String(Props::RECORDING_ACTIVE).getObject(),
        Boolean(false).getObject());
}

void BasicRecorderImpl::onConnected(const InputPortPtr& port)
{
    addInputPort();
    reconfigure();
}

void BasicRecorderImpl::onDisconnected(const InputPortPtr& port)
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

void BasicRecorderImpl::onPacketReceived(const InputPortPtr& port)
{
    if (auto signal = findSignal(port))
        signal->onPacketReceived(port);
}

void BasicRecorderImpl::addProperties()
{
    objPtr.addProperty(StringProperty(Props::PATH, ""));
    objPtr.addProperty(BoolProperty(Props::RECORDING_ACTIVE, false));

    auto reconfigure = std::bind(&BasicRecorderImpl::reconfigure, this);

    objPtr.getOnPropertyValueWrite(Props::PATH) += reconfigure;
    objPtr.getOnPropertyValueWrite(Props::RECORDING_ACTIVE) += reconfigure;
}

void BasicRecorderImpl::addInputPort()
{
    auto c = createAndAddInputPort("Value" + std::to_string(++portCount), PacketReadyNotification::SameThread);
}

void BasicRecorderImpl::reconfigure()
{
    std::filesystem::path path = static_cast<std::string>(objPtr.getPropertyValue(Props::PATH));
    bool recordingActive = objPtr.getPropertyValue(Props::RECORDING_ACTIVE);

    if (recordingActive)
    {
        std::set<IInputPort *> ports;

        auto inputPorts = borrowPtr<FunctionBlockPtr>().getInputPorts();
        for (const auto& inputPort : inputPorts)
        {
            auto connection = inputPort.getConnection();
            if (connection.assigned())
            {
                ports.emplace(inputPort.getObject());

                SignalPtr signal = connection.getSignal();

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

std::shared_ptr<BasicRecorderSignal> BasicRecorderImpl::findSignal(IInputPort *port)
{
    std::shared_ptr<BasicRecorderSignal> signal;
    auto lock = getRecursiveConfigLock();
    auto it = signals.find(port);
    if (it != signals.end())
        signal = it->second;
    return signal;
}

END_NAMESPACE_OPENDAQ_BASIC_RECORDER_MODULE
