/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

#include <coretypes/coretypes.h>

#include "py_core_types/py_opendaq_daq.h"
#include "py_core_types/py_procedure.h"
#include "py_core_types/py_converter.h"

void declareAndDefineIBaseObject(pybind11::module_ m);

PyDaqIntf<daq::IInteger> declareIInteger(pybind11::module_ m);
PyDaqIntf<daq::IFloat> declareIFloat(pybind11::module_ m);
PyDaqIntf<daq::IBoolean> declareIBoolean(pybind11::module_ m);
PyDaqIntf<daq::IString> declareIString(pybind11::module_ m);
PyDaqIntf<daq::IRatio> declareIRatio(pybind11::module_ m);
PyDaqIntf<daq::IComplexNumber> declareIComplexNumber(pybind11::module_ m);
PyDaqIntf<daq::INumber> declareINumber(pybind11::module_ m);

PyDaqIntf<daq::IIterable> declareIIterable(pybind11::module_ m);
PyDaqIntf<daq::IIterator> declareIIterator(pybind11::module_ m);
PyDaqIntf<daq::IList> declareIList(pybind11::module_ m);
PyDaqIntf<daq::IDict> declareIDict(pybind11::module_ m);
PyDaqIntf<daq::IProcedure> declareIProcedure(pybind11::module_ m);
PyDaqIntf<daq::IFunction> declareIFunction(pybind11::module_ m);

// generated
PyDaqIntf<daq::IEventArgs, daq::IBaseObject> declareIEventArgs(pybind11::module_ m);
PyDaqIntf<daq::IType, daq::IBaseObject> declareIType(pybind11::module_ m);
PyDaqIntf<daq::ISimpleType, daq::IType> declareISimpleType(pybind11::module_ m);
PyDaqIntf<daq::ITypeManager, daq::IBaseObject> declareITypeManager(pybind11::module_ m);
PyDaqIntf<daq::IStructType, daq::IType> declareIStructType(pybind11::module_ m);
PyDaqIntf<daq::IStruct, daq::IBaseObject> declareIStruct(pybind11::module_ m);

void defineIInteger(pybind11::module_ m, PyDaqIntf<daq::IInteger> cls);
void defineIFloat(pybind11::module_ m, PyDaqIntf<daq::IFloat> cls);
void defineIBoolean(pybind11::module_ m, PyDaqIntf<daq::IBoolean> cls);
void defineIString(pybind11::module_ m, PyDaqIntf<daq::IString> cls);
void defineIRatio(pybind11::module_ m, PyDaqIntf<daq::IRatio> cls);
void defineIComplexNumber(pybind11::module_ m, PyDaqIntf<daq::IComplexNumber> cls);
void defineINumber(pybind11::module_ m, PyDaqIntf<daq::INumber> cls);

void defineIIterable(pybind11::module_ m, PyDaqIntf<daq::IIterable> cls);
void defineIIterator(pybind11::module_ m, PyDaqIntf<daq::IIterator> cls);
void defineIList(pybind11::module_ m, PyDaqIntf<daq::IList> cls);
void defineIDict(pybind11::module_ m, PyDaqIntf<daq::IDict> cls);
void defineIProcedure(pybind11::module_ m, PyDaqIntf<daq::IProcedure> cls);
void defineIFunction(pybind11::module_ m, PyDaqIntf<daq::IFunction> cls);

// generated
void defineIEventArgs(pybind11::module_ m, PyDaqIntf<daq::IEventArgs, daq::IBaseObject> cls);
void defineIType(pybind11::module_ m, PyDaqIntf<daq::IType, daq::IBaseObject> cls);
void defineISimpleType(pybind11::module_ m, PyDaqIntf<daq::ISimpleType, daq::IType> cls);
void defineITypeManager(pybind11::module_ m, PyDaqIntf<daq::ITypeManager, daq::IBaseObject> cls);
void defineIStructType(pybind11::module_ m, PyDaqIntf<daq::IStructType, daq::IType> cls);
void defineIStruct(pybind11::module_ m, PyDaqIntf<daq::IStruct, daq::IBaseObject> cls);

void wrapDaqComponentCoreTypes(pybind11::module_ m);
