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
#include <opendaq/function_block.h>

BEGIN_NAMESPACE_OPENDAQ

/*#
 * [interfaceSmartPtr(IFunctionBlock, GenericFunctionBlockPtr)]
 */

/*!
 * @ingroup opendaq_function_blocks
 * @addtogroup opendaq_channel Channel
 * @{
 */

/*!
 * @brief Channels represent physical sensors of openDAQ devices. Internally
 * they are standard function blocks with an additional option to provide a list of
 * tags.
 */
DECLARE_OPENDAQ_INTERFACE(IChannel, IFunctionBlock)
{
};
/*!@}*/

END_NAMESPACE_OPENDAQ
