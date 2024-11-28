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
#include <coreobjects/coreobjects.h>
#include "py_core_types/py_opendaq_daq.h"

void wrapDaqComponentCoreObjects(pybind11::module_ m);

PyDaqIntf<daq::IArgumentInfo, daq::IBaseObject> declareIArgumentInfo(pybind11::module_ m);
PyDaqIntf<daq::ICallableInfo, daq::IBaseObject> declareICallableInfo(pybind11::module_ m);
PyDaqIntf<daq::ICoercer, daq::IBaseObject> declareICoercer(pybind11::module_ m);
PyDaqIntf<daq::IEvalValue, daq::IBaseObject> declareIEvalValue(pybind11::module_ m);
PyDaqIntf<daq::IOwnable, daq::IBaseObject> declareIOwnable(pybind11::module_ m);
PyDaqIntf<daq::IProperty, daq::IBaseObject> declareIProperty(pybind11::module_ m);
PyDaqIntf<daq::IPropertyBuilder, daq::IBaseObject> declareIPropertyBuilder(pybind11::module_ m);
PyDaqIntf<daq::IPropertyObject, daq::IBaseObject> declareIPropertyObject(pybind11::module_ m);
PyDaqIntf<daq::IPropertyObjectClass, daq::IType> declareIPropertyObjectClass(pybind11::module_ m);
PyDaqIntf<daq::IPropertyObjectClassBuilder, daq::IBaseObject> declareIPropertyObjectClassBuilder(pybind11::module_ m);
PyDaqIntf<daq::IPropertyObjectProtected, daq::IBaseObject> declareIPropertyObjectProtected(pybind11::module_ m);
PyDaqIntf<daq::IPropertyValueEventArgs, daq::IEventArgs> declareIPropertyValueEventArgs(pybind11::module_ m);
PyDaqIntf<daq::IValidator, daq::IBaseObject> declareIValidator(pybind11::module_ m);
PyDaqIntf<daq::IUnit, daq::IBaseObject> declareIUnit(pybind11::module_ m);
PyDaqIntf<daq::IUnitBuilder, daq::IBaseObject> declareIUnitBuilder(pybind11::module_ m);
PyDaqIntf<daq::IUser, daq::IBaseObject> declareIUser(pybind11::module_ m);
PyDaqIntf<daq::IAuthenticationProvider, daq::IBaseObject> declareIAuthenticationProvider(pybind11::module_ m);
PyDaqIntf<daq::IPermissionsBuilder, daq::IBaseObject> declareIPermissionsBuilder(pybind11::module_ m);
PyDaqIntf<daq::IPermissionMaskBuilder, daq::IBaseObject> declareIPermissionMaskBuilder(pybind11::module_ m);
PyDaqIntf<daq::IPermissionManager, daq::IBaseObject> declareIPermissionManager(pybind11::module_ m);
PyDaqIntf<daq::IPermissions, daq::IBaseObject> declareIPermissions(pybind11::module_ m);

void defineIArgumentInfo(pybind11::module_ m, PyDaqIntf<daq::IArgumentInfo, daq::IBaseObject> cls);
void defineICallableInfo(pybind11::module_ m, PyDaqIntf<daq::ICallableInfo, daq::IBaseObject> cls);
void defineICoercer(pybind11::module_ m, PyDaqIntf<daq::ICoercer, daq::IBaseObject> cls);
void defineIEvalValue(pybind11::module_ m, PyDaqIntf<daq::IEvalValue, daq::IBaseObject> cls);
void defineIOwnable(pybind11::module_ m, PyDaqIntf<daq::IOwnable, daq::IBaseObject> cls);
void defineIProperty(pybind11::module_ m, PyDaqIntf<daq::IProperty, daq::IBaseObject> cls);
void defineIPropertyBuilder(pybind11::module_ m, PyDaqIntf<daq::IPropertyBuilder, daq::IBaseObject> cls);
void defineIPropertyObject(pybind11::module_ m, PyDaqIntf<daq::IPropertyObject, daq::IBaseObject> cls);
void defineIPropertyObjectClass(pybind11::module_ m, PyDaqIntf<daq::IPropertyObjectClass, daq::IType> cls);
void defineIPropertyObjectClassBuilder(pybind11::module_ m, PyDaqIntf<daq::IPropertyObjectClassBuilder, daq::IBaseObject> cls);
void defineIPropertyObjectProtected(pybind11::module_ m, PyDaqIntf<daq::IPropertyObjectProtected, daq::IBaseObject> cls);
void defineIPropertyValueEventArgs(pybind11::module_ m, PyDaqIntf<daq::IPropertyValueEventArgs, daq::IEventArgs> cls);
void defineIValidator(pybind11::module_ m, PyDaqIntf<daq::IValidator, daq::IBaseObject> cls);
void defineIUnit(pybind11::module_ m, PyDaqIntf<daq::IUnit, daq::IBaseObject> cls);
void defineIUnitBuilder(pybind11::module_ m, PyDaqIntf<daq::IUnitBuilder, daq::IBaseObject> cls);
void defineIUser(pybind11::module_ m, PyDaqIntf<daq::IUser, daq::IBaseObject> cls);
void defineIAuthenticationProvider(pybind11::module_ m, PyDaqIntf<daq::IAuthenticationProvider, daq::IBaseObject> cls);
void defineIPermissionsBuilder(pybind11::module_ m, PyDaqIntf<daq::IPermissionsBuilder, daq::IBaseObject> cls);
void defineIPermissionMaskBuilder(pybind11::module_ m, PyDaqIntf<daq::IPermissionMaskBuilder, daq::IBaseObject> cls);
void defineIPermissionManager(pybind11::module_ m, PyDaqIntf<daq::IPermissionManager, daq::IBaseObject> cls);
void defineIPermissions(pybind11::module_ m, PyDaqIntf<daq::IPermissions, daq::IBaseObject> cls);
