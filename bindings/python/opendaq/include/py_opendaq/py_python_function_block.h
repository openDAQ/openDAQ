#pragma once

#include <pybind11/pybind11.h>

#include <opendaq/component_ptr.h>
#include <opendaq/context_ptr.h>
#include <opendaq/function_block_ptr.h>
#include <opendaq/function_block_type_ptr.h>
#include <opendaq/input_port_config_ptr.h>
#include <opendaq/signal_config_ptr.h>
#include <coreobjects/property_object_ptr.h>

#include <py_opendaq/python_function_block.h>

BEGIN_NAMESPACE_OPENDAQ

FunctionBlockPtr createPythonFunctionBlock(const FunctionBlockTypePtr& type,
                                           const ContextPtr& context,
                                           const ComponentPtr& parent,
                                           const StringPtr& localId,
                                           const PropertyObjectPtr& config,
                                           pybind11::object pyFunctionBlock);

END_NAMESPACE_OPENDAQ

