//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (CGenerator v0.5.0) on 01.04.2025 17:02:02.
// </auto-generated>
//------------------------------------------------------------------------------

/*
 * Copyright 2022-2025 openDAQ d.o.o.
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

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

    typedef struct AuthenticationProvider AuthenticationProvider;
    typedef struct String String;
    typedef struct User User;
    typedef struct List List;

    EXPORTED extern const IntfID AUTHENTICATION_PROVIDER_INTF_ID;

    ErrCode EXPORTED AuthenticationProvider_authenticate(AuthenticationProvider* self, String* username, String* password, User** userOut);
    ErrCode EXPORTED AuthenticationProvider_isAnonymousAllowed(AuthenticationProvider* self, Bool* allowedOut);
    ErrCode EXPORTED AuthenticationProvider_authenticateAnonymous(AuthenticationProvider* self, User** userOut);
    ErrCode EXPORTED AuthenticationProvider_findUser(AuthenticationProvider* self, String* username, User** userOut);
    ErrCode EXPORTED AuthenticationProvider_createAuthenticationProvider(AuthenticationProvider** obj, Bool allowAnonymous);
    ErrCode EXPORTED AuthenticationProvider_createStaticAuthenticationProvider(AuthenticationProvider** obj, Bool allowAnonymous, List* userList);
    ErrCode EXPORTED AuthenticationProvider_createJsonStringAuthenticationProvider(AuthenticationProvider** obj, String* jsonString);
    ErrCode EXPORTED AuthenticationProvider_createJsonFileAuthenticationProvider(AuthenticationProvider** obj, String* filename);

#ifdef __cplusplus
}
#endif
