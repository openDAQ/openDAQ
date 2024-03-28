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
#include <opendaq/logger_component.h>
#include <opendaq/logger_sink.h>
#include <coretypes/baseobject.h>
#include <coretypes/common.h>
#include <coretypes/listobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_logger
 * @addtogroup opendaq_logger_logger Logger
 * @{
 */

/*#
 * [include(ILoggerSink)]
 */
/*!
 * @brief Represents a collection of @ref ILoggerComponent "Logger Components" with multiple
 * @ref ILoggerSink "Logger Sinks" and a single @ref ILoggerThreadPool "Logger Thread Pool"
 * shared between components.
 *
 * Logger is used to create, manage and maintain Logger Components associated with different parts of
 * the openDAQ SDK. The Logger provides methods, allowing for components to be added and removed dynamically.
 * The components added within the same Logger object should have unique names. Each newly added
 * component inherits threshold log severity level from the Logger. Then this level can be changed
 * independently per component. The set of sinks is initialized on the Logger object creation
 * and cannot be changed after.
 *
 * Additionally, Logger provides the ability to manage flushing policies for the added components,
 * see `flushOnLevel` method.
 */
DECLARE_OPENDAQ_INTERFACE(ILogger, IBaseObject)
{
    /*!
     * @brief Sets the default log severity level.
     * @param level The log severity level.
     */
    virtual ErrCode INTERFACE_FUNC setLevel(LogLevel level) = 0;

    /*!
     * @brief Gets the default log severity level.
     * @param[out] level The log severity level.
     */
    virtual ErrCode INTERFACE_FUNC getLevel(LogLevel* level) = 0;
    
    /*!
     * @brief Gets an added component by name or creates a new one with a given name and adds it to the Logger object.
     * @param name The component's name.
     * @param[out] component The logger component with the name equal to @p `name`.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER if @p name is empty string.
     */
    virtual ErrCode INTERFACE_FUNC getOrAddComponent(IString* name, ILoggerComponent** component) = 0;

    /*!
     * @brief Creates a component with a given name and adds it to the Logger object.
     * @param name The component's name.
     * @param[out] component Added component.
     * @retval OPENDAQ_ERR_INVALIDPARAMETER if @p name is empty string.
     */
    virtual ErrCode INTERFACE_FUNC addComponent(IString* name, ILoggerComponent** component) = 0;

    /*!
     * @brief Removes the component with a given name from the Logger object.
     * @param name The component's name.
     * @retval OPENDAQ_ERR_NOTFOUND if a component with the specified @p name was not added.
     */
    virtual ErrCode INTERFACE_FUNC removeComponent(IString* name) = 0;

    // [elementType(components, ILoggerComponent)]
    /*!
     * @brief Gets a list of added components.
     * @param[out] components The list of added components.
     */
    virtual ErrCode INTERFACE_FUNC getComponents(IList** components) = 0;

    /*!
     * @brief Gets an added component by name.
     * @param name The component's name.
     * @param[out] component The logger component with the name equal to @p `name`.
     * @retval OPENDAQ_ERR_NOTFOUND if a component with the specified @p name was not added.
     */
    virtual ErrCode INTERFACE_FUNC getComponent(IString* name, ILoggerComponent** component) = 0;

    /*!
     * @brief Triggers writing out the messages stored in temporary buffers for added components
     * and sinks associated with the Logger object.
     */
    virtual ErrCode INTERFACE_FUNC flush() = 0;

    /*!
     * @brief Sets the minimum severity level of messages to be automatically flushed by components of Logger object.
     * @param level The log severity level.
     */
    virtual ErrCode INTERFACE_FUNC flushOnLevel(LogLevel level) = 0;
};

/*!@}*/

// [templateType(sinks, ILoggerSink)]

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Logger, IList*, sinks, LogLevel, level)

END_NAMESPACE_OPENDAQ
