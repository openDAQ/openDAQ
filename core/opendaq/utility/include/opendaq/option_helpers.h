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
#include <coretypes/dict_ptr.h>
#include <coretypes/dictobject.h>

BEGIN_NAMESPACE_OPENDAQ

inline bool getPrettyPrintOnSaveConfig(const DictObjectPtr<IDict, IString, IBaseObject>& options)
{
    if (!options.hasKey("Configuration"))
        return false;

    const auto configurationOptions = options.get("Configuration").template asPtrOrNull<IDict, DictObjectPtr<IDict, IString, IBaseObject>>(true);
    if (!configurationOptions.assigned())
        return false;

    if (configurationOptions.hasKey("SerializePrettyPrint"))
        return configurationOptions.get("SerializePrettyPrint");

    return false;
}


END_NAMESPACE_OPENDAQ

