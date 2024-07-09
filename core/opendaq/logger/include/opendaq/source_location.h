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
#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_logger
 * @addtogroup opendaq_logger_source_location Source location
 * @{
 */

#ifdef __cplusplus
    #define OPENDAQ_CONSTEXPR constexpr
    #define OPENDAQ_NOEXCEPT noexcept
#else
    #define OPENDAQ_CONSTEXPR
    #define OPENDAQ_NOEXCEPT
#endif

#pragma pack(push, 1)

/*!
 * @brief Represents a position in source code.
 */
struct SourceLocation
{
    OPENDAQ_CONSTEXPR SourceLocation() = default;
    OPENDAQ_CONSTEXPR SourceLocation(const char* fileName, Int line, const char* funcName)
        : fileName{fileName}
        , line{line}
        , funcName{funcName}
    {
    }

    OPENDAQ_CONSTEXPR bool empty() const OPENDAQ_NOEXCEPT
    {
        return line == 0;
    }

    const char* fileName{nullptr};  ///< Source file name
    Int line{0};                    ///< Line number
    const char* funcName{nullptr};  ///< Function name
};

/*!@}*/

#pragma pack(pop)

END_NAMESPACE_OPENDAQ
