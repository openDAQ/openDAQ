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
#include "opcuatms_client/objects/tms_client_component_impl.h"
#include "opendaq/folder_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

template <class Impl = FolderImpl<IFolderConfig>>
class TmsClientFolderImpl : public TmsClientComponentBaseImpl<Impl>
{
public:
    explicit TmsClientFolderImpl(const ContextPtr& ctx,
                                 const ComponentPtr& parent,
                                 const StringPtr& localId,
                                 const TmsClientContextPtr& clientContext,
                                 const opcua::OpcUaNodeId& nodeId,
                                 bool customFolderType);
private:
    void findAndCreateFolders();
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
