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
#include <opendaq/authentication_config.h>
#include <coreobjects/property_object_impl.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/serialized_object.h>
#include <opendaq/authentication_method_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

class AuthenticationConfigImpl : public GenericPropertyObjectImpl<IAuthenticationConfig>
{
public:
    using Super = GenericPropertyObjectImpl<IAuthenticationConfig>;

    // The first entry of `authenticationMethods` (in dict iteration order) starts out selected.
    explicit AuthenticationConfigImpl(const DictPtr<IString, IAuthenticationMethod>& authenticationMethods);

    ErrCode INTERFACE_FUNC getSelectedAuthenticationMethod(IAuthenticationMethod** authenticationMethod) override;
    ErrCode INTERFACE_FUNC setAuthenticationMethod(IString* authenticationMethodId) override;
    ErrCode INTERFACE_FUNC getAuthenticationMethods(IDict** authenticationMethods) override;
    ErrCode INTERFACE_FUNC setSuppliedCredential(IDict* credential) override;
    ErrCode INTERFACE_FUNC getSuppliedCredential(IDict** credential) override;

    // An `IAuthenticationConfig` needs to be nestable as an ordinary Object-type property value, but
    // `GenericPropertyObjectImpl::checkContainerType` only allows a nested value whose own
    // `IInspectable::getInterfaceIds()[0]` is exactly `IPropertyObject::Id` - any other primary interface is
    // rejected with `InvalidTypeException`. Overriding it to report `IPropertyObject::Id` first (the real
    // interface list, `IAuthenticationConfig` included, still follows right after - `QueryInterface`/casting
    // to it is entirely unaffected, only the reported order changes).
    ErrCode INTERFACE_FUNC getInterfaceIds(SizeT* idCount, IntfID** ids) override;

    // Intercepted to validate/auto-clear "SuppliedCredential" against whichever authentication method is currently selected
    // whenever "AuthenticationMethod" changes. `setProtectedPropertyValue` gets the same treatment (see
    // `onPropertyValueChanged`) since it's the path generic deserialization (`DeserializePropertyValues`) writes
    // through, bypassing `setPropertyValue`/`setPropertySelectionValue` entirely.
    ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setPropertySelectionValue(IString* propertyName, IBaseObject* value) override;
    ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;

    // Serialization is entirely custom (replaces the inherited generic `IPropertyObject` one, like
    // `DeviceTypeImpl`'s own pattern) - writes only `supportedAuthenticationMethods` (the real
    // `IAuthenticationMethod` objects this config was built from - format/parameters included, not just
    // ids) and the currently selected id. Never writes `"SuppliedCredential"`. `Deserialize` rebuilds the
    // config directly from those two pieces via the real constructor, then restores the saved selection -
    // fully self-contained, no module/type registry re-consultation needed.
    ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;
    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

private:
    static constexpr const char* AuthenticationMethodPropertyName = "AuthenticationMethod";
    static constexpr const char* SuppliedCredentialPropertyName = "SuppliedCredential";
    static constexpr const char* SupportedAuthenticationMethodsSerializedKey = "SupportedAuthenticationMethods";

    void initProperties(const DictPtr<IString, IAuthenticationMethod>& authenticationMethods);

    // Shared tail of `setPropertyValue`/`setPropertySelectionValue`/`setProtectedPropertyValue`, run after the
    // write itself already succeeded: resets "SuppliedCredential" on an "AuthenticationMethod"
    // write.
    ErrCode onPropertyValueChanged(const StringPtr& name);
    AuthenticationMethodPtr onGetSelectedAuthenticationMethod() const;

    // The real `IAuthenticationMethod` objects this config was built from/deserialized with, keyed by their
    // own id - "AuthenticationMethod"'s candidates are just these same keys, as plain strings.
    DictPtr<IString, IAuthenticationMethod> supportedAuthenticationMethods;
};

END_NAMESPACE_OPENDAQ
