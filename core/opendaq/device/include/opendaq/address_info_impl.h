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
#include <opendaq/address_info.h>
#include <opendaq/address_info_private.h>
#include <opendaq/address_info_builder_ptr.h>
#include <coreobjects/property_object_impl.h>
#include <opendaq/context_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AddressInfoImpl : public GenericPropertyObjectImpl<IAddressInfo, IAddressInfoPrivate>
{
public:
    using Super = GenericPropertyObjectImpl;

    explicit AddressInfoImpl();
    explicit AddressInfoImpl(const AddressInfoBuilderPtr& builder);
    
    ErrCode INTERFACE_FUNC getAddress(IString** address) override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC getType(IString** type) override;
    ErrCode INTERFACE_FUNC getReachabilityStatus(AddressReachabilityStatus* addressReachability) override;
    ErrCode INTERFACE_FUNC setReachabilityStatusPrivate(AddressReachabilityStatus addressReachability) override;
    
    ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

protected:
    PropertyObjectPtr createCloneBase() override;

private:
    template <typename T>
    typename InterfaceToSmartPtr<T>::SmartPtr getTypedProperty(const StringPtr& name);
};

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(AddressInfoImpl)

END_NAMESPACE_OPENDAQ
