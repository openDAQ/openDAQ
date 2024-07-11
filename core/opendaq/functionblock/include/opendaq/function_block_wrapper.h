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
#include <opendaq/function_block.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(ICoercer, CoercerPtr, "<coreobjects/coercer_ptr.h>")]
 * [interfaceSmartPtr(IValidator, ValidatorPtr, "<coreobjects/validator_ptr.h>")]
 */

/*!
 * @ingroup opendaq_function_blocks
 * @addtogroup opendaq_function_block Function block
 * @{
 */

/*!
 * @brief Enables to change the configuration behaviour of a function block.
 *
 * Function block wrapper is used when a parent function block creates a child function block.
 * Often it is required that the child function block does not expose all configuration features
 * to the client SDK code. Some configuration is performed by the parent function block. Therefore
 * it is required that the parent function block is able to hide and/or change configuration
 * parameters of the child function block. 
 *
 * The parent function block should create an instance of a function block wrapper to modify the 
 * configuration interface of the child function block. The original function block is passed as
 * a parameter to factory function. The parent function block should configure access to the
 * original child function block using the functions on function block wrapper. Then it should
 * publish the wrapped function block.
 */
DECLARE_OPENDAQ_INTERFACE(IFunctionBlockWrapper, IBaseObject)
{
    /*!
     * @brief Includes the input port to a list of input ports on the function block.
     *
     * @param inputPortName The name of the input port.
     */
    virtual ErrCode INTERFACE_FUNC includeInputPort(IString* inputPortName) = 0;

    /*!
     * @brief Excludes the input port from a list of input ports on the function block.
     * 
     * @param inputPortName The name of the input port.
     */
    virtual ErrCode INTERFACE_FUNC excludeInputPort(IString* inputPortName) = 0;

    /*!
     * @brief Includes the signal to a list of signals on the function block.
     *
     * @param signalLocalId The local id of the signal.
     */
    virtual ErrCode INTERFACE_FUNC includeSignal(IString* signalLocalId) = 0;

    /*!
     * @brief Excludes the signal from a list of signals on the function block.
     *
     * @param signalLocalId The name of the signal.
     */
    virtual ErrCode INTERFACE_FUNC excludeSignal(IString* signalLocalId) = 0;

    /*!
     * @brief Includes the property to a list of visible properties on the function block.
     *
     * @param propertyName The name of the property.
     *
     * Note that if the property is not visible in the wrapped function block,
     * the property will not be included. The method the return value
     * of `IFunctionBlock.GetVisibleProperties` method.
     */
    virtual ErrCode INTERFACE_FUNC includeProperty(IString* propertyName) = 0;

    /*!
     * @brief Excludes the property from a list of visible properties on the function block.
     *
     * @param propertyName The name of the property.
     *
     * Note that if the property is not visible in the wrapped function block,
     * this method will have no effect. The method affects the return value
     * of `IFunctionBlock.GetVisibleProperties` method.
     */
    virtual ErrCode INTERFACE_FUNC excludeProperty(IString* propertyName) = 0;
  
    /*!
     * @brief Includes the function block to a list of sub-function blocks on the function block.
     *
     * @param functionBlockLocalId The local id of the sub-function block.
     */
    virtual ErrCode INTERFACE_FUNC includeFunctionBlock(IString* functionBlockLocalId) = 0;

    /*!
     * @brief Excludes the function block from a list of sub-function blocks on the function block.
     *
     * @param functionBlockLocalId The local id of the function block.
     */
    virtual ErrCode INTERFACE_FUNC excludeFunctionBlock(IString* functionBlockLocalId) = 0;

    /*!
     * @brief Sets a custom coercer for the property.
     * 
     * @param propertyName The name of the property.
     * @param coercer The custom coercer.
     *
     * The custom coercer is applied before the standard coercer of the property.
     */
    virtual ErrCode INTERFACE_FUNC setPropertyCoercer(IString* propertyName, ICoercer* coercer) = 0;

    /*!
     * @brief Sets a custom validator for the property.
     * 
     * @param propertyName The name of the property.
     * @param validator The custom validator.
     *
     * The custom validator is applied before the standard validator of the property.
     */
    virtual ErrCode INTERFACE_FUNC setPropertyValidator(IString* propertyName, IValidator* validator) = 0;

    // [elementType(enumValues, IString)]
    /*!
     * @brief Sets a list of accepted selection values.
     * 
     * @param propertyName The name of the property.
     * @param enumValues The list of accepted selection values. An element of a list is selection index (int).
     *
     * The list of accepted selection values must be a subset of property selection values.
     */
    virtual ErrCode INTERFACE_FUNC setPropertySelectionValues(IString* propertyName, IList* enumValues) = 0;
    
    /*!
     * @brief Returns the wrapped function block which was passed as a parameter to the 
     * constructor/factory.
     * 
     * @param[out] functionBlock The wrapped function block.
     */
    virtual ErrCode INTERFACE_FUNC getWrappedFunctionBlock(IFunctionBlock** functionBlock) = 0;
};
/*!@}*/

/*#
 * [factory(NoConstructor)]
 */
/*!
 * @brief Creates a function block wrapper.
 * @param functionBlock The function block being wrapped.
 * @param includeInputPortsByDefault True if input ports are published by default.
 * @param includeSignalsByDefault True if signals are published by default.
 * @param includePropertiesByDefault True if properties are published by default.
 * @param includeFunctionBlocksByDefault True if child function blocks are published by default.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    FunctionBlockWrapper,
    IFunctionBlock,
    IFunctionBlock*, functionBlock,
    Bool, includeInputPortsByDefault,
    Bool, includeSignalsByDefault,
    Bool, includePropertiesByDefault,
    Bool, includeFunctionBlocksByDefault
)

END_NAMESPACE_OPENDAQ
