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
#include <credential_demo_module/common.h>
#include <credential_demo_module/credential_demo_authenticator.h>
#include <opendaq/streaming_impl.h>
#include <opendaq/streaming_type_ptr.h>
#include <opendaq/credential_request_ptr.h>
#include <coretypes/dict_ptr.h>

/*
 * A dummy streaming connection - it never transports any data and its callbacks are no-ops. Authenticates
 * the same way `CredentialDemoDeviceImpl` does: the constructor verifies the supplied credentials via
 * `authentication::Authenticate` before finishing construction.
 */

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

class CredentialDemoStreamingImpl final : public Streaming
{
public:
    explicit CredentialDemoStreamingImpl(const StringPtr& connectionString,
                                         const ContextPtr& ctx,
                                         const StringPtr& authenticationMethodId,
                                         const DictPtr<IString, IString>& credentials);

    static constexpr const char* Prefix = "daq.credential_demo_streaming";

    static StreamingTypePtr CreateType(const ContextPtr& context);

protected:
    void onSetActive(bool active) override;
    void onAddSignal(const MirroredSignalConfigPtr& signal) override;
    void onRemoveSignal(const MirroredSignalConfigPtr& signal) override;
    void onSubscribeSignal(const StringPtr& signalStreamingId) override;
    void onUnsubscribeSignal(const StringPtr& signalStreamingId) override;
};

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
