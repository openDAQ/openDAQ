#include <parquet_recorder_module/parquet_recorder_impl.h>

#include <functional>

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
    , cachedPath(std::nullopt)
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
    if (recording)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Recording is already active.");
    }

    recording = true;

    reconfigure();

    return OPENDAQ_SUCCESS;
}

ErrCode ParquetRecorderImpl::stopRecording()
{
    LOG_D("ParquetRecorderImpl::stopRecording: Stopping recording...");
    auto lock = getRecursiveConfigLock();
    if (!recording)
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDSTATE, "Recording is not active.");
    }

    recording = false;
    clearWriters();

    return OPENDAQ_SUCCESS;
}

ErrCode ParquetRecorderImpl::getIsRecording(Bool* isRecording)
{
    LOG_D("ParquetRecorderImpl::getIsRecording: Checking if recording is active...");
    OPENDAQ_PARAM_NOT_NULL(isRecording);

    *isRecording = recording;

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
    if (!active || !recording)
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
    auto c = createAndAddInputPort("Value" + std::to_string(portCount.fetch_add(1)), PacketReadyNotification::SameThread);
}

void ParquetRecorderImpl::reconfigure()
{
    LOG_D("ParquetRecorderImpl::reconfigure: Reconfiguring ParquetRecorder...");
    auto lock = getRecursiveConfigLock();
    fs::path path = static_cast<std::string>(objPtr.getPropertyValue(Props::Path));

    bool pathChanged = false;
    if (!cachedPath.has_value() || cachedPath.value() != path.string()){
        cachedPath = path.string();
        pathChanged = true;
    }

    if (!recording)
    {
        // Do not keep writers when not recording.
        writers.clear();
        return;
    }

    std::unordered_set<IInputPort*> ports;

    auto inputPorts = borrowPtr<FunctionBlockPtr>().getInputPorts();
    for (const auto& inputPort : inputPorts)
    {
        auto connection = inputPort.getConnection();
        if (connection.assigned())
        {
            auto signal = connection.getSignal();
            ports.emplace(inputPort.getObject());

            // Create writer for any new ports as well as replace writers when path changes.
            auto it = writers.find(inputPort.getObject());
            if (it == writers.end() || pathChanged)
            {
                // Might take a long time to stop the existing writer and create a new one.
                writers.insert_or_assign(
                    inputPort.getObject(),
                    std::make_shared<ParquetWriter>(path, signal, loggerComponent, context.getScheduler()));
            }
        }
    }

    // Remove writers of disconnected ports.
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
