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
#include <coretypes/impl.h>
#include <coretypes/intfs.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/listobject.h>
#include <coreobjects/property_object.h>
#include <opendaq/context_ptr.h>
#include <opendaq/component_impl.h>
#include <opendaq/sync_component.h>
#include <opendaq/sync_component_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class GenericSyncComponentImpl : public GenericPropertyObjectImpl<ISyncComponent>
{
public:
    using Super = GenericPropertyObjectImpl<ISyncComponent>;

    explicit GenericSyncComponentImpl();

    //ISyncComponent
    ErrCode INTERFACE_FUNC test() override;
    ErrCode INTERFACE_FUNC getSyncLocked(Bool* syncLocked) override;
    ErrCode INTERFACE_FUNC getSelectedSource(IString** selectedSource) override;
    ErrCode INTERFACE_FUNC getInterfaces(IList** interfaces) override;
//    ErrCode INTERFACE_FUNC addInterface(IPropertyObject* interface) override;
    ErrCode INTERFACE_FUNC removeInterface(IString* interfaceName) override;

protected:
    ListPtr<IPropertyObject> interfaces;
    Bool syncLocked;
};

GenericSyncComponentImpl::GenericSyncComponentImpl()
    : Super()
{
}

ErrCode GenericSyncComponentImpl::test()
{
    return OPENDAQ_SUCCESS;
}

ErrCode GenericSyncComponentImpl::getSyncLocked(Bool* syncLocked)
{
    return OPENDAQ_SUCCESS;
}

ErrCode GenericSyncComponentImpl::getSelectedSource(IString** selectedSource)
{
    return OPENDAQ_SUCCESS;
}

ErrCode GenericSyncComponentImpl::getInterfaces(IList** interfaces)
{
    return OPENDAQ_SUCCESS;
}

//
//ErrCode GenericSyncComponentImpl::addInterface(IPropertyObject* interface)
//{
//    return OPENDAQ_SUCCESS;
//}
//

ErrCode GenericSyncComponentImpl::removeInterface(IString* interfaceName)
{
    return OPENDAQ_SUCCESS;
}

//OPENDAQ_REGISTER_DESERIALIZE_FACTORY(SyncComponentImpl)

END_NAMESPACE_OPENDAQ
