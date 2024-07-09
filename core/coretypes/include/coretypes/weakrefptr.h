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
#include <coretypes/objectptr.h>

BEGIN_NAMESPACE_OPENDAQ

/*!
 * @addtogroup types_weak_ref
 * @{
 */

/*!
 * @brief Represents a weak reference to the object.
 *
 * Obtain a weak reference to the object when you want to store it to a local variable
 * and don't want to increment the reference count. Weak reference will not prevent object
 * destruction. Before the object is used, you must obtain a true reference by calling
 * `getRef` or `getRefAs`.
 *
 * Example
 * @code
 * ObjectPtr<IObjectWithWeakReafSupport> obj(CreateObjectWithWeakRefSupport());
 *
 * WeakRefPtr<IObjectWithWeakReafSupport> weakRef = obj;
 *
 * // this will throw exception if obj is already destroyed
 * ObjectPtr<IObjectWithWeakReafSupport> newRef = weakRef.getRef();
 * @endcode
 */
template <typename TInterface, typename TSmartPtr = typename InterfaceToSmartPtr<TInterface>::SmartPtr>
class WeakRefPtr : public ObjectPtr<IWeakRef>
{
public:
    WeakRefPtr() = default;
    WeakRefPtr(std::nullptr_t);
    WeakRefPtr(const ObjectPtr<ISupportsWeakRef>& obj);
    WeakRefPtr(ISupportsWeakRef* intf);
    WeakRefPtr(IBaseObject* intf);
    WeakRefPtr(IWeakRef* intf);

    template <typename V>
    WeakRefPtr(const ObjectPtr<V>& obj);

    /*!
     * @brief Gets a true reference to the object.
     * @return A true reference to the object or @c null if the object has already expired
     */
    TSmartPtr getRef() const;

    friend bool operator==(const WeakRefPtr<TInterface, TSmartPtr>& lhs, const WeakRefPtr<TInterface, TSmartPtr>& rhs)
    {
        return lhs.getRef() == rhs.getRef();
    }
};

/*!@}*/

template <typename TInterface, typename TSmartPtr>
WeakRefPtr<TInterface, TSmartPtr>::WeakRefPtr(std::nullptr_t)
{
}

template <typename TInterface, typename TSmartPtr>
WeakRefPtr<TInterface, TSmartPtr>::WeakRefPtr(const ObjectPtr<ISupportsWeakRef>& obj)
{
    if (obj == nullptr)
        return;

    IWeakRef* weakRefToObject;
    checkErrorInfo(obj->getWeakRef(&weakRefToObject));

    object = weakRefToObject;
    borrowed = false;
}

template <typename TInterface, typename TSmartPtr>
WeakRefPtr<TInterface, TSmartPtr>::WeakRefPtr(ISupportsWeakRef* intf)
{
    if (intf == nullptr)
        return;

    IWeakRef* weakRefToObject;
    checkErrorInfo(intf->getWeakRef(&weakRefToObject));

    object = weakRefToObject;
    borrowed = false;
}

template <typename TInterface, typename TSmartPtr>
WeakRefPtr<TInterface, TSmartPtr>::WeakRefPtr(IBaseObject* intf)
{
    if (intf == nullptr)
        return;

    ISupportsWeakRef* objectWithWeakRefSupport;
    checkErrorInfo(intf->borrowInterface(ISupportsWeakRef::Id, reinterpret_cast<void**>(&objectWithWeakRefSupport)));

    IWeakRef* weakRefToObject;
    checkErrorInfo(objectWithWeakRefSupport->getWeakRef(&weakRefToObject));

    object = weakRefToObject;
    borrowed = false;
}

template <typename TInterface, typename TSmartPtr>
WeakRefPtr<TInterface, TSmartPtr>::WeakRefPtr(IWeakRef* intf)
{
    object = intf;
    if (object != nullptr)
        object->addRef();
}

template <typename TInterface, typename TSmartPtr>
template <typename V>
WeakRefPtr<TInterface, TSmartPtr>::WeakRefPtr(const ObjectPtr<V>& obj)
{
    if (!obj.assigned())
        return;

    ISupportsWeakRef* objectWithWeakRefSupport;
    checkErrorInfo(obj->borrowInterface(ISupportsWeakRef::Id, reinterpret_cast<void**>(&objectWithWeakRefSupport)));

    IWeakRef* weakRefToObject;
    checkErrorInfo(objectWithWeakRefSupport->getWeakRef(&weakRefToObject));

    object = weakRefToObject;
    borrowed = false;
}

template <typename TInterface, typename TSmartPtr>
TSmartPtr WeakRefPtr<TInterface, TSmartPtr>::getRef() const
{
    if (object == nullptr)
        throw InvalidParameterException();

    TInterface* intf = nullptr;
    ErrCode errCode = object->getRefAs(TInterface::Id, reinterpret_cast<void**>(&intf));

    if (errCode == OPENDAQ_ERR_NOTASSIGNED)
    {
        daqClearErrorInfo();
    }
    else if (errCode != OPENDAQ_ERR_NOTASSIGNED)
    {
        checkErrorInfo(errCode);
    }
    return TSmartPtr(std::move(intf));
}

END_NAMESPACE_OPENDAQ
