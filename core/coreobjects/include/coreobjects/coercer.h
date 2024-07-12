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
#include <coretypes/common.h>
#include <coretypes/function.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property
 * @addtogroup objects_coercer Coercer
 * @{
 */

/*!
 * @brief Used by openDAQ properties to coerce a value to match the restrictions imposed by the Property.
 *
 * Whenever a value is set to on a Property object, if the corresponding Property has a coercer configured, the value will
 * be evaluated and modified to fit the restrictions imposed by the coercer. For example, a coercer can enforce lower-than,
 * greater-than, equality, or other number relations on written values.
 *
 * The coercion conditions are configured with an evaluation string when the coercer is constructed. The string constructs an
 * Eval value that replaces any instance of the keyword "value" or "val" with the value being set. The result of the Eval
 * value evaluation is the output of the `coerce` function call. For example, coercers created with the string
 * "if(value > 5, 5, value)" would enforce that the property value is always equal to or lower than 5.
 */

DECLARE_OPENDAQ_INTERFACE(ICoercer, IBaseObject)
{
    /*!
     * @brief Coerces `value` to match the coercion restrictions and outputs the result.
     * @param propObj Optional property object parameter required if the coercion depends on other properties of the Property object.
     * @param value The value to be coerced to fit the restrictions.
     * @param[out] result The coercer output containing the modified `value`.
     * @retval OPENDAQ_ERR_COERCE_FAILED if `value` cannot be coerced to fit the restrictions.
     * @retval OPENDAQ_SUCCESS If the value either already fits the restrictions, or was successfully modified to do so.
     */
    virtual ErrCode INTERFACE_FUNC coerce(IBaseObject* propObj, IBaseObject* value, IBaseObject** result) = 0;

    /*!
     * @brief Gets the string expression used when creating the coercer.
     * @param[out] eval The coercion expression.
     */
    virtual ErrCode INTERFACE_FUNC getEval(IString** eval) = 0;
};

/*!@}*/

/*!
 * @addtogroup objects_coercer_factories Factories
 * @{
 */

/*!
 * @brief Creates a Coercer with the given evaluation expression.
 * @param eval The evaluation expression used for coercion.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Coercer, IString*, eval)

/*!@}*/

END_NAMESPACE_OPENDAQ
