/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <opendaq/log_file_info_builder_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_address_info
 * @addtogroup opendaq_address_info_factories Factories
 * @{
 */
    
/*!
 * @brief Creates an Address with no parameters configured.
 */
inline LogFileInfoPtr LogFileInfo(const StringPtr& name,
                                  LogFileEncodingType encoding = LogFileEncodingType::Utf8,
                                  const StringPtr& description = nullptr,
                                  const StringPtr& localPath = nullptr)
{
    LogFileInfoPtr obj(LogFileInfo_Create(localPath, name, description, encoding));
    return obj;
}

/*!
 * @brief Creates an Address builder with no parameters configured.
 */
inline LogFileInfoBuilderPtr LogFileInfoBuilder()
{
    LogFileInfoBuilderPtr obj(LogFileInfoBuilder_Create());
    return obj;
}
/*!@}*/

END_NAMESPACE_OPENDAQ
