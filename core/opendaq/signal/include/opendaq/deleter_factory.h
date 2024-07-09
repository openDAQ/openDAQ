/*
 * Copyright 2022-2024 openDAQ d. o. o.
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
#include <opendaq/deleter_impl.h>
#include <opendaq/deleter_ptr.h>
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_deleter
 * @addtogroup opendaq_deleter_factories Factories
 * @{
 */

/*!
 * @brief Creates a deleter callback which is called when external memory is no longer needed and can be freed.
 *
 * This object returned is used with blueberry packets that are created with external memory. Provider
 * of external memory is responsible to provide a custom deleter, which is called when the packet is
 * destroyed.
 */
template <class Callback>
DeleterPtr Deleter(Callback&& callback)
{
    auto obj = createWithImplementation<IDeleter, DeleterImpl<Callback>>(std::forward<Callback>(callback));
    return obj;
}

/*!@}*/

END_NAMESPACE_OPENDAQ
