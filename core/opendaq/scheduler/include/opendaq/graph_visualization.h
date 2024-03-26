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
#include <coretypes/baseobject.h>
#include <coretypes/stringobject.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_scheduler_components
 * @addtogroup opendaq_graph_visualization GraphVisualization
 * @{
 */

/*!
 * @brief Represents a way to get a string representation of a graph
 * usually in some diagram description language like DOT, mermaid or D2.
 */
DECLARE_OPENDAQ_INTERFACE(IGraphVisualization, IBaseObject)
{
    /*!
     * @brief Returns the graph representation as a string.
     * @param[out] dot Graph's string representation
     */
    virtual ErrCode INTERFACE_FUNC dump(IString** dot) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
