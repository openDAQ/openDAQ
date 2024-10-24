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
#include <opendaq/log_file_info_builder.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class LogFileInfoBuilderImpl : public ImplementationOf<ILogFileInfoBuilder>
{
public:
    explicit LogFileInfoBuilderImpl();

    ErrCode INTERFACE_FUNC build(ILogFileInfo** logFileInfo) override;

    ErrCode INTERFACE_FUNC getLocalPath(IString** localPath) override;
    ErrCode INTERFACE_FUNC setLocalPath(IString* localPath) override;

    ErrCode INTERFACE_FUNC getName(IString** name) override;
    ErrCode INTERFACE_FUNC setName(IString* name) override;

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC setId(IString* id) override;

    ErrCode INTERFACE_FUNC getDescription(IString** description) override;
    ErrCode INTERFACE_FUNC setDescription(IString* description) override;

    ErrCode INTERFACE_FUNC getSize(SizeT* size) override;
    ErrCode INTERFACE_FUNC setSize(SizeT size) override;

    ErrCode INTERFACE_FUNC getEncoding(IString** encoding) override;
    ErrCode INTERFACE_FUNC setEncoding(IString* encoding) override;

    ErrCode INTERFACE_FUNC getLastModified(IString** lastModified) override;
    ErrCode INTERFACE_FUNC setLastModified(IString* lastModified) override;

private:
    StringPtr localPath;
    StringPtr name;
    StringPtr id;
    StringPtr description;
    StringPtr encoding;
    SizeT size;
    StringPtr lastModified;
};

END_NAMESPACE_OPENDAQ

