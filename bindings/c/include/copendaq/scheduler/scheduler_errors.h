/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <ccoretypes/errors.h>

/*!
 * @ingroup opendaq_errors_group
 * @addtogroup opendaq_errors_macros Error Code Macros
 * @{
 */

#define DAQ_ERRTYPE_SCHEDULER 0x04u

#define DAQ_ERR_SCHEDULER_UNKNOWN DAQ_ERROR_CODE(DAQ_ERRTYPE_SCHEDULER, 0x0000u)
#define DAQ_ERR_SCHEDULER_STOPPED DAQ_ERROR_CODE(DAQ_ERRTYPE_SCHEDULER, 0x0001u)
#define DAQ_ERR_NOT_ENOUGH_TASKS  DAQ_ERROR_CODE(DAQ_ERRTYPE_SCHEDULER, 0x0002u)
#define DAQ_ERR_EMPTY_AWAITABLE   DAQ_ERROR_CODE(DAQ_ERRTYPE_SCHEDULER, 0x0003u)

/*!
 * @}
 */
