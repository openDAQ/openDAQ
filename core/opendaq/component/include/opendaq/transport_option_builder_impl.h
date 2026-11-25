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
#include <opendaq/transport_option_builder.h>
#include <coretypes/impl.h>
#include <coretypes/string_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class TransportOptionBuilderImpl final : public ImplementationOf<ITransportOptionBuilder>
{
public:
    explicit TransportOptionBuilderImpl(IString* id);
    explicit TransportOptionBuilderImpl(ITransportOption* transportOptionToCopy);

    ErrCode INTERFACE_FUNC build(ITransportOption** transportOption) override;

    ErrCode INTERFACE_FUNC getId(IString** id) override;
    ErrCode INTERFACE_FUNC setPort(Int port) override;
    ErrCode INTERFACE_FUNC getPort(Int* port) override;
    ErrCode INTERFACE_FUNC setSecurityMode(SecurityMode securityMode) override;
    ErrCode INTERFACE_FUNC getSecurityMode(SecurityMode* securityMode) override;
    ErrCode INTERFACE_FUNC setCertificateRequired(Bool required) override;
    ErrCode INTERFACE_FUNC getCertificateRequired(Bool* required) override;

private:
    StringPtr id;
    Int port;
    SecurityMode securityMode;
    Bool certificateRequired;
};

END_NAMESPACE_OPENDAQ
