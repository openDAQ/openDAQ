/*
 * Copyright 2022-2026 openDAQ d.o.o.
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
#include <opendaq/address_info_ptr.h>
#include <coretypes/exceptions.h>
#include <algorithm>
#include <utility>
#include <vector>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief The name of an address type, as address infos report it in `getType`.
 * @param type The address type
 * @throws InvalidParameterException when the type is not a known address type.
 */
inline StringPtr getAddressTypeName(AddressType type)
{
    switch (type)
    {
        case AddressType::IPv4:
            return "IPv4";
        case AddressType::IPv6:
            return "IPv6";
    }

    DAQ_THROW_EXCEPTION(InvalidParameterException, "Unknown address type {}", static_cast<Int>(type));
}

/*!
 * @brief Sorts address infos in the order they should be tried when connecting.
 * @param addressInfos The address infos
 * @param primaryAddressType The preferred address type
 * @returns The sorted address infos
 *
 * Reachable addresses come first, then those of unknown reachability, then unreachable ones.
 * Among addresses of equal reachability, those of the primary address type come first.
 * The sort is stable.
 */
inline std::vector<AddressInfoPtr> sortAddressInfos(const ListPtr<IAddressInfo>& addressInfos, AddressType primaryAddressType)
{
    const auto primaryTypeName = getAddressTypeName(primaryAddressType);
    const auto reachabilityRank = [](AddressReachabilityStatus status)
    {
        switch (status)
        {
            case AddressReachabilityStatus::Reachable:
                return 0;
            case AddressReachabilityStatus::Unknown:
                return 1;
            case AddressReachabilityStatus::Unreachable:
                return 2;
        }
        return 2;
    };
    const auto rank = [&reachabilityRank, &primaryTypeName](const AddressInfoPtr& addressInfo)
    {
        return std::make_pair(reachabilityRank(addressInfo.getReachabilityStatus()), addressInfo.getType() != primaryTypeName);
    };

    std::vector<AddressInfoPtr> sorted(addressInfos.begin(), addressInfos.end());
    std::stable_sort(sorted.begin(), sorted.end(), [&rank](const AddressInfoPtr& a, const AddressInfoPtr& b) { return rank(a) < rank(b); });
    return sorted;
}

END_NAMESPACE_OPENDAQ
