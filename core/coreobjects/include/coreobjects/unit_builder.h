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
#include <coreobjects/unit.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup coreobjects_unit Unit
 * @{
 */

/*!
 * @brief Builder component of Unit objects. Contains setter methods to configure the Unit parameters, and a
 * `build` method that builds the Unit object.
 */
DECLARE_OPENDAQ_INTERFACE(IUnitBuilder, IBaseObject)
{
    // [returnSelf]
    /*!
     * @brief Sets the unit ID as defined in <a href="https://unece.org/trade/cefact/UNLOCODE-Download">Codes for Units of Measurement used
     * in International Trade</a>.
     * @param id The unit ID.
     *
     * The ID should be -1 if the unit is not available.
     */
    virtual ErrCode INTERFACE_FUNC setId(Int id) = 0;

    /*!
     * @brief Gets the unit ID as defined in <a href="https://unece.org/trade/cefact/UNLOCODE-Download">Codes for Units of Measurement used
     * in International Trade</a>.
     * @param[out] id The unit ID.
     *
     * Returns -1 if the unit id is not available.
     */
    virtual ErrCode INTERFACE_FUNC getId(Int* id) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the symbol of the unit, i.e. "m/s".
     * @param symbol The unit's symbol.
     */
    virtual ErrCode INTERFACE_FUNC setSymbol(IString* symbol) = 0;

    /*!
     * @brief Gets the symbol of the unit, i.e. "m/s".
     * @param[out] symbol The unit's symbol.
     */
    virtual ErrCode INTERFACE_FUNC getSymbol(IString** symbol) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the full name of the unit, i.e. "meters per second".
     * @param name The unit's full name.
     */
    virtual ErrCode INTERFACE_FUNC setName(IString* name) = 0;

    /*!
     * @brief Gets the full name of the unit, i.e. "meters per second".
     * @param[out] name The unit's full name.
     *
     * `nullptr` if not set.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;

    // [returnSelf]
    /*!
     * @brief Sets the quantity represented by the unit, i.e. "Velocity"
     * @param quantity The unit's quantity.
     */
    virtual ErrCode INTERFACE_FUNC setQuantity(IString* quantity) = 0;

    /*!
     * @brief Gets the quantity represented by the unit, i.e. "Velocity" 
     * @param[out] quantity The unit's quantity.
     *
     * `nullptr` if not set.
     */
    virtual ErrCode INTERFACE_FUNC getQuantity(IString** quantity) = 0;

    /*!
     * @brief Builds and returns a Unit object using the currently set values of the Builder.
     * @param[out] unit The built Unit.
     */
    virtual ErrCode INTERFACE_FUNC build(IUnit** unit) = 0;
};
/*!@}*/

/*!
 * @ingroup coreobjects_unit
 * @addtogroup coreobjects_unit_factories Factories
 * @{
 */

/*!
 * @brief Creates a UnitConfig with no parameters configured.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, UnitBuilder, IUnitBuilder)

/*!
 * @brief UnitConfig copy factory that creates a configurable Unit object from a possibly non-configurable Unit.
 * @param unitToCopy The Unit of which configuration should be copied.
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, UnitBuilderFromExisting, IUnitBuilder,
    IUnit*, unitToCopy
)

/*!@}*/

END_NAMESPACE_OPENDAQ
