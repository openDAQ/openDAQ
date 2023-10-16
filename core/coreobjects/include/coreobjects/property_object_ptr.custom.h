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

class Proxy
{
    GenericPropertyObjectPtr<InterfaceType>& propObj;
    std::string key;

public:
    Proxy(GenericPropertyObjectPtr<InterfaceType>& propObj, std::string key)
        : propObj(propObj)
        , key(std::move(key)){};

    template <typename U, std::enable_if_t<is_ct_conv<U>::value, int> = 0>
    operator U()  // rvalue use
    {
        return static_cast<U>(propObj.getPropertyValue(key));
    }

    Proxy& operator=(const BaseObjectPtr& obj)  // lvalue use
    {
        propObj.setPropertyValue(key, obj);
        return *this;
    }
};

Proxy operator[](const std::string& propName) const
{
    return Proxy(*this, propName);
}
