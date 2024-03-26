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
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include "py_core_objects/py_core_objects.h"
#include "py_core_types/py_converter.h"

PyDaqIntf<daq::IUnitBuilder, daq::IBaseObject> declareIUnitBuilder(pybind11::module_ m)
{
    return wrapInterface<daq::IUnitBuilder, daq::IBaseObject>(m, "IUnitBuilder");
}

void defineIUnitBuilder(pybind11::module_ m, PyDaqIntf<daq::IUnitBuilder, daq::IBaseObject> cls)
{
    cls.doc() = "Builder component of Unit objects. Contains setter methods to configure the Unit parameters, and a `build` method that builds the Unit object.";

    m.def("UnitBuilder", &daq::UnitBuilder_Create);
    m.def("UnitBuilderFromExisting", &daq::UnitBuilderFromExisting_Create);

    cls.def("build",
        [](daq::IUnitBuilder *object)
        {
            const auto objectPtr = daq::UnitBuilderPtr::Borrow(object);
            return objectPtr.build().detach();
        },
        "Builds and returns a Unit object using the currently set values of the Builder.");
    cls.def_property("id",
        [](daq::IUnitBuilder *object)
        {
            const auto objectPtr = daq::UnitBuilderPtr::Borrow(object);
            return objectPtr.getId();
        },
        [](daq::IUnitBuilder *object, daq::Int id)
        {
            const auto objectPtr = daq::UnitBuilderPtr::Borrow(object);
            objectPtr.setId(id);
        },
        "Gets the unit ID as defined in <a href=\"https://unece.org/trade/cefact/UNLOCODE-Download\">Codes for Units of Measurement used in International Trade</a>. / Sets the unit ID as defined in <a href=\"https://unece.org/trade/cefact/UNLOCODE-Download\">Codes for Units of Measurement used in International Trade</a>.");
    cls.def_property("symbol",
        [](daq::IUnitBuilder *object)
        {
            const auto objectPtr = daq::UnitBuilderPtr::Borrow(object);
            return objectPtr.getSymbol().toStdString();
        },
        [](daq::IUnitBuilder *object, const std::string& symbol)
        {
            const auto objectPtr = daq::UnitBuilderPtr::Borrow(object);
            objectPtr.setSymbol(symbol);
        },
        "Gets the symbol of the unit, i.e. \"m/s\". / Sets the symbol of the unit, i.e. \"m/s\".");
    cls.def_property("name",
        [](daq::IUnitBuilder *object)
        {
            const auto objectPtr = daq::UnitBuilderPtr::Borrow(object);
            return objectPtr.getName().toStdString();
        },
        [](daq::IUnitBuilder *object, const std::string& name)
        {
            const auto objectPtr = daq::UnitBuilderPtr::Borrow(object);
            objectPtr.setName(name);
        },
        "Gets the full name of the unit, i.e. \"meters per second\". / Sets the full name of the unit, i.e. \"meters per second\".");
    cls.def_property("quantity",
        [](daq::IUnitBuilder *object)
        {
            const auto objectPtr = daq::UnitBuilderPtr::Borrow(object);
            return objectPtr.getQuantity().toStdString();
        },
        [](daq::IUnitBuilder *object, const std::string& quantity)
        {
            const auto objectPtr = daq::UnitBuilderPtr::Borrow(object);
            objectPtr.setQuantity(quantity);
        },
        "Gets the quantity represented by the unit, i.e. \"Velocity\" / Sets the quantity represented by the unit, i.e. \"Velocity\"");
}
