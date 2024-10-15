/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <pybind11/pybind11.h>
#include <opendaq/opendaq.h>
#include "py_core_types/py_opendaq_daq.h"
#include "py_opendaq/py_mock_signal.h"

void wrapDaqComponentOpenDaq(pybind11::module_ m);

PyDaqIntf<daq::IInstance, daq::IDevice> declareIInstance(pybind11::module_ m);
PyDaqIntf<daq::IInstanceBuilder, daq::IBaseObject> declareIInstanceBuilder(pybind11::module_ m);
PyDaqIntf<daq::IConfigProvider, daq::IBaseObject> declareIConfigProvider(pybind11::module_ m);
PyDaqIntf<daq::IContext, daq::IBaseObject> declareIContext(pybind11::module_ m);
PyDaqIntf<daq::IRemovable, daq::IBaseObject> declareIRemovable(pybind11::module_ m);
PyDaqIntf<daq::IComponent, daq::IPropertyObject> declareIComponent(pybind11::module_ m);
PyDaqIntf<daq::IFolder, daq::IComponent> declareIFolder(pybind11::module_ m);
PyDaqIntf<daq::IFolderConfig, daq::IFolder> declareIFolderConfig(pybind11::module_ m);
PyDaqIntf<daq::IDevice, daq::IFolder> declareIDevice(pybind11::module_ m);
PyDaqIntf<daq::IDeviceDomain, daq::IBaseObject> declareIDeviceDomain(pybind11::module_ m);
PyDaqIntf<daq::IDeviceInfo, daq::IPropertyObject> declareIDeviceInfo(pybind11::module_ m);
PyDaqIntf<daq::IDeviceInfoConfig, daq::IDeviceInfo> declareIDeviceInfoConfig(pybind11::module_ m);
PyDaqIntf<daq::IServerCapability, daq::IPropertyObject> declareIServerCapability(pybind11::module_ m);
PyDaqIntf<daq::IDeviceType, daq::IComponentType> declareIDeviceType(pybind11::module_ m);
PyDaqIntf<daq::IChannel, daq::IFunctionBlock> declareIChannel(pybind11::module_ m);
PyDaqIntf<daq::IFunctionBlock, daq::IFolder> declareIFunctionBlock(pybind11::module_ m);
PyDaqIntf<daq::IFunctionBlockType, daq::IComponentType> declareIFunctionBlockType(pybind11::module_ m);
PyDaqIntf<daq::ILogger, daq::IBaseObject> declareILogger(pybind11::module_ m);
PyDaqIntf<daq::ILoggerComponent, daq::IBaseObject> declareILoggerComponent(pybind11::module_ m);
PyDaqIntf<daq::ILoggerSink, daq::IBaseObject> declareILoggerSink(pybind11::module_ m);
PyDaqIntf<daq::ILoggerThreadPool, daq::IBaseObject> declareILoggerThreadPool(pybind11::module_ m);
PyDaqIntf<daq::IModule, daq::IBaseObject> declareIModule(pybind11::module_ m);
PyDaqIntf<daq::IModuleManager, daq::IBaseObject> declareIModuleManager(pybind11::module_ m);
PyDaqIntf<daq::IDiscoveryServer, daq::IBaseObject> declareIDiscoveryServer(pybind11::module_ m);
PyDaqIntf<daq::IReader, daq::IBaseObject> declareIReader(pybind11::module_ m);
PyDaqIntf<daq::IPacketReader, daq::IReader> declareIPacketReader(pybind11::module_ m);
PyDaqIntf<daq::ISampleReader, daq::IReader> declareISampleReader(pybind11::module_ m);
PyDaqIntf<daq::IBlockReader, daq::ISampleReader> declareIBlockReader(pybind11::module_ m);
PyDaqIntf<daq::IStreamReader, daq::ISampleReader> declareIStreamReader(pybind11::module_ m);
PyDaqIntf<daq::ITailReader, daq::ISampleReader> declareITailReader(pybind11::module_ m);
PyDaqIntf<daq::IMultiReader, daq::ISampleReader> declareIMultiReader(pybind11::module_ m);
PyDaqIntf<daq::IBlockReaderBuilder, daq::IBaseObject> declareIBlockReaderBuilder(pybind11::module_ m);
PyDaqIntf<daq::ITailReaderBuilder, daq::IBaseObject> declareITailReaderBuilder(pybind11::module_ m);
PyDaqIntf<daq::IStreamReaderBuilder, daq::IBaseObject> declareIStreamReaderBuilder(pybind11::module_ m);
PyDaqIntf<daq::IMultiReaderBuilder, daq::IBaseObject> declareIMultiReaderBuilder(pybind11::module_ m);
PyDaqIntf<daq::IReaderStatus, daq::IBaseObject> declareIReaderStatus(pybind11::module_ m);
PyDaqIntf<daq::IBlockReaderStatus, daq::IReaderStatus> declareIBlockReaderStatus(pybind11::module_ m);
PyDaqIntf<daq::ITailReaderStatus, daq::IReaderStatus> declareITailReaderStatus(pybind11::module_ m);
PyDaqIntf<daq::IMultiReaderStatus, daq::IReaderStatus> declareIMultiReaderStatus(pybind11::module_ m);
PyDaqIntf<daq::IAwaitable, daq::IBaseObject> declareIAwaitable(pybind11::module_ m);
PyDaqIntf<daq::IGraphVisualization, daq::IBaseObject> declareIGraphVisualization(pybind11::module_ m);
PyDaqIntf<daq::IScheduler, daq::IBaseObject> declareIScheduler(pybind11::module_ m);
PyDaqIntf<daq::ITask, daq::IBaseObject> declareITask(pybind11::module_ m);
PyDaqIntf<daq::ITaskGraph, daq::ITask> declareITaskGraph(pybind11::module_ m);
PyDaqIntf<daq::IDataDescriptor, daq::IBaseObject> declareIDataDescriptor(pybind11::module_ m);
PyDaqIntf<daq::IDataDescriptorBuilder, daq::IBaseObject> declareIDataDescriptorBuilder(pybind11::module_ m);
PyDaqIntf<daq::IConnection, daq::IBaseObject> declareIConnection(pybind11::module_ m);
PyDaqIntf<daq::IPacketDestructCallback, daq::IBaseObject> declareIPacketDestructCallback(pybind11::module_ m);
PyDaqIntf<daq::IDataPacket, daq::IPacket> declareIDataPacket(pybind11::module_ m);
PyDaqIntf<daq::IDataRule, daq::IBaseObject> declareIDataRule(pybind11::module_ m);
PyDaqIntf<daq::IDataRuleBuilder, daq::IBaseObject> declareIDataRuleBuilder(pybind11::module_ m);
PyDaqIntf<daq::IDimension, daq::IBaseObject> declareIDimension(pybind11::module_ m);
PyDaqIntf<daq::IDimensionBuilder, daq::IBaseObject> declareIDimensionBuilder(pybind11::module_ m);
PyDaqIntf<daq::IDimensionRule, daq::IBaseObject> declareIDimensionRule(pybind11::module_ m);
PyDaqIntf<daq::IDimensionRuleBuilder, daq::IBaseObject> declareIDimensionRuleBuilder(pybind11::module_ m);
PyDaqIntf<daq::IEventPacket, daq::IPacket> declareIEventPacket(pybind11::module_ m);
PyDaqIntf<daq::IInputPort, daq::IComponent> declareIInputPort(pybind11::module_ m);
PyDaqIntf<daq::IInputPortConfig, daq::IInputPort> declareIInputPortConfig(pybind11::module_ m);
PyDaqIntf<daq::IInputPortNotifications, daq::IBaseObject> declareIInputPortNotifications(pybind11::module_ m);
PyDaqIntf<daq::IPacket, daq::IBaseObject> declareIPacket(pybind11::module_ m);
PyDaqIntf<daq::IRange, daq::IBaseObject> declareIRange(pybind11::module_ m);
PyDaqIntf<daq::IScaling, daq::IBaseObject> declareIScaling(pybind11::module_ m);
PyDaqIntf<daq::IScalingBuilder, daq::IBaseObject> declareIScalingBuilder(pybind11::module_ m);
PyDaqIntf<daq::ISignal, daq::IComponent> declareISignal(pybind11::module_ m);
PyDaqIntf<daq::ISignalConfig, daq::ISignal> declareISignalConfig(pybind11::module_ m);
PyDaqIntf<daq::ISignalEvents, daq::IBaseObject> declareISignalEvents(pybind11::module_ m);
PyDaqIntf<daq::ITags, daq::IBaseObject> declareITags(pybind11::module_ m);
PyDaqIntf<daq::ITagsPrivate, daq::IBaseObject> declareITagsPrivate(pybind11::module_ m);
PyDaqIntf<daq::IServer, daq::IFolder> declareIServer(pybind11::module_ m);
PyDaqIntf<daq::IServerType, daq::IComponentType> declareIServerType(pybind11::module_ m);
PyDaqIntf<daq::IStreaming, daq::IBaseObject> declareIStreaming(pybind11::module_ m);
PyDaqIntf<daq::IStreamingType, daq::IComponentType> declareIStreamingType(pybind11::module_ m);
PyDaqIntf<daq::IMirroredSignalConfig, daq::ISignalConfig> declareIMirroredSignalConfig(pybind11::module_ m);
PyDaqIntf<daq::IMirroredSignalPrivate, daq::IBaseObject> declareIMirroredSignalPrivate(pybind11::module_ m);
PyDaqIntf<daq::ISubscriptionEventArgs, daq::IEventArgs> declareISubscriptionEventArgs(pybind11::module_ m);
PyDaqIntf<daq::IMirroredDevice, daq::IDevice> declareIMirroredDevice(pybind11::module_ m);
PyDaqIntf<daq::IMirroredDeviceConfig, daq::IMirroredDevice> declareIMirroredDeviceConfig(pybind11::module_ m);
PyDaqIntf<daq::IComponentPrivate, daq::IBaseObject> declareIComponentPrivate(pybind11::module_ m);
PyDaqIntf<daq::ISearchFilter, daq::IBaseObject> declareISearchFilter(pybind11::module_ m);
PyDaqIntf<daq::IComponentStatusContainer, daq::IBaseObject> declareIComponentStatusContainer(pybind11::module_ m);
PyDaqIntf<daq::IComponentStatusContainerPrivate, daq::IBaseObject> declareIComponentStatusContainerPrivate(pybind11::module_ m);
PyDaqIntf<daq::IAddressInfo, daq::IPropertyObject> declareIAddressInfo(pybind11::module_ m);
PyDaqIntf<daq::IAddressInfoBuilder, daq::IBaseObject> declareIAddressInfoBuilder(pybind11::module_ m);
PyDaqIntf<daq::ISyncComponent, daq::IComponent> declareISyncComponent(pybind11::module_ m);
PyDaqIntf<daq::ISyncComponentPrivate, daq::IBaseObject> declareISyncComponentPrivate(pybind11::module_ m);
PyDaqIntf<daq::IReferenceDomainInfo, daq::IBaseObject> declareIReferenceDomainInfo(pybind11::module_ m);
PyDaqIntf<daq::IReferenceDomainInfoBuilder, daq::IBaseObject> declareIReferenceDomainInfoBuilder(pybind11::module_ m);
PyDaqIntf<daq::IUpdateParameters, daq::IPropertyObject> declareIUpdateParameters(pybind11::module_ m);
PyDaqIntf<daq::ILogFileInfo, daq::IBaseObject> declareILogFileInfo(pybind11::module_ m);

