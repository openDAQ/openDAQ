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
#include <opendaq/folder_ptr.h>
#include <opcuatms_server/objects/tms_server_channel.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

class TmsServerFolder;
using TmsServerFolderPtr = std::shared_ptr<TmsServerFolder>;

class TmsServerFolder : public TmsServerComponent<FolderPtr>
{
public:
    using Super = TmsServerComponent<FolderPtr>;

    TmsServerFolder(const FolderPtr& object, const opcua::OpcUaServerPtr& server, const ContextPtr& context, const TmsServerContextPtr& tmsContext);
    
    void createNonhierarchicalReferences() override;
    void addChildNodes() override;

protected:
    opcua::OpcUaNodeId getTmsTypeId() override;

    std::list<TmsServerChannelPtr> channels;
    std::list<TmsServerFolderPtr> folders;
    std::list<TmsServerComponentPtr> components;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
