#include "py_opendaq/py_opendaq.h"

void wrapDaqComponentOpenDaq(pybind11::module_ m)
{
    py::enum_<daq::SampleType>(m, "SampleType")
        .value("Invalid", daq::SampleType::Invalid)
        .value("Undefined", daq::SampleType::Undefined)
        .value("Float32", daq::SampleType::Float32)
        .value("Float64", daq::SampleType::Float64)
        .value("UInt8", daq::SampleType::UInt8)
        .value("Int8", daq::SampleType::Int8)
        .value("UInt16", daq::SampleType::UInt16)
        .value("Int16", daq::SampleType::Int16)
        .value("UInt32", daq::SampleType::UInt32)
        .value("Int32", daq::SampleType::Int32)
        .value("UInt64", daq::SampleType::UInt64)
        .value("Int64", daq::SampleType::Int64)
        .value("RangeInt64", daq::SampleType::RangeInt64)
        .value("ComplexFloat32", daq::SampleType::ComplexFloat32)
        .value("ComplexFloat64", daq::SampleType::ComplexFloat64)
        .value("Binary", daq::SampleType::Binary)
        .value("String", daq::SampleType::String)
        .value("Struct", daq::SampleType::Struct);

    py::enum_<daq::ScaledSampleType>(m, "ScaledSampleType")
        .value("Invalid", daq::ScaledSampleType::Invalid)
        .value("Float32", daq::ScaledSampleType::Float32)
        .value("Float64", daq::ScaledSampleType::Float64);

    auto classIModuleInfo = declareIModuleInfo(m);
    auto classIComponentType = declareIComponentType(m);
    auto classIRemovable = declareIRemovable(m);
    auto classIComponent = declareIComponent(m);
    auto classIFolder = declareIFolder(m);
    auto classIFolderConfig = declareIFolderConfig(m);
    auto classIDevice = declareIDevice(m);
    auto classIDevicePrivate = declareIDevicePrivate(m);
    auto classIDeviceNetworkConfig = declareIDeviceNetworkConfig(m);
    auto classIInstanceBuilder = declareIInstanceBuilder(m);
    auto classIConfigProvider = declareIConfigProvider(m);
    auto classIInstance = declareIInstance(m);
    auto classIContext = declareIContext(m);
    auto classIDeviceDomain = declareIDeviceDomain(m);
    auto classIDeviceInfo = declareIDeviceInfo(m);
    auto classIDeviceInfoConfig = declareIDeviceInfoConfig(m);
    auto classINetworkInterface = declareINetworkInterface(m);
    auto classIServerCapability = declareIServerCapability(m);
    auto classIConnectedClientInfo = declareIConnectedClientInfo(m);
    auto classIDeviceType = declareIDeviceType(m);
    auto classIFunctionBlock = declareIFunctionBlock(m);
    auto classIChannel = declareIChannel(m);
    auto classIFunctionBlockType = declareIFunctionBlockType(m);
    auto classIRecorder = declareIRecorder(m);
    auto classILogger = declareILogger(m);
    auto classILoggerComponent = declareILoggerComponent(m);
    auto classILoggerSink = declareILoggerSink(m);
    auto classILoggerThreadPool = declareILoggerThreadPool(m);
    auto classIModule = declareIModule(m);
    auto classIModuleManager = declareIModuleManager(m);
    auto classIDiscoveryServer = declareIDiscoveryServer(m);
    auto classIReader = declareIReader(m);
    auto classIPacketReader = declareIPacketReader(m);
    auto classISampleReader = declareISampleReader(m);
    auto classIBlockReader = declareIBlockReader(m);
    auto classIStreamReader = declareIStreamReader(m);
    auto classITailReader = declareITailReader(m);
    auto classIMultiReader = declareIMultiReader(m);
    auto classTimeStreamReader = declareTimeStreamReader(m);
    auto classTimeTailReader = declareTimeTailReader(m);
    auto classTimeBlockReader = declareTimeBlockReader(m);
    auto classTimeMultiReader = declareTimeMultiReader(m);
    auto classIBlockReaderBuilder = declareIBlockReaderBuilder(m);
    auto classITailReaderBuilder = declareITailReaderBuilder(m);
    auto classIStreamReaderBuilder = declareIStreamReaderBuilder(m);
    auto classIMultiReaderBuilder = declareIMultiReaderBuilder(m);
    auto classIReaderStatus = declareIReaderStatus(m);
    auto classIBlockReaderStatus = declareIBlockReaderStatus(m);
    auto classITailReaderStatus = declareITailReaderStatus(m);
    auto classIMultiReaderStatus = declareIMultiReaderStatus(m);
    auto classIAwaitable = declareIAwaitable(m);
    auto classIGraphVisualization = declareIGraphVisualization(m);
    auto classIScheduler = declareIScheduler(m);
    auto classITask = declareITask(m);
    auto classITaskGraph = declareITaskGraph(m);
    auto classIDataDescriptor = declareIDataDescriptor(m);
    auto classIDataDescriptorBuilder = declareIDataDescriptorBuilder(m);
    auto classIConnection = declareIConnection(m);
    auto classIPacketDestructCallback = declareIPacketDestructCallback(m);
    auto classIPacket = declareIPacket(m);
    auto classIDataPacket = declareIDataPacket(m);
    auto classIDataRule = declareIDataRule(m);
    auto classIDataRuleBuilder = declareIDataRuleBuilder(m);
    auto classIDimension = declareIDimension(m);
    auto classIDimensionBuilder = declareIDimensionBuilder(m);
    auto classIDimensionRule = declareIDimensionRule(m);
    auto classIDimensionRuleBuilder = declareIDimensionRuleBuilder(m);
    auto classIEventPacket = declareIEventPacket(m);
    auto classIInputPort = declareIInputPort(m);
    auto classIInputPortConfig = declareIInputPortConfig(m);
    auto classIInputPortNotifications = declareIInputPortNotifications(m);
    auto classIRange = declareIRange(m);
    auto classIScaling = declareIScaling(m);
    auto classIScalingBuilder = declareIScalingBuilder(m);
    auto classISignal = declareISignal(m);
    auto classISignalConfig = declareISignalConfig(m);
    auto classISignalEvents = declareISignalEvents(m);
    auto classITags = declareITags(m);
    auto classITagsPrivate = declareITagsPrivate(m);
    auto classIServer = declareIServer(m);
    auto classIServerType = declareIServerType(m);
    auto classIStreaming = declareIStreaming(m);
    auto classIStreamingType = declareIStreamingType(m);
    auto classIMirroredSignalConfig = declareIMirroredSignalConfig(m);
    auto classIMirroredSignalPrivate = declareIMirroredSignalPrivate(m);
    auto classISubscriptionEventArgs = declareISubscriptionEventArgs(m);
    auto classIMirroredDevice = declareIMirroredDevice(m);
    auto classIMirroredDeviceConfig = declareIMirroredDeviceConfig(m);
    auto classMockSignal = declareMockSignal(m);
    auto classIComponentPrivate = declareIComponentPrivate(m);
    auto classIComponentStatusContainer = declareIComponentStatusContainer(m);
    auto classIComponentStatusContainerPrivate = declareIComponentStatusContainerPrivate(m);
    auto classIConnectionStatusContainerPrivate = declareIConnectionStatusContainerPrivate(m);
    auto classIAddressInfo = declareIAddressInfo(m);
    auto classIAddressInfoBuilder = declareIAddressInfoBuilder(m);
    auto classISyncComponent = declareISyncComponent(m);
    auto classISyncComponentPrivate = declareISyncComponentPrivate(m);
    auto classIReferenceDomainInfo = declareIReferenceDomainInfo(m);
    auto classIReferenceDomainInfoBuilder = declareIReferenceDomainInfoBuilder(m);
    auto classIUpdateParameters = declareIUpdateParameters(m);
    auto classILogFileInfo = declareILogFileInfo(m);
    auto classILogFileInfoBuilder = declareILogFileInfoBuilder(m);

    defineIModuleInfo(m, classIModuleInfo);
    defineIComponentType(m, classIComponentType);
    defineIRemovable(m, classIRemovable);
    defineIComponent(m, classIComponent);
    defineIFolder(m, classIFolder);
    defineIFolderConfig(m, classIFolderConfig);
    defineIDevice(m, classIDevice);
    defineIDevicePrivate(m, classIDevicePrivate);
    defineIDeviceNetworkConfig(m, classIDeviceNetworkConfig);
    defineIInstanceBuilder(m, classIInstanceBuilder);
    defineIInstance(m, classIInstance);
    defineIConfigProvider(m, classIConfigProvider);
    defineIContext(m, classIContext);
    defineIDeviceDomain(m, classIDeviceDomain);
    defineIDeviceInfo(m, classIDeviceInfo);
    defineIDeviceInfoConfig(m, classIDeviceInfoConfig);
    defineINetworkInterface(m, classINetworkInterface);
    defineIServerCapability(m, classIServerCapability);
    defineIConnectedClientInfo(m, classIConnectedClientInfo);
    defineIDeviceType(m, classIDeviceType);
    defineIFunctionBlock(m, classIFunctionBlock);
    defineIChannel(m, classIChannel);
    defineIFunctionBlockType(m, classIFunctionBlockType);
    defineIRecorder(m, classIRecorder);
    defineILogger(m, classILogger);
    defineILoggerComponent(m, classILoggerComponent);
    defineILoggerSink(m, classILoggerSink);
    defineILoggerThreadPool(m, classILoggerThreadPool);
    defineIModule(m, classIModule);
    defineIModuleManager(m, classIModuleManager);
    defineIDiscoveryServer(m, classIDiscoveryServer);
    defineIReader(m, classIReader);
    defineIPacketReader(m, classIPacketReader);
    defineISampleReader(m, classISampleReader);
    defineIBlockReader(m, classIBlockReader);
    defineIStreamReader(m, classIStreamReader);
    defineITailReader(m, classITailReader);
    defineIMultiReader(m, classIMultiReader);
    defineTimeStreamReader(m, classTimeStreamReader);
    defineTimeTailReader(m, classTimeTailReader);
    defineTimeBlockReader(m, classTimeBlockReader);
    defineTimeMultiReader(m, classTimeMultiReader);
    defineIBlockReaderBuilder(m, classIBlockReaderBuilder);
    defineITailReaderBuilder(m, classITailReaderBuilder);
    defineIStreamReaderBuilder(m, classIStreamReaderBuilder);
    defineIMultiReaderBuilder(m, classIMultiReaderBuilder);
    defineIReaderStatus(m, classIReaderStatus);
    defineIBlockReaderStatus(m, classIBlockReaderStatus);
    defineITailReaderStatus(m, classITailReaderStatus);
    defineIMultiReaderStatus(m, classIMultiReaderStatus);
    defineIAwaitable(m, classIAwaitable);
    defineIGraphVisualization(m, classIGraphVisualization);
    defineIScheduler(m, classIScheduler);
    defineITask(m, classITask);
    defineITaskGraph(m, classITaskGraph);
    defineIDataDescriptor(m, classIDataDescriptor);
    defineIDataDescriptorBuilder(m, classIDataDescriptorBuilder);
    defineIConnection(m, classIConnection);
    defineIPacketDestructCallback(m, classIPacketDestructCallback);
    defineIPacket(m, classIPacket);
    defineIDataPacket(m, classIDataPacket);
    defineIDataRule(m, classIDataRule);
    defineIDataRuleBuilder(m, classIDataRuleBuilder);
    defineIDimension(m, classIDimension);
    defineIDimensionBuilder(m, classIDimensionBuilder);
    defineIDimensionRule(m, classIDimensionRule);
    defineIDimensionRuleBuilder(m, classIDimensionRuleBuilder);
    defineIEventPacket(m, classIEventPacket);
    defineIInputPort(m, classIInputPort);
    defineIInputPortConfig(m, classIInputPortConfig);
    defineIInputPortNotifications(m, classIInputPortNotifications);
    defineIRange(m, classIRange);
    defineIScaling(m, classIScaling);
    defineIScalingBuilder(m, classIScalingBuilder);
    defineISignal(m, classISignal);
    defineISignalConfig(m, classISignalConfig);
    defineISignalEvents(m, classISignalEvents);
    defineITags(m, classITags);
    defineITagsPrivate(m, classITagsPrivate);
    defineIServer(m, classIServer);
    defineIServerType(m, classIServerType);
    defineIStreaming(m, classIStreaming);
    defineIStreamingType(m, classIStreamingType);
    defineIMirroredSignalConfig(m, classIMirroredSignalConfig);
    defineIMirroredSignalPrivate(m, classIMirroredSignalPrivate);
    defineISubscriptionEventArgs(m, classISubscriptionEventArgs);
    defineIMirroredDevice(m, classIMirroredDevice);
    defineIMirroredDeviceConfig(m, classIMirroredDeviceConfig);
    defineMockSignal(m, classMockSignal);
    defineIComponentPrivate(m, classIComponentPrivate);
    defineIComponentStatusContainer(m, classIComponentStatusContainer);
    defineIComponentStatusContainerPrivate(m, classIComponentStatusContainerPrivate);
    defineIConnectionStatusContainerPrivate(m, classIConnectionStatusContainerPrivate);
    defineIAddressInfo(m, classIAddressInfo);
    defineIAddressInfoBuilder(m, classIAddressInfoBuilder);
    defineISyncComponent(m, classISyncComponent);
    defineISyncComponentPrivate(m, classISyncComponentPrivate);
    defineIReferenceDomainInfo(m, classIReferenceDomainInfo);
    defineIReferenceDomainInfoBuilder(m, classIReferenceDomainInfoBuilder);
    defineIUpdateParameters(m, classIUpdateParameters);
    defineILogFileInfo(m, classILogFileInfo);
    defineILogFileInfoBuilder(m, classILogFileInfoBuilder);

    defineComponentSearchFilterFactories(m);

    m.def("Instance", []() { return daq::Instance(".").detach(); });
    m.def("NullContext", []() { return daq::NullContext().detach(); });
}
