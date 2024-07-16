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
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property
 * @addtogroup objects_validator Validator
 * @{
 */

/*!
 * @brief Used by openDAQ properties to validate whether a value fits the value restrictions of the Property.
 *
 * Whenever a value is set to on a Property object, if the corresponding Property has a validator configured, the value will
 * be validated, throwing a validation error, if the value is not compliant with the validation restrictions. For example,
 * a validator can check the written value for lower-than, greater-than, equality, or other number relations.
 *
 * The validation conditions are configured with an evaluation string when the validator is constructed. The string constructs an
 * Eval value that replaces any instance of the keyword "value" or "val" with the value being set. The result of the Eval
 * value evaluation is the output of the `validate` function call. For example, validators created with the string
 * "value == 5" would reject any value that is not equal to 5.
 */

DECLARE_OPENDAQ_INTERFACE(IValidator, IBaseObject)
{
    /*!
     * @brief Checks whether `value` adheres to the validity conditions of the validator.
     * @param propObj Optional property object parameter required if the validation depends on other properties of the Property object.
     * @param value The value to be checked for whether it is valid or not.
     * @retval OPENDAQ_ERR_VALIDATE_FAILED if `value` is invalid.
     * @retval OPENDAQ_SUCCESS if `value` is valid.
     */
    virtual ErrCode INTERFACE_FUNC validate(IBaseObject* propObj, IBaseObject* value) = 0;

    /*!
     * @brief Gets the string expression used when creating the validator.
     * @param[out] eval The validation expression.
     */
    virtual ErrCode INTERFACE_FUNC getEval(IString** eval) = 0;
};

/*!@}*/

/*!
 * @addtogroup objects_validator_factories Factories
 * @{
 */

/*!
 * @brief Creates a Validator with the given evaluation expression.
 * @param eval The evaluation expression used for validation.
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Validator, IString*, eval)

/*!@}*/

END_NAMESPACE_OPENDAQ
