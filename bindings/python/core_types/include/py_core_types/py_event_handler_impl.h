/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <coretypes/common.h>
#include <coretypes/intfs.h>
#include <coretypes/objectptr.h>
#include <coretypes/event_handler.h>
#include "py_core_types/py_base_object.h"
#include "py_core_types/py_opendaq_daq.h"

BEGIN_NAMESPACE_OPENDAQ

class PyEventHandlerImpl : public PyObjectImpl<pybind11::object, IEventHandler>
{
public:
    using PyObjectImpl::PyObjectImpl;

    ErrCode INTERFACE_FUNC handleEvent(IBaseObject* sender, IEventArgs* eventArgs) override
    {
        PY_ACQUIRE_GIL_OR_RETURN();
        const ErrCode errCode = daqTry([&] { pyObject(baseObjectToPyObject(sender), baseObjectToPyObject(eventArgs)); });
        OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to handle event in PyEventHandlerImpl");
        return errCode;
    }
};

END_NAMESPACE_OPENDAQ
