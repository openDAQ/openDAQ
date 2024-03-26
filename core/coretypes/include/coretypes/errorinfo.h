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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_utility
 * @{
 */

/*!
 * @brief Contains detailed information about error.
 *
 * Most of openDAQ's methods in interfaces return `ErrCode`. This is just an integer number. With `IErrorInfo`
 * interface it is possible to attach an error message and a source to the last error. The Interface function that
 * returns an error can create an object that implements this interface and attaches it to thread-local storage
 * using `daqSetErrorInfo` function. The Client can check the return value of an arbitrary interface function and
 * in case of an error, it can check if `IErrorInfo` is stored in thread-local storage using `daqGetErrorInfo` for
 * additional error information.
 *
 * `makeErrorInfo` automatically creates IErrorInfo and calls `daqSetErrorInfo`. In case of an error, `checkErrorInfo`
 * calls `daqGetErrorInfo` to get extended error information and throws an exception.
 *
 * Example:
 * @code
 * ErrCode ISomeInterface::checkValue(Int value)
 * {
 *     if (value < 0)
 *         return makeErrorInfo(OPENDAQ_ERR_INVALIDPARAMETER, "Parameter should be >= 0", nullptr);
 *     return OPENDAQ_SUCCESS;
 * };
 *
 * auto errCode = someInterface->checkValue(-1);
 * checkErrorInfo(errCode); // this will throw InvalidParameterException with above error message
 * @endcode
 */

DECLARE_OPENDAQ_INTERFACE(IErrorInfo, IBaseObject)
{
    /*!
     * @brief Sets the message of the error.
     * @param message Error description.
     */
    virtual ErrCode INTERFACE_FUNC setMessage(IString* message) = 0;
    /*!
     * @brief Gets the message of the error.
     * @param message Error description.
     */
    virtual ErrCode INTERFACE_FUNC getMessage(IString** message) = 0;
    /*!
     * @brief Sets the source of the error.
     * @param source Error source.
     */
    virtual ErrCode INTERFACE_FUNC setSource(IString* source) = 0;
    /*!
     * @brief Gets the source of the error.
     * @param source Error source.
     */
    virtual ErrCode INTERFACE_FUNC getSource(IString** source) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, ErrorInfo)

END_NAMESPACE_OPENDAQ

extern "C" void PUBLIC_EXPORT daqSetErrorInfo(daq::IErrorInfo* errorInfo);
extern "C" void PUBLIC_EXPORT daqGetErrorInfo(daq::IErrorInfo** errorInfo);
extern "C" void PUBLIC_EXPORT daqClearErrorInfo();
