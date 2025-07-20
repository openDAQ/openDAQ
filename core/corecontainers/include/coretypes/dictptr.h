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
#include <coretypes/objectptr.h>
#include <coretypes/iterable_ptr.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dict_element_type.h>

#include <utility>

BEGIN_NAMESPACE_OPENDAQ

template <typename T,
          typename KeyT,
          typename ValueT,
          typename KeyPtr = typename InterfaceOrTypeToSmartPtr<KeyT>::SmartPtr,
          typename ValuePtr = typename InterfaceOrTypeToSmartPtr<ValueT>::SmartPtr>
class DictObjectPtr;

template <>
struct InterfaceToSmartPtr<IDict>
{
    typedef DictObjectPtr<IDict, IBaseObject, IBaseObject> SmartPtr;
};

/*!
 * @addtogroup types_dict
 * @{
 */


/*!
 * @brief Represents a collection of key/value pairs.
 */
template <typename T,
          typename KeyT,
          typename ValueT,
          typename KeyPtr,
          typename ValuePtr>
class DictObjectPtr : public ObjectPtr<T>
{
public:
    using ObjectPtr<T>::ObjectPtr;
    using DictPtr = DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>;

    using IteratorType = NativeIterator<std::pair<KeyPtr, ValuePtr>>;

    // To satisfy Container trait
    using iterator_category = typename IteratorType::iterator_category;
    using value_type = typename IteratorType::value_type;
    using reference = typename IteratorType::reference;
    using const_reference = typename IteratorType::const_reference;
    using iterator = IteratorType;
    // using const_iterator = const IteratorType;
    using difference_type = typename IteratorType::difference_type;
    using size_type = std::size_t;

    template <bool Const>
    class ProxyBase
    {
    public:
        using DictType = std::conditional_t<Const, const DictPtr&, DictPtr&>;

        ProxyBase(DictType dict, KeyPtr key)
            : key(std::move(key))
            , dict(dict)
        {
        }

        template <typename TValue, std::enable_if_t<std::is_convertible<ValuePtr, TValue>::value, int> = 0>
        operator TValue() const
        {
            return dict.get(key);
        }

        ValueT* addRefAndReturn()
        {
            return dict.get(key).addRefAndReturn();
        }

        friend bool operator==(const ProxyBase& lhs, const ProxyBase& rhs)
        {
            return lhs.key == rhs.key
                   && lhs.dict == rhs.dict;
        }

        friend bool operator!=(const ProxyBase& lhs, const ProxyBase& rhs)
        {
            return !(lhs == rhs);
        }

        template <typename TCompareValue>
        friend bool operator==(const ProxyBase& lhs, const TCompareValue& rhs)
        {
            return lhs.dict.get(lhs.key) == rhs;
        }

        template <typename TCompareValue>
        friend bool operator==(const TCompareValue& lhs, const ProxyBase& rhs)
        {
            return operator==(rhs, lhs);
        }

        template <typename TCompareValue>
        friend bool operator!=(const ProxyBase& lhs, const TCompareValue& rhs)
        {
            return !(lhs == rhs);
        }

        template <typename TCompareValue>
        friend bool operator!=(const TCompareValue& lhs, const ProxyBase& rhs)
        {
            return !(lhs == rhs);
        }

    protected:
        KeyPtr key;
        DictType dict;
    };

    using ConstProxy = ProxyBase<true>;

    class Proxy : public ProxyBase<false>
    {
    public:
        using ProxyBase<false>::ProxyBase;

        Proxy& operator=(const ValuePtr& value)
        {
            this->dict.set(this->key, value);
            return *this;
        }           
    };

    DictObjectPtr(const ObjectPtr<T>& ptr)
        : ObjectPtr<T>(ptr)
    {
    }

    DictObjectPtr(ObjectPtr<T>&& ptr)
        : ObjectPtr<T>(std::forward<decltype(ptr)>(ptr))
    {
    }

    /*!
     * @brief Gets the element with the specified key.
     * @param key The key of the element to get.
     * @return The element with the specified key.
     *
     * Throws an exception if the key does not exist.
     */
    ValuePtr get(const KeyPtr& key) const;


    /*!
     * @brief Gets the element with the specified key.
     * @param key The key of the element to get.
     * @return The element with the specified key.
     *
     * Returns the default value if the key does not exist.
     */
    ValuePtr getOrDefault(const KeyPtr& key, const ValuePtr& defaultValue = nullptr) const;

