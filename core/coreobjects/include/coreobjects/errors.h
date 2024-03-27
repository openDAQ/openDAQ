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
#include <coretypes/errors.h>

/*!
 * @ingroup objects_errors_group
 * @addtogroup objects_errors_macros Error Code Macros
 * @{
 */

#define OPENDAQ_ERRTYPE_COREOBJECTS   0x06u
/*!
 * @objects_error{
 * 0x80060001,
 * Failed to retrieve the object's owner as it has no owner.
 * }
 */
#define OPENDAQ_ERR_NO_OWNER             OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_COREOBJECTS, 0x001u)
#define OPENDAQ_ERR_OWNER_EXPIRED        OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_COREOBJECTS, 0x002u)
#define OPENDAQ_ERR_MANAGER_NOT_ASSIGNED OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_COREOBJECTS, 0x002u)

/*!
 * @}
 */
