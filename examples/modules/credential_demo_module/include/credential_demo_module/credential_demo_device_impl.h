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
#include <opendaq/device_impl.h>
#include <opendaq/mirrored_device_impl.h>
#include <opendaq/credential_request_ptr.h>
#include <opendaq/authentication_method_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <coretypes/dict_ptr.h>

/*
 * Minimal mirrored device implementation with no signals or channels. When connected to via the
 * authenticated path, authenticates via the credential framework using a username/password, a PIN
 * code, a private-key challenge, or - trivially - anonymously, the four showcased auth methods.
 * Anonymous authentication requires no credentials and connects exactly like the plain,
 * non-authenticated path below, which likewise requires no credentials. A mirrored device,
 * so a real / mock streaming connection can be attached to it automatically or manually.
 */

BEGIN_NAMESPACE_CREDENTIAL_DEMO_MODULE

class CredentialDemoDeviceImpl final : public MirroredDevice
{
public:
    explicit CredentialDemoDeviceImpl(const PropertyObjectPtr& config,
                                      const ContextPtr& ctx,
                                      const ComponentPtr& parent,
                                      const DeviceInfoPtr& info,
                                      bool authenticated,
                                      const StringPtr& authenticationMethodId = nullptr,
                                      const DictPtr<IString, IString>& credentials = nullptr);

    static constexpr const char* Prefix = "daq.credential_demo";

    static DeviceInfoPtr CreateDeviceInfo(const DictPtr<IString, IBaseObject>& moduleOptions, const ContextPtr& context);
    static DeviceTypePtr CreateType(const ContextPtr& context);
    static void ValidateConnectionString(const StringPtr& connectionString);

protected:
    StringPtr onGetRemoteId() const override;
    bool isAddedToLocalComponentTree() override;
};

END_NAMESPACE_CREDENTIAL_DEMO_MODULE