    /*!
     * @brief Gets the element with the specified key.
     * @param key The key of the element to get.
     * @param[out] value The element with the specified key.
     * @return True if they exists, False otherwise.
     */
    bool tryGet(const KeyPtr& key, ValuePtr& value) const;

    /*!
     * @brief Sets the element with the specified key.
     * @param key The key of the element to set.
     * @param value The element with the specified key.
     */
    void set(const KeyPtr& key, const ValuePtr& value);

    /*!
     * @brief Removes the element with the specified key.
     * @param key The key of the element to remove.
     * @return The element with the specified key.
     *
     * Throws an exception if the key does not exist.
     */
    ValuePtr remove(const KeyPtr& key);

    /*!
     * @brief Removes the element with the specified key.
     * @param key The key of the element to remove.
     * @return True if the key exists, False otherwise.
     */
    bool tryRemove(const KeyPtr& key);

    /*!
     * @brief Deletes the element with the specified key.
     * @param key The key of the element to delete.
     */
    void deleteItem(const KeyPtr& key);

    /*!
     * @brief Gets the number of elements contained in the dictionary.
     * @return The number of elements contained in the dictionary.
     */
    SizeT getCount() const;

    /*!
     * @brief Removes all elements from the list.
     */
    void clear();

    /*!
     * @brief Checks if the element with the specified key exists in the dictionary.
     * @param key The key of the element to check.
     * @return True if the element exists, False otherwise.
     */
    bool hasKey(const KeyPtr& key) const;

    /*!
     * @brief Gets the list of all keys in the dictionary.
     * @return The list of the keys.
     *
     * The order of the keys is not defined.
     */
    ListPtr<KeyT, KeyPtr> getKeyList() const;

    /*!
     * @brief Gets the list of all elements in the dictionary.
     * @return The list of the elements.
     *
     * The order of the elements is not defined.
     */
    ListPtr<ValueT, ValuePtr> getValueList() const;

    /*!
     * @brief Gets the iterable interface of the keys.
     * @return The iterable interface of the keys.
     *
     * The Iterable object enables iteration through the keys.
     */
    IterablePtr<KeyT, KeyPtr> getKeys() const;

    /*!
     * @brief Gets the iterable interface of the elements.
     * @return The iterable interface of the elements.
     *
     * The Iterable object enables iteration through the elements.
     */
    IterablePtr<ValueT, ValuePtr> getValues() const;

    /*!
     * @brief Returns the interface id of the expected key type.
     * @return The interface id of the expected key type otherwise returns the id of `IUnknown`.
     */
    [[nodiscard]] IntfID getKeyInterfaceId() const;

    /*!
     * @brief Returns the interface id of the expected value type.
     * @return The interface id of the expected value type otherwise returns the id of `IUnknown`.
     */
    [[nodiscard]] IntfID getValueInterfaceId() const;

    Proxy operator [](const KeyPtr& key);
    ConstProxy operator [](const KeyPtr& key) const;

    template <typename U = KeyT, typename V = typename std::enable_if<std::is_same<U, IInteger>::value, int>::type>
    Proxy operator [](V key);

    template <typename U = KeyT, typename V = typename std::enable_if<std::is_same<U, IInteger>::value, int>::type>
    ConstProxy operator [](V key) const;

    /*!
     * @brief Creates start iterator for the dictionary.
     */
    IteratorType begin() const;
    
    /*!
     * @brief Creates end iterator for the dictionary.
     */
    IteratorType end() const;
};

