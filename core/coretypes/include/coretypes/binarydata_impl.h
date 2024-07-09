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
#include <coretypes/binarydata.h>
#include <coretypes/intfs.h>
#include <coretypes/coretype.h>

BEGIN_NAMESPACE_OPENDAQ

class BinaryDataImpl : public ImplementationOf<IBinaryData, ICoreType>
{
public:
    explicit BinaryDataImpl(SizeT size);

    ErrCode INTERFACE_FUNC getAddress(void** data) override;
    ErrCode INTERFACE_FUNC getSize(SizeT* size) override;

    ErrCode INTERFACE_FUNC getCoreType(CoreType* coreType) override;

protected:
    void internalDispose(bool) override;

private:
    char* data;
    size_t size;
};

END_NAMESPACE_OPENDAQ
