#pragma once

#include <pybind11/pybind11.h>

#include <opendaq/component_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_type_ptr.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/signal_config_ptr.h>
#include <coreobjects/property_object_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

FunctionBlockPtr createPythonFunctionBlock(const FunctionBlockTypePtr& type,
                                           const ContextPtr& context,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const PropertyObjectPtr& config,
                                           pybind11::object pyFunctionBlock);

InputPortConfigPtr pythonFunctionBlockCreateAndAddInputPort(IFunctionBlock* fb,
                                                            const std::string& localId,
                                                            PacketReadyNotification notificationMethod,
                                                            const BaseObjectPtr& customData,
                                                            bool requestGapPackets,
                                                            const PermissionsPtr& permissions);

SignalConfigPtr pythonFunctionBlockCreateAndAddSignal(IFunctionBlock* fb,
                                                      const std::string& localId,
                                                      const DataDescriptorPtr& descriptor,
                                                      bool visible,
                                                      bool isPublic,
                                                      const PermissionsPtr& permissions);

void pythonFunctionBlockRemoveInputPort(IFunctionBlock* fb, IInputPortConfig* port);
void pythonFunctionBlockRemoveSignal(IFunctionBlock* fb, ISignalConfig* signal);

END_NAMESPACE_OPENDAQ

