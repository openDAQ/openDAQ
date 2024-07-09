/*
 * Copyright 2022-2024 openDAQ d. o. o.
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

#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;

    #if defined(BOOST_DLL_USE_STD_FS)
        namespace std { namespace filesystem = std::experimental::filesystem; }
    #endif

    #if !defined(_WIN32)
        namespace std::experimental::filesystem
        {
            inline path relative(const path& p)
            {
                #pragma message "Experimental std::filesystem does not have `relative()`."
                return p;
            }
        }
    #endif
#else
    #error "Must have <filesystem> or <experimental/filesystem> library"
#endif