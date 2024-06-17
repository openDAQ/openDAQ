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

#pragma once
#include <opendaq/component_errors.h>
#include <coretypes/exceptions.h>

BEGIN_NAMESPACE_OPENDAQ

class ComponentException : public DaqException
{
public:
    using DaqException::DaqException;
};

#define DEFINE_COMPONENT_EXCEPTION(excName, errCode, excMsg) DEFINE_EXCEPTION_BASE(daq::ComponentException, excName, errCode, excMsg)

/*
 * Should be in the order of the error's numerical value
 */

DEFINE_COMPONENT_EXCEPTION(ComponentRemoved, OPENDAQ_ERR_COMPONENT_REMOVED, "The operation failed because component is removed")

END_NAMESPACE_OPENDAQ
