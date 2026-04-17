/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#include <pybind11/gil.h>
#include <pybind11/pybind11.h>

#include "py_opendaq/py_python_function_block.h"
#include "py_opendaq/py_opendaq.h"

namespace py = pybind11;

template <typename FunctionBlockClass>
void definePythonFunctionBlockSupport(pybind11::module_ /*m*/, FunctionBlockClass functionBlockClass)
{
    // Methods for Python-implemented function blocks (created from C++ wrapper)
    functionBlockClass.def("_create_and_add_input_port",
          [](daq::IFunctionBlock* fb,
             const std::string& localId,
             daq::PacketReadyNotification notificationMethod,
             daq::IBaseObject* customData,
             bool requestGapPackets,
             daq::IPermissions* permissions)
          {
              py::gil_scoped_release release;
              return daq::pythonFunctionBlockCreateAndAddInputPort(fb,
                                                                  localId,
                                                                  notificationMethod,
                                                                  daq::BaseObjectPtr::Borrow(customData),
                                                                  requestGapPackets,
                                                                  daq::PermissionsPtr::Borrow(permissions))
                  .detach();
          },
          py::arg("local_id"),
          py::arg("notification_method"),
          py::arg("custom_data") = nullptr,
          py::arg("request_gap_packets") = false,
          py::arg("permissions") = nullptr);

    functionBlockClass.def("_create_and_add_signal",
          [](daq::IFunctionBlock* fb,
             const std::string& localId,
             daq::IDataDescriptor* descriptor,
             bool visible,
             bool isPublic,
             daq::IPermissions* permissions)
          {
              py::gil_scoped_release release;
              return daq::pythonFunctionBlockCreateAndAddSignal(fb,
                                                               localId,
                                                               daq::DataDescriptorPtr::Borrow(descriptor),
                                                               visible,
                                                               isPublic,
                                                               daq::PermissionsPtr::Borrow(permissions))
                  .detach();
          },
          py::arg("local_id"),
          py::arg("descriptor") = nullptr,
          py::arg("visible") = true,
          py::arg("is_public") = true,
          py::arg("permissions") = nullptr);

    functionBlockClass.def("_remove_input_port",
          [](daq::IFunctionBlock* fb, daq::IInputPortConfig* port)
          {
              py::gil_scoped_release release;
              daq::pythonFunctionBlockRemoveInputPort(fb, port);
          },
          py::arg("port"));

    functionBlockClass.def("_remove_signal",
          [](daq::IFunctionBlock* fb, daq::ISignalConfig* signal)
          {
              py::gil_scoped_release release;
              daq::pythonFunctionBlockRemoveSignal(fb, signal);
          },
          py::arg("signal"));
}

// Explicit instantiation for the concrete wrapper type used in this module.
template void definePythonFunctionBlockSupport<PyDaqIntf<daq::IFunctionBlock, daq::IFolder>>(
    pybind11::module_ /*m*/,
    PyDaqIntf<daq::IFunctionBlock, daq::IFolder> /*functionBlockClass*/);

