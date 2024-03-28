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
 * @ingroup opendaq_errors_group
 * @addtogroup opendaq_errors_macros Error Code Macros
 * @{
 */

#define OPENDAQ_ERRTYPE_SIGNAL 0x0Au

#define OPENDAQ_ERR_RANGE_BOUNDARIES_INVALID             OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x001u)
#define OPENDAQ_ERR_DIMENSION_IMPLICIT                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x002u)
#define OPENDAQ_ERR_DIMENSION_EXPLICIT                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x003u)
#define OPENDAQ_ERR_CONFIGURATION_INCOMPLETE             OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x004u)
#define OPENDAQ_ERR_INVALID_DIMENSION_LABEL_TYPES        OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x005u)
#define OPENDAQ_ERR_INVALID_PARAMETERS                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x006u)
#define OPENDAQ_ERR_UNKNOWN_RULE_TYPE                    OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x007u)
#define OPENDAQ_ERR_INVALID_SAMPLE_TYPE                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x008u)
#define OPENDAQ_ERR_SIGNAL_NOT_ACCEPTED                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x009u)
#define OPENDAQ_ERR_PACKET_MEMORY_ALLOCATION             OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x00Au)
#define OPENDAQ_ERR_PACKET_MEMORY_DEALLOCATION           OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_SIGNAL, 0x00Bu)

/*!
 * @}
 */
