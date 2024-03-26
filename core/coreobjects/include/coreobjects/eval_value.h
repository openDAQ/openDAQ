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
#include <coretypes/coretypes.h>
#include <coreobjects/property_object.h>
#include <coretypes/listobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup objects_property
 * @addtogroup objects_eval_value_obj EvalValue 
 * @{
 */

/*!
 * @brief Dynamic expression evaluator
 *
 * Provides dynamic evaluation of expressions. Expression is passed as an argument to a
 * factory function. Expression is evaluated at runtime when result value is requested.
 */
    
DECLARE_OPENDAQ_INTERFACE(IEvalValue, IBaseObject)
{
    /*!
     * @brief Gets the expression.
     * @param[out] eval The expression.
     *
     * Expression is passed as a parameter to the factory function.
     */
    virtual ErrCode INTERFACE_FUNC getEval(IString** eval) = 0;

    /*!
     * @brief Gets the result of the expression.
     * @param[out] obj The result of the expression.
     * @retval OPENDAQ_ERR_CALCFAILED when calculation failed.
     * @retval OPENDAQ_ERR_RESOLVEFAILED when reference resolution failed
     * @retval OPENDAQ_ERR_PARSEFAILED when expression parsing failed
     *
     * When this method is called for the first time, the object will trigger
     * execution and return result.
     */
    virtual ErrCode INTERFACE_FUNC getResult(IBaseObject** obj) = 0;

    /*!
     * @brief Clones the object and attaches an owner.
     * @param owner The owner to attach to the cloned eval value.
     * @param[out] clonedValue The cloned object.
     *
     * When the expression contains reference to some property object, then the expression cannot be
     * evaluated unless an owner is attached to eval value. However, the object can be cloned with the
     * specified owner attached. The client can then evaluate the cloned object.
     */
    virtual ErrCode INTERFACE_FUNC cloneWithOwner(IPropertyObject* owner, IEvalValue** clonedValue) = 0;

    /*!
     * @brief Returns the parse error code.
     *
     * When an eval value object is created, the expression is passed as an argument to the factory
     * function. Parsing of the expression can fail, but the factory function will always succeed. Use this
     * function to check if the parsing of the expression succeeded without evaluating the expression.
     */
    virtual ErrCode INTERFACE_FUNC getParseErrorCode() = 0;

    // [templateType(propertyReferences, IString)]
    /*!
     * @brief Returns the names of all properties referenced by the eval value.
     * @param[out] propertyReferences The names of referenced properties.
     *
     * Referenced properties are all occurrences matching the '"%" propref' pattern in the evaluation string.
     */
    virtual ErrCode INTERFACE_FUNC getPropertyReferences(IList** propertyReferences) = 0;
};

/*!
 *@}
 */

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, EvalValue, IString*, eval)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, EvalValueArgs, IEvalValue, IString*, eval, IList*, args)

OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, EvalValueFunc, IEvalValue, IString*, eval, IFunction*, func)

END_NAMESPACE_OPENDAQ
