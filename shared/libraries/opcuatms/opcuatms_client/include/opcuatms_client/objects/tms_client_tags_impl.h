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
#include "opendaq/tags_impl.h"
#include "opcuaclient/opcuaclient.h"
#include "opcuatms/opcuatms.h"
#include "opcuatms_client/objects/tms_client_object_impl.h"

BEGIN_NAMESPACE_OPENDAQ_OPCUA_TMS

// TmsClientSignalImpl

class TmsClientTagsImpl final : public TmsClientObjectImpl, public TagsImpl
{
public:
    explicit TmsClientTagsImpl(const ContextPtr& ctx, const TmsClientContextPtr& clientContext, const opcua::OpcUaNodeId& nodeId);


    ErrCode INTERFACE_FUNC getList(IList** value) override;
    ErrCode INTERFACE_FUNC add(IString* name) override;
    ErrCode INTERFACE_FUNC set(IList* tags) override;
    ErrCode INTERFACE_FUNC remove(IString* name) override;
    ErrCode INTERFACE_FUNC contains(IString* name, Bool* value) override;
    ErrCode INTERFACE_FUNC query(IString* query, Bool* value) override;

private:
    void refreshTags();
    LoggerComponentPtr loggerComponent;
};

END_NAMESPACE_OPENDAQ_OPCUA_TMS
