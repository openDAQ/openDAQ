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
#include <coreobjects/property_object_impl.h>

/* Will crash if runner and DLL are compiled with different compilers because of different RTTI (dynamic_cast)
 *
 *  - MSVC Runner and GCC DLL will emit "Access Voilation - no RTTI" and continue (failing the test)
 *  - GCC Runner and MSVC DLL will just crash completely
*/

inline bool isOwnedBy(const daq::PropertyObjectPtr& propObj,
                      const daq::PropertyObjectPtr& owner)
{
    using namespace daq;

    auto* impl = static_cast<PropertyObjectImpl*>(propObj.getObject());  // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    auto ownableOwnerRef = impl->getOwner();

    GenericPropertyObjectPtr ownableOwner;
    if (ownableOwnerRef.assigned())
    {
        ownableOwner = ownableOwnerRef.getRef();
    }
    return owner == ownableOwner;
}
