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
#include <opendaq/tags_config_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_tags
 * @addtogroup opendaq_tag_factories Factories
 * @{
 */

/*!
 * @brief Creates a new Tags object with an empty list of tags.
 */
inline TagsConfigPtr Tags()
{
    return TagsConfigPtr(Tags_Create());
}

/*!
 * @brief Creates a new Tags object with the same tags as the other one.
 */
inline TagsConfigPtr TagsCopy(TagsPtr tags)
{
    return TagsConfigPtr(TagsFromExisting_Create(tags));
}

/*!@}*/

END_NAMESPACE_OPENDAQ
