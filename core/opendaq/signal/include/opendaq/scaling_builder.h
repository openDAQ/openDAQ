/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coretypes/stringobject.h>
#include <opendaq/scaling.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [include(ISampleType)]
 * [interfaceLibrary(INumber, CoreTypes)]
 */

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup opendaq_scaling Scaling
 * @{
 */

/*!
 * @brief Configuration component of Scaling objects. Contains setter methods that allow for Scaling
 * parameter configuration, and a `build` method that builds the Scaling object.
 */
DECLARE_OPENDAQ_INTERFACE(IScalingBuilder, IBaseObject)
{
    /*!
     * @brief Builds and returns a Scaling object using the currently set values of the Builder.
     * @param[out] scaling The built Scaling object.
     */
    virtual ErrCode INTERFACE_FUNC build(IScaling** scaling) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the scaling's input data type.
     * @param type The input data type.
     *
     * The input data type corresponds to the raw data passed through the signal path in
     * data packets.
     */
    virtual ErrCode INTERFACE_FUNC setInputDataType(SampleType type) = 0;

    /*!
     * @brief Gets the scaling's input data type.
     * @param[out] type The input data type.
     */
    virtual ErrCode INTERFACE_FUNC getInputDataType(SampleType* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the scaling's output data type.
     * @param type The output data type
     *
     * The output data type corresponds to the type specified in the value descriptor of
     * a signal, and is the type in which said signal's data should be read in after having
     * the scaling applied to it.
     */
    virtual ErrCode INTERFACE_FUNC setOutputDataType(ScaledSampleType type) = 0;

    /*!
     * @brief Gets the scaling's output data type.
     * @param[out] type The output data type
     */
    virtual ErrCode INTERFACE_FUNC getOutputDataType(ScaledSampleType* type) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the type of the scaling that determines how the scaling parameters should be interpreted
     * and how the scaling should be calculated.
     * @param type The type of the scaling.
     */
    virtual ErrCode INTERFACE_FUNC setScalingType(ScalingType type) = 0;

    /*!
     * @brief Gets the type of the scaling that determines how the scaling parameters should be interpreted
     * and how the scaling should be calculated.
     * @param[out] type The type of the scaling.
     */
    virtual ErrCode INTERFACE_FUNC getScalingType(ScalingType* type) = 0;

    // [templateType(parameters, IString, IBaseObject), returnSelf]
    /*!
     * @brief Gets the list of parameters that are used to calculate the scaling in conjunction with the
     * input data.
     * @param parameters The list of parameters. All elements are Number types.
     * @retval OPENDAQ_ERR_FROZEN if the object is frozen.
     */
    virtual ErrCode INTERFACE_FUNC setParameters(IDict* parameters) = 0;

    // [templateType(parameters, IString, IBaseObject)]
    /*!
     * @brief Gets the list of parameters that are used to calculate the scaling in conjunction with the
     * input data.
     * @param[out] parameters The list of parameters. All elements are Number types.
     */
    virtual ErrCode INTERFACE_FUNC getParameters(IDict** parameters) = 0;

    // [returnSelf]
    /*!
     * @brief Adds a string-object pair parameter to the Dictionary of Scaling parameters.
     * @param name The string-type name of the parameter.
     * @param parameter The object-type parameter.
     */
    virtual ErrCode INTERFACE_FUNC addParameter(IString* name, IBaseObject* parameter) = 0;

    // [returnSelf]
    /*!
     * @brief Removes the parameter with the given name from the Dictionary of Scaling parameters.
     */
    virtual ErrCode INTERFACE_FUNC removeParameter(IString* name) = 0;
};
/*!@}*/

/*!
 * @ingroup opendaq_scaling
 * @addtogroup opendaq_scaling_factories Factories
 * @{
 */

/*!
 * @brief Creates a Scaling builder object with no parameters configured.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, ScalingBuilder, IScalingBuilder)

/*!
 * @brief Scaling builder copy factory that creates a configurable Scaling object from a
 * non-configurable one.
 * @param scalingToCopy The scaling of which configuration should be copied.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, ScalingBuilderFromExisting, IScalingBuilder,
    IScaling*, scalingToCopy
)

/*!@}*/

END_NAMESPACE_OPENDAQ
