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
BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_utility
 * @{
 */

DECLARE_OPENDAQ_INTERFACE(IErrorGuard, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getFormattedMessage(IString** message) const = 0;
    virtual ErrCode INTERFACE_FUNC getLastErrorInfo(IErrorInfo** errorInfo) const = 0;
    virtual ErrCode INTERFACE_FUNC getErrorInfoList(IList** errorInfos) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(
    LIBRARY_FACTORY, ErrorGuard, 
    ConstCharPtr, fileName,
    Int, fileLine)

END_NAMESPACE_OPENDAQ