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

#include <pybind11/pybind11.h>

#include "py_opendaq/py_opendaq.h"

namespace py = pybind11;

PyDaqIntf<daq::IPythonFunctionBlock, daq::IFunctionBlock> declareIPythonFunctionBlock(pybind11::module_ m)
{
    return wrapInterface<daq::IPythonFunctionBlock, daq::IFunctionBlock>(m, "IPythonFunctionBlock");
}

void defineIPythonFunctionBlock(pybind11::module_ /*m*/,
                                PyDaqIntf<daq::IPythonFunctionBlock, daq::IFunctionBlock> cls)
{
    cls.doc() = "Function block implemented in Python. Provides helper methods for creating ports and signals.";
}

template <typename PythonFunctionBlockClass>
void definePythonFunctionBlock(pybind11::module_ /*m*/, PythonFunctionBlockClass functionBlockClass)
{
    // Methods for Python-implemented function blocks (created from C++ wrapper)
    functionBlockClass.def("_create_and_add_input_port",
          [](daq::IPythonFunctionBlock* fb,
             const std::string& localId,
             daq::PacketReadyNotification notificationMethod,
             daq::IBaseObject* customData,
             bool requestGapPackets,
             daq::IPermissions* permissions)
          {
              // IMPORTANT: do not release the GIL here.
              // This method is invoked from Python (via opendaq/function_block.py) while the interpreter
              // is still unwinding temporary argument objects. Releasing the GIL in the middle of the
              // call can cause pybind refcounting (dec_ref) to run without the GIL and abort the process.
              daq::InputPortConfigPtr port;
              daq::checkErrorInfo(fb->createAndAddInputPort(&port,
                                                            daq::String(localId),
                                                            notificationMethod,
                                                            customData,
                                                            requestGapPackets,
                                                            permissions));
              return port;
          },
          py::arg("local_id"),
          py::arg("notification_method"),
          py::arg("custom_data") = nullptr,
          py::arg("request_gap_packets") = false,
          py::arg("permissions") = nullptr);

    functionBlockClass.def("_create_and_add_signal",
          [](daq::IPythonFunctionBlock* fb,
             const std::string& localId,
             daq::IDataDescriptor* descriptor,
             bool visible,
             bool isPublic,
             daq::IPermissions* permissions)
          {
              daq::SignalConfigPtr signal;
              daq::checkErrorInfo(fb->createAndAddSignal(&signal,
                                                         daq::String(localId),
                                                         descriptor,
                                                         visible,
                                                         isPublic,
                                                         permissions));
              return signal;
          },
          py::arg("local_id"),
          py::arg("descriptor") = nullptr,
          py::arg("visible") = true,
          py::arg("is_public") = true,
          py::arg("permissions") = nullptr);

    functionBlockClass.def("_remove_input_port",
          [](daq::IPythonFunctionBlock* fb, daq::IInputPortConfig* port)
          {
              daq::checkErrorInfo(fb->removeInputPort(port));
          },
          py::arg("port"));

    functionBlockClass.def("_remove_signal",
          [](daq::IPythonFunctionBlock* fb, daq::ISignalConfig* signal)
          {
              daq::checkErrorInfo(fb->removeSignal(signal));
          },
          py::arg("signal"));
}

// Explicit instantiation for the concrete wrapper type used in this module.
template void definePythonFunctionBlock<PyDaqIntf<daq::IPythonFunctionBlock, daq::IFunctionBlock>>(
    pybind11::module_ /*m*/,
    PyDaqIntf<daq::IPythonFunctionBlock, daq::IFunctionBlock> /*functionBlockClass*/);