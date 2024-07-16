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

BEGIN_NAMESPACE_OPENDAQ

extern "C"
ErrCode PUBLIC_EXPORT createJsonSerializer(ISerializer** obj, Bool pretty = False);

inline ISerializer* JsonSerializer_Create(Bool pretty = False)
{
    ISerializer* obj;
    ErrCode res = createJsonSerializer(&obj, pretty);
    if (OPENDAQ_SUCCEEDED(res))
        return obj;

    throw std::bad_alloc();
}

END_NAMESPACE_OPENDAQ
