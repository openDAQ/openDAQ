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

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum daqOperationModeType
    {
        daqOperationModeTypeUnknown = 0,
        daqOperationModeTypeIdle,
        daqOperationModeTypeOperation,
        daqOperationModeTypeSafeOperation
    } daqOperationModeType;

    typedef enum daqComponentTypeSort
    {
        daqComponentTypeSortUndefined = 0,
        daqComponentTypeSortServer = 1,
        daqComponentTypeSortDevice,
        daqComponentTypeSortFunctionBlock,
        daqComponentTypeSortStreaming
    } daqComponentTypeSort;

#ifdef __cplusplus
}
#endif