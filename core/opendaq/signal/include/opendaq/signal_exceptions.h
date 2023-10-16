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
#include <coretypes/exceptions.h>
#include <opendaq/signal_errors.h>

DEFINE_EXCEPTION(RangeBoundariesInvalid, OPENDAQ_ERR_RANGE_BOUNDARIES_INVALID, "The low and high boundaries of the range are invalid.")
DEFINE_EXCEPTION(DimensionImplicit, OPENDAQ_ERR_DIMENSION_IMPLICIT, "Operation is invalid on implicit dimensions.")
DEFINE_EXCEPTION(DimensionExplicit, OPENDAQ_ERR_DIMENSION_EXPLICIT, "Operation is invalid on explicit dimensions.")
DEFINE_EXCEPTION(ConfigurationIncomplete, OPENDAQ_ERR_CONFIGURATION_INCOMPLETE, "Call not allowed until configuration is complete.")
DEFINE_EXCEPTION(InvalidLabelTypes, OPENDAQ_ERR_INVALID_DIMENSION_LABEL_TYPES, "Dimension explicit labels are of an invalid type.")
DEFINE_EXCEPTION(InvalidParameters, OPENDAQ_ERR_INVALID_PARAMETERS, "Parameters of rule/scaling are not valid.")
DEFINE_EXCEPTION(UnknownRuleType, OPENDAQ_ERR_UNKNOWN_RULE_TYPE, "Rule is custom and cannot be interpreted by openDAQ.")
DEFINE_EXCEPTION(InvalidSampleType, OPENDAQ_ERR_INVALID_SAMPLE_TYPE, "Provided sample type is not supported.")
DEFINE_EXCEPTION(SignalNotAccepted, OPENDAQ_ERR_SIGNAL_NOT_ACCEPTED, "Input port does not accept the provided signal.")
DEFINE_EXCEPTION(MemoryAllocationFailed, OPENDAQ_ERR_PACKET_MEMORY_ALLOCATION, "Packet data buffer memory allocation failed.")
DEFINE_EXCEPTION(MemoryDeallocationFailed, OPENDAQ_ERR_PACKET_MEMORY_DEALLOCATION, "Packet data buffer memory deallocation failed.")
