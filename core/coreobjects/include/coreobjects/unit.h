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

BEGIN_NAMESPACE_OPENDAQ

struct IUnitBuilder;

/*!
 * @ingroup opendaq_data_descriptor
 * @addtogroup coreobjects_unit Unit
 * @{
 */

/*!
 * @brief Describes a measurement unit with IDs as defined in
 * <a href="https://unece.org/trade/cefact/UNLOCODE-Download">Codes for Units of Measurement used in International Trade</a>.
 * 
 * Unit objects implement the Struct methods internally and are Core type `ctStruct`.
 *
 * @subsection unit_components Unit components
 * - ID: The unit ID as defined in <a href="https://unece.org/trade/cefact/UNLOCODE-Download">Codes for Units of Measurement used in International Trade</a>.
 *   Should be set to -1 if the unit ID is not available.
 * - Symbol: The symbol of the unit, i.e. "m/s". It is mandatory and must be set before the Unit object is frozen.
 * - Name: The full name of the unit, i.e. "meters per second". The unit's name is optional.
 * - Quantity: The quantity represented by the unit, i.e. "Velocity". The unit's quantity is optional.
 */
DECLARE_OPENDAQ_INTERFACE(IUnit, IBaseObject)
{
    /*!
     * @brief Gets the unit ID as defined in <a href="https://unece.org/trade/cefact/UNLOCODE-Download">Codes for Units of Measurement used in International Trade</a>.
     * @param[out] id The unit ID.
     *
     * Returns -1 if the unit id is not available.
     */
    virtual ErrCode INTERFACE_FUNC getId(Int* id) = 0;
    /*!
     * @brief Gets the symbol of the unit, i.e. "m/s". 
     * @param[out] symbol The unit's symbol.
     */
    virtual ErrCode INTERFACE_FUNC getSymbol(IString** symbol) = 0;
    /*!
     * @brief Gets the full name of the unit, i.e. "meters per second".
     * @param[out] name The unit's full name.
     *
     * `nullptr` if not set.
     */
    virtual ErrCode INTERFACE_FUNC getName(IString** name) = 0;
    /*!
     * @brief Gets the quantity represented by the unit, i.e. "Velocity" 
     * @param[out] quantity The unit's quantity.
     *
     * `nullptr` if not set.
     */
    virtual ErrCode INTERFACE_FUNC getQuantity(IString** quantity) = 0;
};
/*!@}*/

/*!
 * @brief Creates a Unit struct with its `id`, `symbol`, `name`, and `quantity` fields configured.
 *
 * @param id The unit ID as defined in <a href="https://unece.org/trade/cefact/UNLOCODE-Download">Codes for Units of Measurement used in
 * International Trade</a>.
 * @param symbol The symbol of the unit, i.e. "m/s".
 * @param name The full name of the unit, i.e. "meters per second". (optional)
 * @param quantity The quantity represented by the unit, i.e. "Velocity". (optional)
 */
OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Unit,
    Int, id,
    IString*, symbol,
    IString*, name,
    IString*, quantity
)

//[factory(Hide)]
/*!
 * @brief Creates a Unit with Builder
 * @param builder Unit Builder
 */
OPENDAQ_DECLARE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY, UnitFromBuilder, IUnit,
    IUnitBuilder*, builder
)

END_NAMESPACE_OPENDAQ
