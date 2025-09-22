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

#pragma once
#include <coretypes/errorinfo.h>
#include <coretypes/objectptr.h>
#include <coretypes/error_guard.h>

BEGIN_NAMESPACE_OPENDAQ

inline ObjectPtr<IErrorInfo> ErrorInfo()
{
    return ErrorInfo_Create();
}

inline ObjectPtr<IErrorGuard> ErrorGuard(ConstCharPtr fileName, Int fileLine)
{
    return ErrorGuard_Create(fileName, fileLine);
}

using ErrorInfoPtr = ObjectPtr<IErrorInfo>;
using ErrorGuardPtr = ObjectPtr<IErrorGuard>;

END_NAMESPACE_OPENDAQ

#define DAQ_ERROR_GUARD() daq::ErrorGuard(__FILE__, __LINE__)
