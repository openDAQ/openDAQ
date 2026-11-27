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

#include <coretypes/dictobject_factory.h>
#include <opendaq/transport_option_factory.h>
#include <initializer_list>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Builds a transport option in one call, so that tests stay short.
 */
inline TransportOptionPtr TestTransportOption(const StringPtr& id,
                                              Int port,
                                              SecurityMode securityMode = SecurityMode::None,
                                              Bool certificateRequired = False)
{
    return TransportOptionBuilder(id)
        .setPort(port)
        .setSecurityMode(securityMode)
        .setCertificateRequired(certificateRequired)
        .build();
}

/*!
 * @brief Gets transport options keyed by their ids, in the given order.
 */
inline DictPtr<IString, ITransportOption> TransportOptionsById(std::initializer_list<TransportOptionPtr> options)
{
    auto dict = Dict<IString, ITransportOption>();
    for (const auto& option : options)
        dict.set(option.getId(), option);

    return dict;
}

END_NAMESPACE_OPENDAQ
