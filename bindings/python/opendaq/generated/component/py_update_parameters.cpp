//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (PythonGenerator).
// </auto-generated>
//------------------------------------------------------------------------------

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

#include "py_opendaq/py_opendaq.h"
#include "py_core_types/py_converter.h"


PyDaqIntf<daq::IUpdateParameters, daq::IPropertyObject> declareIUpdateParameters(pybind11::module_ m)
{
    return wrapInterface<daq::IUpdateParameters, daq::IPropertyObject>(m, "IUpdateParameters");
}

void defineIUpdateParameters(pybind11::module_ m, PyDaqIntf<daq::IUpdateParameters, daq::IPropertyObject> cls)
{
    cls.doc() = "IUpdateParameters interface provides a set of methods to give user flexibility to load instance configuration.";

    m.def("UpdateParameters", &daq::UpdateParameters_Create);

    cls.def_property("re_add_devices_enabled",
        [](daq::IUpdateParameters *object)
        {
            const auto objectPtr = daq::UpdateParametersPtr::Borrow(object);
            return objectPtr.getReAddDevicesEnabled();
        },
        [](daq::IUpdateParameters *object, const bool enabled)
        {
            const auto objectPtr = daq::UpdateParametersPtr::Borrow(object);
            objectPtr.setReAddDevicesEnabled(enabled);
        },
        "Returns whether the re-add devices is enabled. If enabled, the devices will be re-added in update process. / Sets the re-add devices enabled flag.");
}
