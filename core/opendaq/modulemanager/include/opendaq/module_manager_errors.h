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
#include <coretypes/errors.h>

/*!
 * @ingroup opendaq_errors_group
 * @addtogroup opendaq_errors_macros Error Code Macros
 * @{
 */

#define OPENDAQ_ERRTYPE_MODULE_MANAGER 0x03u

#define OPENDAQ_ERR_MODULE_MANAGER_UNKNOWN           OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_MODULE_MANAGER, 0x0000u)
#define OPENDAQ_ERR_MODULE_LOAD_FAILED               OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_MODULE_MANAGER, 0x0001u)
#define OPENDAQ_ERR_MODULE_NO_ENTRY_POINT            OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_MODULE_MANAGER, 0x0002u)
#define OPENDAQ_ERR_MODULE_ENTRY_POINT_FAILED        OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_MODULE_MANAGER, 0x0003u)
#define OPENDAQ_ERR_MODULE_INCOMPATIBLE_DEPENDENCIES OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_MODULE_MANAGER, 0x0004u)

/*!
 * @}
 */
