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
#include <coretypes/common.h>
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>
#include <coretypes/listobject.h>

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
 * `DAQ_MAKE_ERROR_INFO` automatically creates IErrorInfo and calls `daqSetErrorInfo`. In case of an error, `DAQ_CHECK_ERROR_INFO`
 * calls `daqGetErrorInfo` to get extended error information and throws an exception.
 *
 * Example:
 * @code
 * ErrCode ISomeInterface::checkValue(Int value)
 * {
 *     if (value < 0)
 *         return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDPARAMETER, "Parameter should be >= 0");
 *     return OPENDAQ_SUCCESS;
 * };
 *
 * auto errCode = someInterface->checkValue(-1);
 * DAQ_CHECK_ERROR_INFO(errCode); // this will throw InvalidParameterException with above error message
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

    /*!
     * @brief Sets the file name where the error occurred.
     * @param fileName File name.
     */
    virtual ErrCode INTERFACE_FUNC setFileName(ConstCharPtr fileName) = 0;

    /*!
     * @brief Gets the file name where the error occurred.
     * @param fileName File name.
     */
    virtual ErrCode INTERFACE_FUNC getFileName(ConstCharPtr* fileName) = 0;

    /*!
     * @brief Sets the line number in the file where the error occurred.
     * @param fileLine Line number.
     */
    virtual ErrCode INTERFACE_FUNC setFileLine(Int fileLine) = 0;

    /*!
     * @brief Gets the line number in the file where the error occurred.
     * @param fileLine Line number.
     */
    virtual ErrCode INTERFACE_FUNC getFileLine(Int* fileLine) = 0;

    /*!
     * @brief Extends the current error information with additional error details.
     * @param errorInfo The additional error information to merge into the current error info.
     *
     * This method allows appending or merging additional error information from another
     * `IErrorInfo` object into the current error info. It is useful for tracing errors
     * across multiple layers or modules.
     */
    virtual ErrCode INTERFACE_FUNC extend(IErrorInfo* errorInfo) = 0;

    /*!
     * @brief Gets a formatted error message containing the error description, file name, and line number.
     * @param message A pointer to store the formatted error message.
     *
     * This method returns a string that combines the error message, the file name where the error occurred,
     * and the line number in a human-readable format. It is useful for logging or displaying detailed error
     * information.
     */
    virtual ErrCode INTERFACE_FUNC getFormatMessage(IString** message) = 0;
};

/*!@}*/

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, ErrorInfo)

END_NAMESPACE_OPENDAQ

extern "C" void PUBLIC_EXPORT daqSetErrorInfo(daq::IErrorInfo* errorInfo);
extern "C" void PUBLIC_EXPORT daqExtendErrorInfo(daq::IErrorInfo* errorInfo);
extern "C" void PUBLIC_EXPORT daqGetErrorInfo(daq::IErrorInfo** errorInfo);
extern "C" void PUBLIC_EXPORT daqGetErrorInfoList(daq::IList** errorInfoList);
extern "C" void PUBLIC_EXPORT daqClearErrorInfo();
