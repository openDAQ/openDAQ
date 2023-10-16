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
#include <bbopcua/opcua_common.h>
#include <coretypes/coretypes.h>

BEGIN_NAMESPACE_OPENDAQ_OPCUA

/*#
 * [templated]
 * [interfaceLibrary(IString, CoreTypes)]
 * [interfaceLibrary(IBinaryData, CoreTypes)]
 * [interfaceLibrary(IList, CoreTypes)]
 */
DECLARE_OPENDAQ_OPCUA_INTERFACE(IOpcuaSecurityConfig, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getSecurityMode(Int* value) = 0;
    virtual ErrCode INTERFACE_FUNC setSecurityMode(Int value) = 0;
    virtual ErrCode INTERFACE_FUNC getAppUri(IString** value) = 0;
    virtual ErrCode INTERFACE_FUNC setAppUri(IString* value) = 0;
    virtual ErrCode INTERFACE_FUNC getCertificate(IBinaryData** value) = 0;
    virtual ErrCode INTERFACE_FUNC setCertificate(IBinaryData* value) = 0;
    virtual ErrCode INTERFACE_FUNC getPrivateKey(IBinaryData** value) = 0;
    virtual ErrCode INTERFACE_FUNC setPrivateKey(IBinaryData* value) = 0;
    // [elementType(value, IBinaryData)]
    virtual ErrCode INTERFACE_FUNC getTrustList(IList** value) = 0;
    // [elementType(value, IBinaryData)]
    virtual ErrCode INTERFACE_FUNC setTrustList(IList* value) = 0;
    // [elementType(value, IBinaryData)]
    virtual ErrCode INTERFACE_FUNC getRevocationList(IList** value) = 0;
    // [elementType(value, IBinaryData)]
    virtual ErrCode INTERFACE_FUNC setRevocationList(IList* value) = 0;
    virtual ErrCode INTERFACE_FUNC getTrustAll(Bool* value) = 0;
    virtual ErrCode INTERFACE_FUNC setTrustAll(Bool value) = 0;
};

END_NAMESPACE_OPENDAQ_OPCUA
