#include <parquet_recorder_module/parquet_recorder_impl.h>

#include <functional>
#include <string>

#include <coreobjects/callable_info_factory.h>

#include <parquet_recorder_module/common.h>
#include <parquet_recorder_module/parquet_writer.h>

BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE

FunctionBlockTypePtr ParquetRecorderImpl::createType()
{
    return FunctionBlockType(TypeId, "ParquetRecorder", "Parquet recording functionality", PropertyObject());
}

ParquetRecorderImpl::ParquetRecorderImpl(const ContextPtr& context,
                                         const ComponentPtr& parent,
                                         const StringPtr& localId,
                                         const PropertyObjectPtr& config)
    : FunctionBlockImpl<IFunctionBlock, IRecorder>(createType(), context, parent, localId, nullptr)
{
    tags.add(Tags::Recorder);
    scheduler = context.getScheduler();

    if (loggerComponent.assigned())
        loggerComponent.setLevel(LogLevel::Trace);

    if (!scheduler.assigned())
    {
        throw InvalidParameterException("ParquetRecorderImpl: Scheduler must be assigned for recording to work");
    }

    addInputPort();
    addProperties();
}

ParquetRecorderImpl::~ParquetRecorderImpl()
{
    auto lock = getRecursiveConfigLock();
    clearWriters();
}

ErrCode ParquetRecorderImpl::startRecording()
{
    LOG_D("ParquetRecorderImpl::startRecording: Starting recording...");
    auto lock = getRecursiveConfigLock();
    if (recording.load(std::memory_order_relaxed))
    {
        LOG_E("ParquetRecorderImpl::startRecording: Recording is already active.");
        return OPENDAQ_ERR_INVALIDSTATE;
    }

    recording.store(true, std::memory_order_relaxed);

    reconfigure();

    return OPENDAQ_SUCCESS;
}

ErrCode ParquetRecorderImpl::stopRecording()
{
    LOG_D("ParquetRecorderImpl::stopRecording: Stopping recording...");
    auto lock = getRecursiveConfigLock();
    if (!recording.load(std::memory_order_relaxed))
    {
        LOG_E("ParquetRecorderImpl::stopRecording: Recording is not active.");
        return OPENDAQ_ERR_INVALIDSTATE;
    }

    recording.store(false, std::memory_order_relaxed);
    clearWriters();

    return OPENDAQ_SUCCESS;
}

ErrCode ParquetRecorderImpl::getIsRecording(Bool* isRecording)
{
    LOG_D("ParquetRecorderImpl::getIsRecording: Checking if recording is active...");
    OPENDAQ_PARAM_NOT_NULL(isRecording);

    *isRecording = recording.load(std::memory_order_relaxed);

    return OPENDAQ_SUCCESS;
}

void ParquetRecorderImpl::onConnected(const InputPortPtr& port)
{
    LOG_D("ParquetRecorderImpl::onConnected: Input port connected: {}", port.getName());
    auto lock = getRecursiveConfigLock();
    addInputPort();
    reconfigure();
}

void ParquetRecorderImpl::onDisconnected(const InputPortPtr& port)
{
    LOG_D("ParquetRecorderImpl::onDisconnected: Input port disconnected: {}", port.getName());
    auto lock = getRecursiveConfigLock();

    removeInputPort(port);

    reconfigure();
}

void ParquetRecorderImpl::activeChanged()
{
    LOG_D("Parquet recorder active: {}", active);
}

void ParquetRecorderImpl::onPacketReceived(const InputPortPtr& port)
{
    if (!active || !recording.load(std::memory_order_relaxed))
        return;

    auto writer = findWriterForSignal(port);
    auto list = port.getConnection().dequeueAll();

    if (!writer || !list.assigned())
        return;

    writer->enqueuePacketList(list);
}

void ParquetRecorderImpl::addProperties()
{
    objPtr.addProperty(StringProperty(Props::Path, ""));

    objPtr.getOnPropertyValueWrite(Props::Path) += std::bind(&ParquetRecorderImpl::reconfigure, this);

    const auto startRecordingProp = FunctionProperty(Props::StartRecording, ProcedureInfo());
    objPtr.addProperty(startRecordingProp);
    objPtr.setPropertyValue(Props::StartRecording, Procedure([this] { this->startRecording(); }));

    const auto stopRecordingProp = FunctionProperty(Props::StopRecording, ProcedureInfo());
    objPtr.addProperty(stopRecordingProp);
    objPtr.setPropertyValue(Props::StopRecording, Procedure([this] { this->stopRecording(); }));
}

void ParquetRecorderImpl::addInputPort()
{
    LOG_D("ParquetRecorderImpl::addInputPort: Adding new input port for ParquetRecorder");
    auto c = createAndAddInputPort("Value" + std::to_string(portCount.fetch_add(1, std::memory_order_relaxed)),
                                   PacketReadyNotification::SameThread);
}

void ParquetRecorderImpl::reconfigure()
{
    LOG_D("ParquetRecorderImpl::reconfigure: Reconfiguring ParquetRecorder...");
    auto lock = getRecursiveConfigLock();
    fs::path path = static_cast<std::string>(objPtr.getPropertyValue(Props::Path));

    std::unordered_set<IInputPort*> ports;

    auto inputPorts = borrowPtr<FunctionBlockPtr>().getInputPorts();
    for (const auto& inputPort : inputPorts)
    {
        auto connection = inputPort.getConnection();
        if (connection.assigned())
        {
            auto signal = connection.getSignal();
            ports.emplace(inputPort.getObject());

            if (writers.find(inputPort.getObject()) == writers.end())
            {
                writers.emplace(inputPort.getObject(),
                                std::make_shared<ParquetWriter>(path, signal, loggerComponent, context.getScheduler()));
            }
        }
    }

    auto it = writers.begin();
    while (it != writers.end())
    {
        if (ports.find(it->first) == ports.end())
            it = writers.erase(it);
        else
            ++it;
    }
}

void ParquetRecorderImpl::clearWriters()
{
    LOG_D("ParquetRecorderImpl::clearWriters: Clearing all Parquet writers...");
    writers.clear();
}

std::shared_ptr<ParquetWriter> ParquetRecorderImpl::findWriterForSignal(IInputPort* port)
{
    auto lock = getAcquisitionLock();
    if (auto it = writers.find(port); it != writers.end())
        return it->second;
    return nullptr;
}

END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE
