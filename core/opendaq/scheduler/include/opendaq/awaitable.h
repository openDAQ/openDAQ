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
#include <coretypes/baseobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scheduler_components
 * @addtogroup opendaq_awaitable Awaitable
 * @{
 */

/*
 * @brief Represents the state of an asynchronous operation that can
 * be canceled, waited upon or queried for the result.
 */
DECLARE_OPENDAQ_INTERFACE(IAwaitable, IBaseObject)
{
    /*!
     * @brief Cancels the outstanding work if it has not already started.
     * @param[out] canceled Is @c true if the execution was canceled
     *                      or @c false if the execution has already completed
     */
    virtual ErrCode INTERFACE_FUNC cancel(Bool* canceled) = 0;

    /*
     * @brief Blocks until the execution finishes ether by exception or providing the result.
     * @retval OPENDAQ_ERR_EMPTY_AWAITABLE when there is no work associated with the awaitable.
     * @retval OPENDAQ_IGNORED when the execution has already finished.
     */
    virtual ErrCode INTERFACE_FUNC wait() = 0;

    /*!
     * @brief Waits until the awaitable has a valid result and retrieves it or
     * re-throws the exception that occurred during the execution.
     * @param[out] result The execution result if any otherwise @c nullptr.
     * @retval OPENDAQ_ERR_EMPTY_AWAITABLE when there is no work associated with the awaitable.
     */
    virtual ErrCode INTERFACE_FUNC getResult(IBaseObject** result) = 0;

    /*!
     * @brief Checks if the execution has already finished.
     * @param[out] completed Is @c true if the execution has finished
     *                       or @c false if the execution is in progress or there is no work associated
     */
    virtual ErrCode INTERFACE_FUNC hasCompleted(Bool* completed) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
