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
#include <opendaq/function_block_errors.h>
#include <opendaq/module_manager_errors.h>
#include <opendaq/scheduler_errors.h>
#include <opendaq/logger_errors.h>
#include <opendaq/device_errors.h>
#include <opendaq/signal_errors.h>
#include <opendaq/reader_errors.h>

/*!
 * @ingroup opendaq_errors_group
 * @addtogroup opendaq_errors_macros Error Code Macros
 * @{
 */

#define OPENDAQ_ERRTYPE_OPENDAQ 0x01u

#define OPENDAQ_ERR_CONNECTION_LOST             OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_OPENDAQ, 0x001u)
#define OPENDAQ_ERR_CONNECTION_LIMIT_REACHED OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_OPENDAQ, 0x002u)
#define OPENDAQ_ERR_SERVER_VERSION_TOO_LOW OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_OPENDAQ, 0x003u)
#define OPENDAQ_ERR_CONTROL_CLIENT_REJECTED  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_OPENDAQ, 0x004u)

/*!
 * @}
 */
