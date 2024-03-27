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
#include <opendaq/io_folder_impl.h>
#include "opcuatms_client/objects/tms_client_folder_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsClientIoFolderImpl : public TmsClientFolderImpl<IoFolderImpl<ITmsClientComponent>>
{
public:
    explicit TmsClientIoFolderImpl(const ContextPtr& ctx,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId,
                                 const TmsClientContextPtr& clientContext,
                                 const opcua::OpcUaNodeId& nodeId);

protected:
    LoggerComponentPtr loggerComponent;
    void findAndCreateChannels(std::map<uint32_t, ComponentPtr>& orderedComponents, std::vector<ComponentPtr>& unorderedComponents);
    void findAndCreateIoFolders(std::map<uint32_t, ComponentPtr>& orderedComponents, std::vector<ComponentPtr>& unorderedComponents);
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
