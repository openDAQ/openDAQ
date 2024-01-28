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
#include <opendaq/config_provider.h>
#include <rapidjson/document.h>
#include <coretypes/baseobject_factory.h>

#ifdef _WIN32
    #include <windows.h>
#endif

BEGIN_NAMESPACE_OPENDAQ

class EnvConfigProviderImpl final : public ImplementationOf<IConfigProvider>
{
public:
    explicit EnvConfigProviderImpl();

    ErrCode INTERFACE_FUNC populateOptions(IDict* options) override 
    { 
    }

    // ErrCode INTERFACE_FUNC populateModuleOptions(IDict* options)
    // {
    //     extern char **environ;

    //     std::string prefix = "OPENDAQ_CONFIG_MODULES_";
    //     for (char **env = environ; *env != 0; env++) {
    //         std::string envVar = *env;
    //         if (envVar.find(prefix) == 0) {
    //             // std::cout << envVar << std::endl;
    //         }
    //     }
    // }

    // ErrCode INTERFACE_FUNC populateModuleOptions(IDict* options)
    // {
    //     // Get a pointer to the environment block.
    //     LPWCH lpvEnv = GetEnvironmentStrings();

    //     // If the returned pointer is NULL, exit.
    //     if (lpvEnv == NULL) {
    //         std::cerr << "Failed to get environment strings." << std::endl;
    //         return 1;
    //     }

    //     // Variable to store individual environment strings.
    //     std::wstring envVar;

    //     // Process each string in the block.
    //     for (LPWCH p = lpvEnv; *p != L'\0'; p++) {
    //         // If we hit a null character, we've reached the end of a string.
    //         if (*p == L'\0') {
    //             // Check if the environment variable starts with the desired prefix
    //             if (envVar.find(L"OPENDAQ_CONFIG_") == 0) {
    //                 // Convert wide string to narrow string (if needed) and print it
    //                 std::wcout << envVar << std::endl;
    //             }
    //             envVar.clear();
    //         } else {
    //             // Append the character to our current environment variable string.
    //             envVar += *p;
    //         }
    //     }

    //     // Free the memory allocated for the environment strings.
    //     FreeEnvironmentStrings(lpvEnv);

    //     return 0;
    // }


private:

};

END_NAMESPACE_OPENDAQ