/*!@}*/

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
ValuePtr DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::get(const KeyPtr& key) const
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    IBaseObject* obj;
    checkErrorInfo(this->object->get(key, &obj));

    return ValuePtr(std::move(obj));
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
ValuePtr DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::getOrDefault(const KeyPtr& key, const ValuePtr& defaultValue) const
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    IBaseObject* obj;
    const ErrCode errCode = this->object->get(key, &obj);

    if (errCode == OPENDAQ_ERR_NOTFOUND)
    {
        if (daqClearErrorInfo() != errCode)
        {
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Failed to clear error info");
        }
        return defaultValue;
    }

    checkErrorInfo(errCode);

    return ValuePtr(std::move(obj));
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
bool DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::tryGet(const KeyPtr& key, ValuePtr& value) const
{
    if (!this->object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    IBaseObject* obj{};
    ErrCode errCode = this->object->get(key, &obj);
    value = ValuePtr(std::move(obj));

    if (errCode == OPENDAQ_ERR_NOTFOUND)
    {
        if (daqClearErrorInfo() != errCode)
        {
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Failed to clear error info");
        }
        return false;
    }

    checkErrorInfo(errCode);

    return true;
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
void DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::set(const KeyPtr& key, const ValuePtr& value)
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    checkErrorInfo(this->object->set(key, value));
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
ValuePtr DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::remove(const KeyPtr& key)
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    IBaseObject* obj;
    checkErrorInfo(this->object->remove(key, &obj));

    return ValuePtr(std::move(obj));
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
bool DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::tryRemove(const KeyPtr& key)
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    ObjectPtr<IBaseObject> obj;
    const ErrCode errCode = this->object->remove(key, &obj);

    if (errCode == OPENDAQ_ERR_NOTFOUND)
    {
        if (daqClearErrorInfo() != errCode)
        {
            DAQ_THROW_EXCEPTION(InvalidParameterException, "Failed to clear error info");
        }
        return false;
    }

    checkErrorInfo(errCode);

    return true;
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
void DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::deleteItem(const KeyPtr& key)
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    checkErrorInfo(this->object->deleteItem(key));
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
SizeT DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::getCount() const
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    SizeT size{};
    checkErrorInfo(this->object->getCount(&size));

    return size;
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
void DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::clear()
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    checkErrorInfo(this->object->clear());
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
bool DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::hasKey(const KeyPtr& key) const
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    Bool keyExists = False;
    checkErrorInfo(this->object->hasKey(key, &keyExists));

    return keyExists;
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
ListPtr<KeyT, KeyPtr> DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::getKeyList() const
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    IList* list;
    checkErrorInfo(this->object->getKeyList(&list));

    return ListPtr<KeyT, KeyPtr>(std::move(list));
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
ListPtr<ValueT, ValuePtr> DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::getValueList() const
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    IList* list;
    checkErrorInfo(this->object->getValueList(&list));

    return ListPtr<ValueT, ValuePtr>(std::move(list));
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
IterablePtr<KeyT, KeyPtr> DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::getKeys() const
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    IIterable* iterable;
    checkErrorInfo(this->object->getKeys(&iterable));

    return IterablePtr<KeyT, KeyPtr>::Adopt(iterable);
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
IterablePtr<ValueT, ValuePtr> DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::getValues() const
{
    if (!ObjectPtr<T>::object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    IIterable* iterable;
    checkErrorInfo(this->object->getValues(&iterable));

    return IterablePtr<ValueT, ValuePtr>::Adopt(iterable);
}

template <typename T, typename KeyT, typename ValueT, typename KeyPtr, typename ValuePtr>
IntfID DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::getKeyInterfaceId() const
{
    if (!this->object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    auto elementType = this->template asPtrOrNull<IDictElementType>(true);

    IntfID id{};
    if (elementType.assigned())
        checkErrorInfo(elementType->getKeyInterfaceId(&id));

    return id;
}

template <typename T, typename KeyT, typename ValueT, typename KeyPtr, typename ValuePtr>
IntfID DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::getValueInterfaceId() const
{
    if (!this->object)
        DAQ_THROW_EXCEPTION(InvalidParameterException);

    auto elementType = this->template asPtrOrNull<IDictElementType>(true);

    IntfID id{};
    if (elementType.assigned())
        checkErrorInfo(elementType->getValueInterfaceId(&id));

    return id;
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
typename DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::ConstProxy
DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::operator[](const KeyPtr& key) const
{
    return ConstProxy(*this, key);
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
template <typename U, typename V>
typename DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::ConstProxy
DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::operator [](V key) const
{
    return ConstProxy(*this, key);
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
typename DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::Proxy
DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::operator[](const KeyPtr& key)
{
    return Proxy(*this, key);
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
template <typename U, typename V>
typename DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::Proxy
DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::operator [](V key)
{
    return Proxy(*this, key);
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
NativeIterator<std::pair<KeyPtr, ValuePtr>> DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::begin() const
{
    return this->createStartIteratorInterface();
}

template <class T, class KeyT, class ValueT, class KeyPtr, class ValuePtr>
NativeIterator<std::pair<KeyPtr, ValuePtr>> DictObjectPtr<T, KeyT, ValueT, KeyPtr, ValuePtr>::end() const
{
    return this->createEndIteratorInterface();
}

END_NAMESPACE_OPENDAQ
