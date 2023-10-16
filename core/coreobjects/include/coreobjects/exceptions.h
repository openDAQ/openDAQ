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
#include <coretypes/exceptions.h>
#include <coreobjects/errors.h>

BEGIN_NAMESPACE_OPENDAQ

DEFINE_EXCEPTION(CalcFailed, OPENDAQ_ERR_CALCFAILED, "Calculation failed")
DEFINE_EXCEPTION(ManagerNotAssigned, OPENDAQ_ERR_MANAGER_NOT_ASSIGNED, "Property object class manager is not assigned")

END_NAMESPACE_OPENDAQ
