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

#include <coretypes/common.h>

#define BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE BEGIN_NAMESPACE_OPENDAQ_MODULE(parquet_recorder_module)
#define END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE END_NAMESPACE_OPENDAQ_MODULE

BEGIN_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE
/*!
 * @brief The type ID of this function block.
 */
static constexpr const char* TYPE_ID = "ParquetRecorder";

/*!
 * @brief Contains constants for the names of tags assigned to this function block.
 */
struct Tags
{
    /*!
     * @brief A tag identifying this function block as a recorder.
     */
    static constexpr const char* RECORDER = "Recorder";
};

/*!
 * Contains constants for the names of properties supported by this function block.
 */
struct Props
{
    /*!
     * @brief The absolute path to the directory where Parquet files should be written.
     *
     * A separate Parquet file is written for each recorded signal. The current implementation
     * interprets relative paths with respect to the current working directory of the
     * process, but this behavior is not guaranteed.
     */
    static constexpr const char* PATH = "Path";
};
END_NAMESPACE_OPENDAQ_PARQUET_RECORDER_MODULE
