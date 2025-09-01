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

#include <opendaq/module.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @ingroup opendaq_instance
 * @addtogroup opendaq_module_authenticator ModuleAuthentication
 * @{
 */

/*#
 * [interfaceLibrary(Bool, "coretypes")]
 * [interfaceLibrary(IString, "coretypes")]
 */

/*!
 * @brief Module authenticator interface. 
 */
DECLARE_OPENDAQ_INTERFACE(IModuleAuthenticator, IBaseObject)
{
    /*!
     * @brief Verify the module binary with a certificate file.
     * @param binaryPath Absolute path to the binary.
     * @param[out] binaryValid The binarys checksum matches and it's considered valid.
     * @param[out] vendorKey Key for the vendor to identify if their certificate was used to authenticate the module.
     */
    virtual ErrCode INTERFACE_FUNC authenticateModuleBinary(Bool* binaryValid, IString** vendorKey, IString* binaryPath) = 0;
};
/*!@}*/

END_NAMESPACE_OPENDAQ
