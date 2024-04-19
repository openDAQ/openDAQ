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

#define OPENDAQ_ERRTYPE_READER 0x0Du

#define OPENDAQ_ERR_READER_UNKNOWN OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_READER, 0x0000u)
#define OPENDAQ_ERR_INVALID_DATA   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_READER, 0x0001u)

/*!
 * @}
 */
