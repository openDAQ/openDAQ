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
#include <opendaq/address_info_builder_ptr.h>
#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

class AddressInfoBuilderImpl : public ImplementationOf<IAddressInfoBuilder>
{
public:
    explicit AddressInfoBuilderImpl();

    ErrCode INTERFACE_FUNC build(IAddressInfo** address) override;
    ErrCode INTERFACE_FUNC setAddress(IString* address) override;
    ErrCode INTERFACE_FUNC getAddress(IString** address) override;
    ErrCode INTERFACE_FUNC setConnectionString(IString* connectionString) override;
    ErrCode INTERFACE_FUNC getConnectionString(IString** connectionString) override;
    ErrCode INTERFACE_FUNC setType(IString* type) override;
    ErrCode INTERFACE_FUNC getType(IString** type) override;
    ErrCode INTERFACE_FUNC setReachabilityStatus(AddressReachabilityStatus addressReachability) override;
    ErrCode INTERFACE_FUNC getReachabilityStatus(AddressReachabilityStatus* addressReachability) override;

private:
    StringPtr address;
    StringPtr type;
    AddressReachabilityStatus reachability;
    StringPtr connectionString;
};

END_NAMESPACE_OPENDAQ
