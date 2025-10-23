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
#include <level4/property_object_view.h>
#include <coreobjects/property_object_impl.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @brief Base template class for creating views of PropertyObjects.
 *
 * A view is a separate PropertyObject that contains standardized properties
 * (e.g., Level 4 specification). Views will be extended with specific property
 * sets (e.g., ChannelView, DeviceView).
 *
 * The view maintains a reference to the original owner object.
 *
 * @tparam Impl The implementation interface this view should inherit from
 *              (typically PropertyObjectImpl)
 */
template <typename Impl>
class GenericPropertyObjectView : public Impl
{
public:
    template <class... Args>
    explicit GenericPropertyObjectView(IPropertyObject* viewOwner,
                                      const Args&... args);

    // IPropertyObjectView
    ErrCode INTERFACE_FUNC getViewOwner(IPropertyObject** propObject) override;

protected:
    WeakRefPtr<IPropertyObject> viewOwner;
};

END_NAMESPACE_OPENDAQ