py::class_<daq::TimeReader<daq::StreamReaderPtr>> declareTimeStreamReader(pybind11::module_ m);
py::class_<daq::TimeReader<daq::TailReaderPtr>> declareTimeTailReader(pybind11::module_ m);
py::class_<daq::TimeReader<daq::BlockReaderPtr>> declareTimeBlockReader(pybind11::module_ m);
py::class_<daq::TimeReader<daq::MultiReaderPtr>> declareTimeMultiReader(pybind11::module_ m);

py::class_<daq::MockSignal> declareMockSignal(py::module_ m);



void defineIInstance(pybind11::module_ m, PyDaqIntf<daq::IInstance, daq::IDevice> cls);
void defineIInstanceBuilder(pybind11::module_ m, PyDaqIntf<daq::IInstanceBuilder, daq::IBaseObject> cls);
void defineIConfigProvider(pybind11::module_ m, PyDaqIntf<daq::IConfigProvider, daq::IBaseObject> cls);
void defineIContext(pybind11::module_ m, PyDaqIntf<daq::IContext, daq::IBaseObject> cls);
void defineIRemovable(pybind11::module_ m, PyDaqIntf<daq::IRemovable, daq::IBaseObject> cls);
void defineIComponent(pybind11::module_ m, PyDaqIntf<daq::IComponent, daq::IPropertyObject> cls);
void defineIFolder(pybind11::module_ m, PyDaqIntf<daq::IFolder, daq::IComponent> cls);
void defineIFolderConfig(pybind11::module_ m, PyDaqIntf<daq::IFolderConfig, daq::IFolder> cls);
void defineIDevice(pybind11::module_ m, PyDaqIntf<daq::IDevice, daq::IFolder> cls);
void defineIDeviceDomain(pybind11::module_ m, PyDaqIntf<daq::IDeviceDomain, daq::IBaseObject> cls);
void defineIDeviceInfo(pybind11::module_ m, PyDaqIntf<daq::IDeviceInfo, daq::IPropertyObject> cls);
void defineIDeviceInfoConfig(pybind11::module_ m, PyDaqIntf<daq::IDeviceInfoConfig, daq::IDeviceInfo> cls);
void defineIServerCapability(pybind11::module_ m, PyDaqIntf<daq::IServerCapability, daq::IPropertyObject> cls);
void defineIDeviceType(pybind11::module_ m, PyDaqIntf<daq::IDeviceType, daq::IComponentType> cls);
void defineIChannel(pybind11::module_ m, PyDaqIntf<daq::IChannel, daq::IFunctionBlock> cls);
void defineIFunctionBlock(pybind11::module_ m, PyDaqIntf<daq::IFunctionBlock, daq::IFolder> cls);
void defineIFunctionBlockType(pybind11::module_ m, PyDaqIntf<daq::IFunctionBlockType, daq::IComponentType> cls);
void defineILogger(pybind11::module_ m, PyDaqIntf<daq::ILogger, daq::IBaseObject> cls);
void defineILoggerComponent(pybind11::module_ m, PyDaqIntf<daq::ILoggerComponent, daq::IBaseObject> cls);
void defineILoggerSink(pybind11::module_ m, PyDaqIntf<daq::ILoggerSink, daq::IBaseObject> cls);
void defineILoggerThreadPool(pybind11::module_ m, PyDaqIntf<daq::ILoggerThreadPool, daq::IBaseObject> cls);
void defineIModule(pybind11::module_ m, PyDaqIntf<daq::IModule, daq::IBaseObject> cls);
void defineIModuleManager(pybind11::module_ m, PyDaqIntf<daq::IModuleManager, daq::IBaseObject> cls);
void defineIDiscoveryServer(pybind11::module_ m, PyDaqIntf<daq::IDiscoveryServer, daq::IBaseObject> cls);
void defineIReader(pybind11::module_ m, PyDaqIntf<daq::IReader, daq::IBaseObject> cls);
void defineIPacketReader(pybind11::module_ m, PyDaqIntf<daq::IPacketReader, daq::IReader> cls);
void defineISampleReader(pybind11::module_ m, PyDaqIntf<daq::ISampleReader, daq::IReader> cls);
void defineIBlockReader(pybind11::module_ m, PyDaqIntf<daq::IBlockReader, daq::ISampleReader> cls);
void defineIStreamReader(pybind11::module_ m, PyDaqIntf<daq::IStreamReader, daq::ISampleReader> cls);
void defineIMultiReader(pybind11::module_ m, PyDaqIntf<daq::IMultiReader, daq::ISampleReader> cls);
void defineTimeStreamReader(pybind11::module_ m, py::class_<daq::TimeReader<daq::StreamReaderPtr>> cls);
void defineTimeTailReader(pybind11::module_ m, py::class_<daq::TimeReader<daq::TailReaderPtr>> cls);
void defineTimeBlockReader(pybind11::module_ m, py::class_<daq::TimeReader<daq::BlockReaderPtr>> cls);
void defineTimeMultiReader(pybind11::module_ m, py::class_<daq::TimeReader<daq::MultiReaderPtr>> cls);
void defineITailReader(pybind11::module_ m, PyDaqIntf<daq::ITailReader, daq::ISampleReader> cls);
void defineIBlockReaderBuilder(pybind11::module_ m, PyDaqIntf<daq::IBlockReaderBuilder, daq::IBaseObject> cls);
void defineITailReaderBuilder(pybind11::module_ m, PyDaqIntf<daq::ITailReaderBuilder, daq::IBaseObject> cls);
void defineIStreamReaderBuilder(pybind11::module_ m, PyDaqIntf<daq::IStreamReaderBuilder, daq::IBaseObject> cls);
void defineIMultiReaderBuilder(pybind11::module_ m, PyDaqIntf<daq::IMultiReaderBuilder, daq::IBaseObject> cls);
void defineIReaderStatus(pybind11::module_ m, PyDaqIntf<daq::IReaderStatus, daq::IBaseObject> cls);
void defineIBlockReaderStatus(pybind11::module_ m, PyDaqIntf<daq::IBlockReaderStatus, daq::IReaderStatus> cls);
void defineITailReaderStatus(pybind11::module_ m, PyDaqIntf<daq::ITailReaderStatus, daq::IReaderStatus> cls);
void defineIMultiReaderStatus(pybind11::module_ m, PyDaqIntf<daq::IMultiReaderStatus, daq::IReaderStatus> cls);
void defineIAwaitable(pybind11::module_ m, PyDaqIntf<daq::IAwaitable, daq::IBaseObject> cls);
void defineIGraphVisualization(pybind11::module_ m, PyDaqIntf<daq::IGraphVisualization, daq::IBaseObject> cls);
void defineIScheduler(pybind11::module_ m, PyDaqIntf<daq::IScheduler, daq::IBaseObject> cls);
void defineITask(pybind11::module_ m, PyDaqIntf<daq::ITask, daq::IBaseObject> cls);
void defineITaskGraph(pybind11::module_ m, PyDaqIntf<daq::ITaskGraph, daq::ITask> cls);
void defineIDataDescriptor(pybind11::module_ m, PyDaqIntf<daq::IDataDescriptor, daq::IBaseObject> cls);
void defineIDataDescriptorBuilder(pybind11::module_ m, PyDaqIntf<daq::IDataDescriptorBuilder, daq::IBaseObject> cls);
void defineIConnection(pybind11::module_ m, PyDaqIntf<daq::IConnection, daq::IBaseObject> cls);
void defineIPacketDestructCallback(pybind11::module_ m, PyDaqIntf<daq::IPacketDestructCallback, daq::IBaseObject> cls);
void defineIDataPacket(pybind11::module_ m, PyDaqIntf<daq::IDataPacket, daq::IPacket> cls);
void defineIDataRule(pybind11::module_ m, PyDaqIntf<daq::IDataRule, daq::IBaseObject> cls);
void defineIDataRuleBuilder(pybind11::module_ m, PyDaqIntf<daq::IDataRuleBuilder, daq::IBaseObject> cls);
void defineIDimension(pybind11::module_ m, PyDaqIntf<daq::IDimension, daq::IBaseObject> cls);
void defineIDimensionBuilder(pybind11::module_ m, PyDaqIntf<daq::IDimensionBuilder, daq::IBaseObject> cls);
void defineIDimensionRule(pybind11::module_ m, PyDaqIntf<daq::IDimensionRule, daq::IBaseObject> cls);
void defineIDimensionRuleBuilder(pybind11::module_ m, PyDaqIntf<daq::IDimensionRuleBuilder, daq::IBaseObject> cls);
void defineIEventPacket(pybind11::module_ m, PyDaqIntf<daq::IEventPacket, daq::IPacket> cls);
void defineIInputPort(pybind11::module_ m, PyDaqIntf<daq::IInputPort, daq::IComponent> cls);
void defineIInputPortConfig(pybind11::module_ m, PyDaqIntf<daq::IInputPortConfig, daq::IInputPort> cls);
void defineIInputPortNotifications(pybind11::module_ m, PyDaqIntf<daq::IInputPortNotifications, daq::IBaseObject> cls);
void defineIPacket(pybind11::module_ m, PyDaqIntf<daq::IPacket, daq::IBaseObject> cls);
void defineIRange(pybind11::module_ m, PyDaqIntf<daq::IRange, daq::IBaseObject> cls);
void defineIScaling(pybind11::module_ m, PyDaqIntf<daq::IScaling, daq::IBaseObject> cls);
void defineIScalingBuilder(pybind11::module_ m, PyDaqIntf<daq::IScalingBuilder, daq::IBaseObject> cls);
void defineISignal(pybind11::module_ m, PyDaqIntf<daq::ISignal, daq::IComponent> cls);
void defineISignalConfig(pybind11::module_ m, PyDaqIntf<daq::ISignalConfig, daq::ISignal> cls);
void defineISignalEvents(pybind11::module_ m, PyDaqIntf<daq::ISignalEvents, daq::IBaseObject> cls);
void defineITags(pybind11::module_ m, PyDaqIntf<daq::ITags, daq::IBaseObject> cls);
void defineITagsPrivate(pybind11::module_ m, PyDaqIntf<daq::ITagsPrivate, daq::IBaseObject> cls);
void defineIServer(pybind11::module_ m, PyDaqIntf<daq::IServer, daq::IFolder> cls);
void defineIServerType(pybind11::module_ m, PyDaqIntf<daq::IServerType, daq::IComponentType> cls);
void defineIStreaming(pybind11::module_ m, PyDaqIntf<daq::IStreaming, daq::IBaseObject> cls);
void defineIStreamingType(pybind11::module_ m, PyDaqIntf<daq::IStreamingType, daq::IComponentType> cls);
void defineIMirroredSignalConfig(pybind11::module_ m, PyDaqIntf<daq::IMirroredSignalConfig, daq::ISignalConfig> cls);
void defineIMirroredSignalPrivate(pybind11::module_ m, PyDaqIntf<daq::IMirroredSignalPrivate, daq::IBaseObject> cls);
void defineISubscriptionEventArgs(pybind11::module_ m, PyDaqIntf<daq::ISubscriptionEventArgs, daq::IEventArgs> cls);
void defineIMirroredDevice(pybind11::module_ m, PyDaqIntf<daq::IMirroredDevice, daq::IDevice> cls);
void defineIMirroredDeviceConfig(pybind11::module_ m, PyDaqIntf<daq::IMirroredDeviceConfig, daq::IMirroredDevice> cls);
void defineMockSignal(pybind11::module_ m, py::class_<daq::MockSignal> cls);
void defineIComponentPrivate(pybind11::module_ m, PyDaqIntf<daq::IComponentPrivate, daq::IBaseObject> cls);
void defineISearchFilter(pybind11::module_ m, PyDaqIntf<daq::ISearchFilter, daq::IBaseObject> cls);
void defineIComponentStatusContainer(pybind11::module_ m, PyDaqIntf<daq::IComponentStatusContainer, daq::IBaseObject> cls);
void defineIComponentStatusContainerPrivate(pybind11::module_ m, PyDaqIntf<daq::IComponentStatusContainerPrivate, daq::IBaseObject> cls);
void defineIAddressInfo(pybind11::module_ m, PyDaqIntf<daq::IAddressInfo, daq::IPropertyObject> cls);
void defineIAddressInfoBuilder(pybind11::module_ m, PyDaqIntf<daq::IAddressInfoBuilder, daq::IBaseObject> cls);
void defineISyncComponent(pybind11::module_ m, PyDaqIntf<daq::ISyncComponent, daq::IComponent> cls);
void defineISyncComponentPrivate(pybind11::module_ m, PyDaqIntf<daq::ISyncComponentPrivate, daq::IBaseObject> cls);
void defineIReferenceDomainInfo(pybind11::module_ m, PyDaqIntf<daq::IReferenceDomainInfo, daq::IBaseObject> cls);
void defineIReferenceDomainInfoBuilder(pybind11::module_ m, PyDaqIntf<daq::IReferenceDomainInfoBuilder, daq::IBaseObject> cls);
void defineIUpdateParameters(pybind11::module_ m, PyDaqIntf<daq::IUpdateParameters, daq::IPropertyObject> cls);
void defineILogFileInfo(pybind11::module_ m, PyDaqIntf<daq::ILogFileInfo, daq::IBaseObject> cls);