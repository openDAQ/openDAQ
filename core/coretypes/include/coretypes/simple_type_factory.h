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
#include <coretypes/simple_type.h>
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ


/*!
 * @ingroup types_types_simple_type
 * @addtogroup types_types_simple_type_factories Factories
 * @{
 */

/*!
 * @brief Creates a SimpleType from a given CoreType
 * @param coreType The core type that will be wrapped by the SimpleType
 */
inline ObjectPtr<ISimpleType> SimpleType(CoreType coreType)
{
    return {SimpleType_Create(coreType)};
}

/*!@}*/

END_NAMESPACE_OPENDAQ
