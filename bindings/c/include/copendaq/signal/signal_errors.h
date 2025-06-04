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

#define DAQ_ERRTYPE_SIGNAL 0x0Au

#define DAQ_ERR_RANGE_BOUNDARIES_INVALID             DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x001u)
#define DAQ_ERR_DIMENSION_IMPLICIT                   DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x002u)
#define DAQ_ERR_DIMENSION_EXPLICIT                   DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x003u)
#define DAQ_ERR_CONFIGURATION_INCOMPLETE             DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x004u)
#define DAQ_ERR_INVALID_DIMENSION_LABEL_TYPES        DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x005u)
#define DAQ_ERR_INVALID_PARAMETERS                   DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x006u)
#define DAQ_ERR_UNKNOWN_RULE_TYPE                    DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x007u)
#define DAQ_ERR_INVALID_SAMPLE_TYPE                  DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x008u)
#define DAQ_ERR_SIGNAL_NOT_ACCEPTED                  DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x009u)
#define DAQ_ERR_PACKET_MEMORY_ALLOCATION             DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x00Au)
#define DAQ_ERR_PACKET_MEMORY_DEALLOCATION           DAQ_ERROR_CODE(DAQ_ERRTYPE_SIGNAL, 0x00Bu)

/*!
 * @}
 */
