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

#include "py_core_objects/py_core_objects.h"
#include "py_core_types/py_converter.h"


PyDaqIntf<daq::IPermissionManager, daq::IBaseObject> declareIPermissionManager(pybind11::module_ m)
{
    return wrapInterface<daq::IPermissionManager, daq::IBaseObject>(m, "IPermissionManager");
}

void defineIPermissionManager(pybind11::module_ m, PyDaqIntf<daq::IPermissionManager, daq::IBaseObject> cls)
{
    cls.doc() = "A class which is responsible for managing permissions on an object level. Given a user's group, it is possible to restrict or allow read, write and execute permissions for each object. It is also possible to specify if permissions are inherited from parent object";

    m.def("PermissionManager", &daq::PermissionManager_Create);
    m.def("DisabledPermissionManager", &daq::DisabledPermissionManager_Create);

    cls.def_property("permissions",
        nullptr,
        [](daq::IPermissionManager *object, daq::IPermissions* permissions)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PermissionManagerPtr::Borrow(object);
            objectPtr.setPermissions(permissions);
        },
        "Set object permission configuration.");
    cls.def("is_authorized",
        [](daq::IPermissionManager *object, daq::IUser* user, daq::Permission permission)
        {
            py::gil_scoped_release release;
            const auto objectPtr = daq::PermissionManagerPtr::Borrow(object);
            return objectPtr.isAuthorized(user, permission);
        },
        py::arg("user"), py::arg("permission"),
        "Check if user has a given permission on an object of the permission manager.");
}
