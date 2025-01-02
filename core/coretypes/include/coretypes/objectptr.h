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
#include <coretypes/baseobject.h>
#include <coretypes/boolean.h>
#include <coretypes/comparable.h>
#include <coretypes/convertible.h>
#include <coretypes/coretype.h>
#include <coretypes/coretype_traits.h>
#include <coretypes/ctutils.h>
#include <coretypes/errorinfo.h>
#include <coretypes/exceptions.h>
#include <coretypes/float.h>
#include <coretypes/freezable.h>
#include <coretypes/function.h>
#include <coretypes/impl.h>
#include <coretypes/integer.h>
#include <coretypes/iterable.h>
#include <coretypes/iterator.h>
#include <coretypes/listobject.h>
#include <coretypes/mem.h>
#include <coretypes/procedure.h>
#include <coretypes/ratio.h>
#include <coretypes/serializable.h>
#include <coretypes/serializer.h>
#include <coretypes/stringobject.h>
#include <coretypes/weakref.h>
#include <cassert>
#include <cstddef>

BEGIN_NAMESPACE_OPENDAQ

template <typename T>
class ObjectPtr;

template <typename Intf>
struct InterfaceToSmartPtr
{
    using SmartPtr = ObjectPtr<Intf>;
};

template <typename T, typename Enable = void>
struct InterfaceOrTypeToSmartPtr;

template <typename T>
struct InterfaceOrTypeToSmartPtr<T, typename std::enable_if_t<std::is_base_of_v<IBaseObject, T>>> 
    : public InterfaceToSmartPtr<T>
{
};

template <typename T>
struct InterfaceOrTypeToSmartPtr<T, typename std::enable_if_t<IsDerivedFromTemplate<T, ObjectPtr>::Value, void>>
{
    using SmartPtr = T;
};

template <typename T>
struct InterfaceOrTypeToSmartPtr<T, typename std::enable_if_t<Is_ct_conv<T>::value>>
    : public InterfaceToSmartPtr<typename CoreTypeHelper<T>::Interface>
{
};

/*!
 * @addtogroup types_base_concepts
 * @{
 */

/*!
 * @brief Smart pointer wrapper for IBaseObject interface and its descendants.
 *
 * Smart pointer is an object that wraps IBaseObject and its derived interfaces. It provides
 * automatic reference counting for wrapped interface, data type conversion, comparison and querying
 * for other supported interfaces.
 */

template <typename T>
class ObjectPtr
{
public:
    using DeclaredInterface = T;

    /*!
     * @brief Creates a smart pointer with no wrapped interface.
     */
    ObjectPtr();

    /*!
     * @brief Creates a smart pointer with no wrapped interface.
     */
    ObjectPtr(std::nullptr_t);

    /*!
     * @brief Creates a smart pointer from another smart pointer.
     * @param objPtr The source smart pointer with the same type of wrapped interface.
     */
    ObjectPtr(const ObjectPtr<T>& objPtr);

    /*!
     * @brief Creates a smart pointer from another smart pointer with move semantics.
     * @param objPtr The source smart pointer with the same type of wrapped interface.
     *
     * Source smart pointer is moved into the smart pointer. The Reference count is not incremented.
     */
    ObjectPtr(ObjectPtr<T>&& objPtr) noexcept;

    /*!
     * @brief Creates a smart pointer from another smart pointer of a different interface type.
     * @param objPtr The source smart pointer with a different type of wrapped interface.
     */
    template <class U>
    ObjectPtr(const ObjectPtr<U>& objPtr);

    /*!
     * @brief Creates a smart pointer from another smart pointer of a different interface type with move semantics.
     * @param objPtr The source smart pointer with a different type of wrapped interface.
     *
     * Source smart pointer is moved into the smart pointer. The Reference count is not incremented.
     */
    template <class U>
    ObjectPtr(ObjectPtr<U>&& objPtr);

    /*!
     * @brief Creates a smart pointer from an interface pointer.
     * @param obj The interface pointer.
     *
     * The reference count is incremented.
     */
    ObjectPtr(T*& obj);

    /*!
     * @brief Creates a smart pointer from an interface pointer with move semantics.
     * @param obj The interface pointer.
     *
     * The reference count is not incremented.
     */
    ObjectPtr(T*&& obj);

    /*!
     * @brief Creates a smart pointer from a weak reference interface pointer
     * @param obj The weak reference interface pointer.
     *
     * If the object that the weak reference interface pointer represents does not exist
     * anymore, the smart pointer sets the wrapped interface to nullptr.
     */
    ObjectPtr(IWeakRef* obj);

    template <class U, std::enable_if_t<std::is_base_of_v<IBaseObject, U>, int> = 0>
    ObjectPtr(U*& obj);

    template <class U, std::enable_if_t<std::is_base_of_v<IBaseObject, U>, int> = 0>
    ObjectPtr(U*&& obj);

    template <typename TComPtr, std::enable_if_t<HasIID<TComPtr>::value, int> = 0>
    ObjectPtr(const TComPtr& unknown);

    /*!
     * @brief Creates a string object smart pointer from UTF16 null terminated sequence.
     * @param value The UTF16 null terminated sequence.
     */
    ObjectPtr(const wchar_t* value);

    /*!
     * @brief Creates a string object smart pointer from UTF8 null terminated sequence.
     * @param value The UTF8 null terminated sequence.
     */
    ObjectPtr(ConstCharPtr value);

    /*!
     * @brief Creates a smart pointer wrapper from value type.
     * @param value The value.
     *
     * Value can be any type that has interface wrapper, such as Int, Bool, Float,
     * uint16_t, float, double, etc...
     */
    template <typename U, std::enable_if_t<is_ct_conv<U>::value && !std::is_enum_v<U>, int> = 0>
    ObjectPtr(const U& value);

    /*!
     * @brief Destructor.
     *
     * If the interface is not borrowed it releases the reference.
     */
    virtual ~ObjectPtr();

    /*!
     * @brief Borrows an interface.
     * @param obj The source interface.
     *
     * The smart pointer does not increment the reference count when it borrows the interface. It also does not decrement
     * the reference count when the smart pointer is destroyed. The client is responsible that it keeps the object alive
     * during the lifetime of the smart pointer.
     */
    template <class U = T>
    static typename InterfaceToSmartPtr<U>::SmartPtr Borrow(T*& obj);

    static ObjectPtr<T> Borrow(T*&& obj) = delete;

    static ObjectPtr<T> Borrow(const ObjectPtr<T>& ptr);

    /*!
     * @brief Adopts an interface.
     * @param obj The source interface.
     *
     * The smart pointer does not increment the reference count when it adopts the interface. However, the smart pointer
     * takes ownership of the interface and releases the interface when it is destroyed. After the interface is adopted,
     * the client should not call any methods directly on the interface.
     */
    template <typename U = T>
    static typename InterfaceToSmartPtr<U>::SmartPtr Adopt(T* obj);

    /*!
     * @brief Borrows an interface from the interface of another type.
     * @param obj The source interface.
     *
     * The smart pointer does not increment the reference count when it borrows the interface. It also does not decrement
     * the reference count when the smart pointer is destroyed. The client is responsible that it keeps the object alive
     * during the lifetime of the smart pointer.
     */
    template <class U, class V = T, std::enable_if_t<std::is_base_of_v<IBaseObject, U>, int> = 0>
    static typename InterfaceToSmartPtr<V>::SmartPtr Borrow(U*& obj);

    template <class U, class V = T, std::enable_if_t<std::is_base_of_v<IBaseObject, U>, int> = 0>
    static typename InterfaceToSmartPtr<V>::SmartPtr Borrow(U*&& obj) = delete;

    /*!
     * @brief Compares object to another object for equality.
     * @param other Interface to another object for comparison.
     * @returns The value @c true indicating if the @c other is equivalent to this one otherwise @c false.
     *
     * For reference objects, it compares the address of the object. For value objects, such as numbers and
     * strings, it compares values.
     *
     * `ImplementationOf` which provides default implementation of `IBaseObject` uses
     * pointer address to compare for equality.
     */
    bool equals(ObjectPtr<IBaseObject> other) const;

    /*!
     * @brief Value based comparison of two objects. If both objects are nullptr, they are considered to be equal.
     * @param a Object to compare.
     * @param b Object to compare.
     */
    static Bool Equals(const ObjectPtr<IBaseObject>& a, const ObjectPtr<IBaseObject>& b);

    /*!
     * @brief Disassociates this smart pointer object from the interface that it represents.
     *
     * The reference count is not decremented nor incremented.
     *
     * Use this function when the object is created in a function and the function returns the interface
     * to the object.
     *
     * If the object is created outside of the function and the function returns the interface, use
     * `addRefAndReturn`
     *
     * Example:
     * @code
     * IBaseObject* createObject()
     * {
     *     // create smart pointer with wrapped IBaseObject interface
     *     auto obj = BaseObject();
     *     // return wrapped interface, the reference count is not decremented
     *     return obj.detach();
     * }
     * @endcode
     */
    T* detach();

    /*!
     * @brief Disposes all references held by the object.
     *
     * An object that holds references to other interfaces must reset these references to null in `dispose`.
     * This method is called automatically when the reference count drops to zero.
     *
     * Call this method manually to break cycle references. If two objects have reference to each other,
     * calling `dispose` will break cycle references.
     *
     * After `dispose` is called, the object should not be accessed again, except by calling `releaseRef`
     * function.
     */
    void dispose() const;

    /*!
     * @brief Resets the wrapped interface to nullptr.
     *
     * If the smart pointer does not borrow the interface, calling `release` decrements the reference count. If the
     * reference count equals 0 after calling `release`, it destroys the object that implements the interface.
     *
     * The wrapped interface is set to `nullptr`.
     *
     * Smart pointer destructor does the same thing as `release`.
     */
    void release();

    /*!
     * @brief Increments the reference count and returns the interface.
     *
     * Call `addRefAndReturn` to return internally stored interface as the function return value. The caller is
     * responsible for releasing returned interface.
     *
     * Example:
     * @code
     * class SomeClass
     * {
     * public:
     *     BaseObjectPtr obj;
     * }
     *
     * IBaseObject* SomeClass::createObject()
     * {
     *     return obj.addRefAndReturn();
     * }
     * @endcode
     */
    T* addRefAndReturn() const;

    ObjectPtr<T>& operator=(const ObjectPtr<T>& ptr);
    ObjectPtr<T>& operator=(ObjectPtr<T>&& ptr) noexcept;

    template <class U>
    ObjectPtr<T>& operator=(const ObjectPtr<U>& ptr);

    template <class U>
    ObjectPtr<T>& operator=(ObjectPtr<U>&& ptr) noexcept;

    ObjectPtr<T>& operator=(T*& obj);
    ObjectPtr<T>& operator=(T*&& obj);
    ObjectPtr<T>& operator=(std::nullptr_t);
    ObjectPtr<T>& operator=(const wchar_t* value);
    ObjectPtr<T>& operator=(ConstCharPtr value);

    template <typename TComPtr, std::enable_if_t<HasIID<TComPtr>::value, int> = 0>
    ObjectPtr<T>& operator=(const TComPtr& unknown);

    template <typename U, std::enable_if_t<is_ct_conv<U>::value, int> = 0>
    ObjectPtr<T>& operator=(const U& value);

    template <typename U,
              typename V = T,
              std::enable_if_t<is_ct_conv<U>::value && !std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int> = 0>
    operator U() const;

    template <typename U,
              typename V = T,
              std::enable_if_t<is_ct_conv<U>::value && std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int> = 0>
    operator U() const;

    template <typename U,
              typename V = T,
              std::enable_if_t<is_ct_conv<U>::value && !std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int> = 0>
    U getValue(U defaultValue) const;

    template <typename U,
              typename V = T,
              std::enable_if_t<is_ct_conv<U>::value && std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int> = 0>
    U getValue(U defaultValue) const;

    // operator +
    template <class U>
    ObjectPtr<T> operator+(const U& other);

    template <class U>
    ObjectPtr<IBaseObject> operator+(const ObjectPtr<U>& other);

    template <typename V = T, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int> = 0>
    ObjectPtr<T> operator+(const ObjectPtr<T>& other);

    // operator -
    template <class U>
    ObjectPtr<T> operator-(const U& other);

    template <class U>
    ObjectPtr<IBaseObject> operator-(const ObjectPtr<U>& other);

    template <typename V = T, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int> = 0>
    ObjectPtr<T> operator-(const ObjectPtr<T>& other);

    // operator unary minus
    template <typename V = T, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int> = 0>
    ObjectPtr<T> operator-();

    template <typename V = T, std::enable_if_t<!is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int> = 0>
    ObjectPtr<T> operator-();

    // dereference * operator
    T* operator*() const;

    // operator *
    template <class U>
    ObjectPtr<T> operator*(const U& other);

    template <class U>
    ObjectPtr<IBaseObject> operator*(const ObjectPtr<U>& other);

    template <typename V = T, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int> = 0>
    ObjectPtr<T> operator*(const ObjectPtr<T>& other);

    // operator /
    template <class U>
    ObjectPtr<T> operator/(const U& other);

    template <class U>
    ObjectPtr<IBaseObject> operator/(const ObjectPtr<U>& other);

    template <typename V = T, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int> = 0>
    ObjectPtr<T> operator/(const ObjectPtr<T>& other);

    // operator ||
    template <class U>
    ObjectPtr<T> operator||(const U& other);

    template <class U>
    ObjectPtr<IBaseObject> operator||(const ObjectPtr<U>& other);

    template <typename V = T, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int> = 0>
    ObjectPtr<T> operator||(const ObjectPtr<T>& other);

    // operator &&
    template <class U>
    ObjectPtr<T> operator&&(const U& other);

    template <class U>
    ObjectPtr<IBaseObject> operator&&(const ObjectPtr<U>& other);

    template <typename V = T, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int> = 0>
    ObjectPtr<T> operator&&(const ObjectPtr<T>& other);

    T* operator->() const;

    /*!
     * @brief Casts the pointer to the wrapped interface.
     * @return The pointer to the interface.
     */
    operator T*() const;

    /*!
     * @brief Checks if the smart pointer is not empty.
     */
    bool assigned() const;

    /*!
     * @brief Gets the pointer to the wrapped interface.
     * @return The pointer to the interface.
     */
    T* getObject() const;

    /*!
     * @brief Gets the address of the pointer to the wrapped interface.
     * @return The address of the pointer to the interface.
     */
    T** addressOf();

    /*!
     * @brief Gets the address of the pointer to the wrapped interface.
     * @return The address of the pointer to the interface.
     */
    T** operator&();  // NOLINT(google-runtime-operator)
    // explicit operator bool() const;

    /*!
     * @brief Casts the wrapped interface to another interface
     * @param borrow Increments the reference count if false, keeps the reference count otherwise.
     *
     * Throws an exception if cast fails.
     */
    template <class U>
    U* as(bool borrow = false) const;

    /*!
     * @brief Casts the wrapped interface to another interface
     * @param borrow Increments the reference count if false, keeps the reference count otherwise.
     *
     * If cast fails, the wrapped interface set to nullptr.
     */
    template <class U>
    U* asOrNull(bool borrow = false) const;

    /*!
     * @brief Casts the wrapped interface to another interface and wraps in a smart pointer.
     * @param borrow Increments the reference count if false, keeps the reference count otherwise.
     *
     * Throws an exception if cast fails.
     */
    template <class U, class Ptr = typename InterfaceToSmartPtr<U>::SmartPtr>
    Ptr asPtr(bool borrow = false) const;

    /*!
     * @brief Casts the wrapped interface to another interface and wraps in a smart pointer.
     * @param borrow Increments the reference count if false, keeps the reference count otherwise.
     *
     * If cast fails, the wrapped interface set to nullptr.
     */
    template <class U, class Ptr = typename InterfaceToSmartPtr<U>::SmartPtr>
    Ptr asPtrOrNull(bool borrow = false) const;

    /*!
     * @brief Checks if the wrapped interface supports another interface.
     * @tparam U The interface to check for support.
     * @return True if the interface is supported, False otherwise.
     */
    template <class U>
    bool supportsInterface() const;

    /*!
     * @brief Checks if the wrapped interface supports another interface using the interface ID.
     * @param id The ID of the interface to check for support.
     * @return True if the interface is supported, False otherwise.
     */
    bool supportsInterface(const IntfID& id) const;

    /*!
     * @brief Gets the object's `CoreType`.
     * @return CoreType of the object.
     *
     * If the object does not support `ICoreType` interface, ctObject is returned as CoreType.
     */
    CoreType getCoreType() const;

    /*!
     * @brief Converts the object to the specified `CoreType`.
     * @return ct `CoreType` of the converted object.
     */
    ObjectPtr<IBaseObject> convertTo(CoreType ct) const;

    /*!
     * @brief Returns hash code of the object.
     * @return Hash code Hash code.
     */
    SizeT getHashCode() const;

    /*!
     * @brief Converts the object to the string object.
     * @return The Converted smart pointer wrapped to the IString interface.
     */
    ObjectPtr<IString> toString() const;

    /*!
     * @brief Freezes the object.
     *
     * Function freezes the object if it supports the IFreezable interface. Otherwise, it throws
     * an exception.
     *
     * Freezing the object makes it immutable.
     */
    void freeze() const;

    /*!
     * @brief Checks if the object is frozen.
     * @retval True The object is frozen.
     * @retval False The object is not frozen.
     *
     * Function freezes the object if it supports the IFreezable interface. Otherwise, it throws
     * an exception.
     *
     * If it does not support the IFreezable interface, it throws
     * an exception.
     */
    Bool isFrozen() const;

    ConstCharPtr getSerializeId() const;
    void serialize(const ObjectPtr<ISerializer>& serializer) const;

    /*!
     * @brief Calls the stored callback.
     * @param params Parameters passed to the callback.
     *
     * If it does not support the IProcedure interface, it throws an exception.
     *
     * If the callback expects no parameters, the `params` parameter has to be `nullptr`. If it
     * expects a single parameter, pass any openDAQ object as the `params` parameter.
     * If it expects multiple parameters, pass a list of openDAQ objects as the `params` parameter.
     */
    void dispatch(const ObjectPtr<IBaseObject>& params) const;

    /*!
     * @brief Calls the stored callback without any parameters.
     *
     * If it does not support the IProcedure interface, it throws an exception.
     */
    void dispatch() const;

    /*!
     * @brief Calls the stored callback without any parameters.
     *
     * If it does not support the `IProcedure` interface, it throws an exception.
     */
    void execute() const;

    /*!
     * @brief Calls the stored callback.
     * @param params Parameters passed to the callback.
     *
     * If it does not support the `IProcedure` interface, it throws an exception.
     *
     * Params are of supported Core types type which are internally converted to their object
     * representation.
     */
    template <typename... Params>
    void execute(Params... params) const;

    /*!
     * @brief Calls the stored callback and returns the result.
     * @param params Parameters passed to the callback.
     * @return Result wrapped in a smart pointer.
     *
     * If it does not support the `IFunction` interface, it throws an exception.
     *
     * Params are of supported Core types type which is internally converted to it's object
     * representation.
     */
    template <typename... Params>
    ObjectPtr<IBaseObject> call(Params... params) const;

    /*!
     * @brief Checks if the reference is borrowed.
     * @return Result True if it is a borrowed reference.
     */
    bool isBorrowed() const;

protected:
    T* object{};
    bool borrowed{};
    IIterator* createStartIteratorInterface() const;
    IIterator* createEndIteratorInterface() const;

private:
    template <typename U,
              BinOperationType O,
              typename V = T,
              std::enable_if_t<is_ct_conv<U>::value && std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int> = 0>
    ObjectPtr<T> internalBinOp(const U& other);

    template <typename U,
              BinOperationType O,
              typename V = T,
              std::enable_if_t<is_ct_conv<U>::value && !std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int> = 0>
    ObjectPtr<T> internalBinOp(const U& other);
};

/*!@}*/

template <class T>
struct ObjectPtrHash
{
    size_t operator()(const ObjectPtr<T>& key) const
    {
        SizeT hashCode;
        if (key.assigned())
            return key.getHashCode();
        else
            hashCode = 0;
        return hashCode;
    }
};

template <class T>
struct ObjectPtrEqualTo
{
    bool operator()(const ObjectPtr<T>& a, const ObjectPtr<T>& b) const
    {
        bool r = a == b;
        return r;
    }
};

#define INTF_CAST(obj, intf) obj.as<intf>()
#define PTR_CAST(obj, intf) obj.asPtr<intf>()

// getters

template <class Intf>
static std::string getString(Intf* obj)
{
    return objectToString(obj);
}

template <class Intf>
static std::wstring getWString(Intf* obj)
{
    CharPtr str;
    auto err = obj->toString(&str);
    checkErrorInfo(err);

    std::string s = str;
    daqFreeMemory(str);
    return CoreTypeHelper<std::wstring>::stringToWString(s);
}

template <typename T, class Intf = typename CoreTypeHelper<T>::Interface>
static T getValueFromObject(Intf* typeObj)
{
    typename CoreTypeHelper<T>::TrueType trueTypeValue;
    checkErrorInfo(typeObj->getValue(&trueTypeValue));

    T value = static_cast<T>(trueTypeValue);
    return value;
}

template <>
inline std::wstring getValueFromObject<std::wstring, IString>(IString* typeObj)
{
    ConstCharPtr value;
    checkErrorInfo(typeObj->getCharPtr(&value));
    std::string str(value);

    return CoreTypeHelper<std::wstring>::stringToWString(str);
}

template <>
inline std::string getValueFromObject<std::string, IString>(IString* typeObj)
{
    ConstCharPtr value;
    checkErrorInfo(typeObj->getCharPtr(&value));
    return std::string(value);
}

template <>
inline ComplexFloat32 getValueFromObject<ComplexFloat32, IComplexNumber>(IComplexNumber* typeObj)
{
    ComplexFloat64 value;
    typeObj->getValue(&value);
    return ComplexFloat32(value.real, value.imaginary);
}

template <typename T>
static T getValueFromConvertible(IBaseObject* obj)
{
    IConvertible* convObj;
    ErrCode err = obj->borrowInterface(IConvertible::Id, reinterpret_cast<void**>(&convObj));
    checkErrorInfo(err);

    T value;
    err = CoreTypeHelper<T>::FromConvertible(value, convObj);
    checkErrorInfo(err);
    return value;
}

template <typename T, class Intf = typename CoreTypeHelper<T>::Interface>
static T baseObjectToValue(IBaseObject* obj)
{
    T value;
    Intf* typeObj;
    ErrCode err = obj->borrowInterface(Intf::Id, reinterpret_cast<void**>(&typeObj));
    if (OPENDAQ_FAILED(err))
    {
        value = getValueFromConvertible<T>(obj);
    }
    else
    {
        try
        {
            value = getValueFromObject<T, Intf>(typeObj);
        }
        catch (const InvalidTypeException&)
        {
            value = getValueFromConvertible<T>(obj);
        }
    }
    return value;
}

template <>
inline std::string baseObjectToValue(IBaseObject* obj)
{
    std::string value;
    IString* typeObj;
    ErrCode err = obj->borrowInterface(IString::Id, reinterpret_cast<void**>(&typeObj));
    if (OPENDAQ_SUCCEEDED(err))
    {
        try
        {
            value = getValueFromObject<std::string, IString>(typeObj);
        }
        catch (const InvalidTypeException&)
        {
            value = getString(obj);
        }
    }
    else
    {
        value = getString(obj);
    }
    return value;
}

template <>
inline std::wstring baseObjectToValue(IBaseObject* obj)
{
    std::wstring value;
    IString* typeObj;
    ErrCode err = obj->borrowInterface(IString::Id, reinterpret_cast<void**>(&typeObj));
    if (OPENDAQ_SUCCEEDED(err))
    {
        try
        {
            value = getValueFromObject<std::wstring, IString>(typeObj);
        }
        catch (const InvalidTypeException&)
        {
            value = getWString(obj);
        }
    }
    else
    {
        value = getWString(obj);
    }
    return value;
}

// binary op helpers

template <class U, class V, BinOperationType O, typename CT>
ObjectPtr<IBaseObject> baseObjectBinOpOfType(const ObjectPtr<U>& left, const ObjectPtr<V>& right)
{
    return ObjectPtr<IBaseObject>(BinOperation<CT, O>::Op(static_cast<CT>(left), static_cast<CT>(right)));
}

template <class U, class V, BinOperationType O>
ObjectPtr<IBaseObject> baseObjectBinOp(const ObjectPtr<U>& left, const ObjectPtr<V>& right);

template <class U, class V, BinOperationType O>
ObjectPtr<IBaseObject> baseObjectBinOpOfTwoList(const ObjectPtr<U>& left,
                                                const CoreType /*leftCoreType*/,
                                                const ObjectPtr<V>& right,
                                                const CoreType /*rightCoreType*/)
{
    auto leftList = left.template asPtr<IList, ObjectPtr<IList>>(true);
    auto rightList = right.template asPtr<IList, ObjectPtr<IList>>(true);
    size_t leftCount;
    size_t rightCount;
    checkErrorInfo(leftList->getCount(&leftCount));
    checkErrorInfo(leftList->getCount(&rightCount));

    if (leftCount != rightCount)
        throw InvalidTypeException();

    ObjectPtr<IList> list = List_Create();
    for (size_t i = 0; i < leftCount; i++)
    {
        IBaseObject* leftObj;
        checkErrorInfo(leftList->getItemAt(i, &leftObj));
        IBaseObject* rightObj;
        checkErrorInfo(rightList->getItemAt(i, &rightObj));

        auto resultObj = baseObjectBinOp<IBaseObject, IBaseObject, O>(std::move(leftObj), std::move(rightObj));
        checkErrorInfo(list->moveBack(resultObj.detach()));
    }
    return list;
}

template <class U, class V, BinOperationType O>
ObjectPtr<IBaseObject> baseObjectBinOpOfListAndScalar(const ObjectPtr<U>& left,
                                                      const CoreType leftCoreType,
                                                      const ObjectPtr<V>& right,
                                                      const CoreType /*rightCoreType*/)
{
    ObjectPtr<IList> listObj;
    ObjectPtr<IBaseObject> scalarObj;
    if (leftCoreType == ctList)
    {
        listObj = left.template asPtr<IList, ObjectPtr<IList>>(true);
        scalarObj = right.template asPtr<IBaseObject>(true);
    }
    else
    {
        listObj = right.template asPtr<IList, ObjectPtr<IList>>(true);
        scalarObj = left.template asPtr<IBaseObject>(true);
    }

    size_t count;
    checkErrorInfo(listObj->getCount(&count));
    ObjectPtr<IList> list = List_Create();
    for (size_t i = 0; i < count; i++)
    {
        IBaseObject* itemObj;
        checkErrorInfo(listObj->getItemAt(i, &itemObj));

        auto resultObj = baseObjectBinOp<IBaseObject, IBaseObject, O>(std::move(itemObj), scalarObj);
        checkErrorInfo(list->moveBack(resultObj.detach()));
    }
    return list;
}

template <class U, class V, BinOperationType O>
ObjectPtr<IBaseObject> baseObjectBinOpOfList(const ObjectPtr<U>& left,
                                             const CoreType leftCoreType,
                                             const ObjectPtr<V>& right,
                                             const CoreType rightCoreType)
{
    if (leftCoreType == rightCoreType)
        return baseObjectBinOpOfTwoList<U, V, O>(left, leftCoreType, right, rightCoreType);

    return baseObjectBinOpOfListAndScalar<U, V, O>(left, leftCoreType, right, rightCoreType);
}

template <class U, class V, BinOperationType O>
ObjectPtr<IBaseObject> baseObjectBinOp(const ObjectPtr<U>& left, const ObjectPtr<V>& right)
{
    auto leftCoreType = left.getCoreType();
    auto rightCoreType = right.getCoreType();
    auto resultCoreType = leftCoreType > rightCoreType ? leftCoreType : rightCoreType;
    if (resultCoreType > ctList)
        throw InvalidTypeException();

    switch (resultCoreType)
    {
        case ctBool:
            return baseObjectBinOpOfType<U, V, O, Bool>(left, right);
        case ctInt:
            return baseObjectBinOpOfType<U, V, O, Int>(left, right);
        case ctFloat:
            return baseObjectBinOpOfType<U, V, O, Float>(left, right);
        case ctString:
            return baseObjectBinOpOfType<U, V, O, std::wstring>(left, right);
        case ctList:
            return baseObjectBinOpOfList<U, V, O>(left, leftCoreType, right, rightCoreType);
        default:
            throw InvalidTypeException();
    }
}

template <class T,
          typename U,
          class O,
          typename V = T,
          typename std::enable_if<is_ct_conv<U>::value && std::is_same<V, typename CoreTypeHelper<U>::Interface>::value, int>::type = 0>
ObjectPtr<T> baseObjectBinOp(const ObjectPtr<T>& obj, const U other)
{
    U valueThis = getValueFromObject<U>(obj.getObject());
    return CoreTypeHelper<U>::Create(O{}(valueThis, other));
}

template <class T,
          typename U,
          class O1,
          BinOperationType O2,
          typename V = T,
          typename std::enable_if<is_ct_conv<U>::value && !std::is_same<V, typename CoreTypeHelper<U>::Interface>::value, int>::type = 0>
ObjectPtr<T> baseObjectBinOpDynamic(const ObjectPtr<T>& obj, const U other)
{
    if (obj.getCoreType() == ctList)
    {
        typedef typename CoreTypeHelper<U>::Interface Intf;
        ObjectPtr<Intf> scalarObj(other);
        return baseObjectBinOpOfListAndScalar<T, Intf, O2>(obj, ctList, scalarObj, scalarObj.getCoreType());
    }

    U valueThis = baseObjectToValue<U>(obj.getObject());
    return CoreTypeHelper<U>::Create(O1{}(valueThis, other));
}

template <class T, BinOperationType O>
ObjectPtr<T> baseObjectBinOp(const ObjectPtr<T>& left, const ObjectPtr<T>& right)
{
    typedef typename IntfToCoreType<T>::CoreType CT;

    auto valueLeft = static_cast<CT>(left);
    auto valueRight = static_cast<CT>(right);
    return CoreTypeHelper<CT>::Create(typename BinOperationToStdOp<O>::op{}(valueLeft, valueRight));
}

template <class T, UnaryOperationType O>
ObjectPtr<T> baseObjectUnaryOp(const ObjectPtr<T>& obj)
{
    typedef typename IntfToCoreType<T>::CoreType CT;

    auto value = static_cast<CT>(obj);
    return CoreTypeHelper<CT>::Create(typename UnaryOperationToStdOp<O>::op{}(value));
}

template <class T, UnaryOperationType O, typename CT>
ObjectPtr<IBaseObject> baseObjectUnaryOpOfType(const ObjectPtr<T>& value)
{
    return ObjectPtr<IBaseObject>(UnaryOperation<CT, O>::Op(static_cast<CT>(value)));
}

template <class T, UnaryOperationType O>
ObjectPtr<IBaseObject> baseObjectUnaryOpBaseObject(const ObjectPtr<T>& value)
{
    auto valueCoreType = value.getCoreType();
    if (valueCoreType != ctInt && valueCoreType != ctFloat)
        throw InvalidTypeException();

    switch (valueCoreType)
    {
        case ctInt:
            return baseObjectUnaryOpOfType<T, O, Int>(value);
        case ctFloat:
            return baseObjectUnaryOpOfType<T, O, Float>(value);
        default:
            throw InvalidTypeException();
    }
}

// operators

template <class T>
std::ostream& operator<<(std::ostream& stream, const ObjectPtr<T>& value)
{
    stream << objectToString(value.getObject());
    return stream;
}

// comparison operators

template <class T, typename V, typename std::enable_if<is_ct_conv<V>::value, int>::type = 0>
bool operator==(const ObjectPtr<T>& lhs, V rhs)
{
    IBaseObject* obj = lhs.getObject();
    if (obj != nullptr)
        return baseObjectToValue<V>(obj) == rhs;

    throw InvalidParameterException();
}

template <class T, typename V, typename std::enable_if<is_ct_conv<V>::value, int>::type = 0>
bool operator==(V lhs, const ObjectPtr<T>& rhs)
{
    return rhs == lhs;
}

template <class T>
bool operator==(const ObjectPtr<T>& lhs, ConstCharPtr rhs)
{
    IBaseObject* obj = lhs.getObject();
    if (obj != nullptr)
    {
        std::string str = baseObjectToValue<std::string>(obj);
        return str == rhs;
    }

    throw InvalidParameterException();
}

template <class T>
bool operator==(ConstCharPtr lhs, const ObjectPtr<T>& rhs)
{
    return rhs == lhs;
}

template <class T>
bool operator!=(const ObjectPtr<T>& lhs, ConstCharPtr rhs)
{
    IBaseObject* obj = lhs.getObject();
    if (obj != nullptr)
    {
        std::string str = baseObjectToValue<std::string>(obj);
        return str != rhs;
    }

    throw InvalidParameterException();
}

template <class T>
bool operator!=(ConstCharPtr lhs, const ObjectPtr<T>& rhs)
{
    return rhs != lhs;
}

template <class T, class U, ErrCode Op>
bool compareObjectPtr(const ObjectPtr<T>& lhs, const ObjectPtr<U>& rhs)
{
    if (lhs.getObject() == nullptr)
        return rhs.getObject() == nullptr;

    auto lhsComp = lhs.template asPtrOrNull<IComparable>(true);
    if (lhsComp.assigned())
    {
        ErrCode err = lhsComp->compareTo(rhs.getObject());
        checkErrorInfo(err);
        return err == Op;
    }

    return lhs.equals(rhs);
}

template <class T, class U>
bool operator==(const ObjectPtr<T>& lhs, const ObjectPtr<U>& rhs)
{
    return compareObjectPtr<T, U, OPENDAQ_EQUAL>(lhs, rhs);
}

template <class T, class U>
bool operator!=(const ObjectPtr<T>& lhs, const ObjectPtr<U>& rhs)
{
    return !compareObjectPtr<T, U, OPENDAQ_EQUAL>(lhs, rhs);
}

template <class T, class U>
bool operator>(const ObjectPtr<T>& lhs, const ObjectPtr<U>& rhs)
{
    return compareObjectPtr<T, U, OPENDAQ_GREATER>(lhs, rhs);
}

template <class T, class U>
bool operator<(const ObjectPtr<T>& lhs, const ObjectPtr<U>& rhs)
{
    return compareObjectPtr<T, U, OPENDAQ_LOWER>(lhs, rhs);
}

template <class T, class U>
bool operator>=(const ObjectPtr<T>& lhs, const ObjectPtr<U>& rhs)
{
    return (compareObjectPtr<T, U, OPENDAQ_GREATER>(lhs, rhs) || compareObjectPtr<T, U, OPENDAQ_EQUAL>(lhs, rhs));
}

template <class T, class U>
bool operator<=(const ObjectPtr<T>& lhs, const ObjectPtr<U>& rhs)
{
    return (compareObjectPtr<T, U, OPENDAQ_LOWER>(lhs, rhs) || compareObjectPtr<T, U, OPENDAQ_EQUAL>(lhs, rhs));
}

template <class T>
bool operator==(const ObjectPtr<T>& lhs, std::nullptr_t /*rhs*/)
{
    return (lhs.getObject() == nullptr);
}

template <class T>
bool operator!=(const ObjectPtr<T>& lhs, std::nullptr_t /*rhs*/)
{
    return (lhs.getObject() != nullptr);
}

template <class T>
bool operator==(std::nullptr_t lhs, const ObjectPtr<T>& rhs)
{
    return rhs == lhs;
}

template <class T>
bool operator!=(std::nullptr_t lhs, const ObjectPtr<T>& rhs)
{
    return rhs != lhs;
}

template <class T>
ObjectPtr<T>::ObjectPtr()
    : object(nullptr)
    , borrowed(false)
{
}

template <class T>
ObjectPtr<T>::ObjectPtr(std::nullptr_t)
    : object(nullptr)
    , borrowed(false)
{
}

template <class T>
ObjectPtr<T>::ObjectPtr(const ObjectPtr<T>& objPtr)
    : object(objPtr.object)
    , borrowed(false)
{
    if (object)
        object->addRef();
}

template <class T>
template <class U>
ObjectPtr<T>::ObjectPtr(const ObjectPtr<U>& objPtr)
    : borrowed(false)
{
    if (objPtr.getObject() != nullptr)
    {
        T* newIntf;
        ErrCode err = objPtr->queryInterface(T::Id, (void**) &newIntf);
        checkErrorInfo(err);

        object = newIntf;
    }
    else
    {
        object = nullptr;
    }
}

template <class T>
template <class U>
ObjectPtr<T>::ObjectPtr(ObjectPtr<U>&& objPtr)
{
    U* otherObj = objPtr.getObject();
    if (otherObj != nullptr)
    {
        object = objPtr.template as<T>(true);
        borrowed = objPtr.isBorrowed();
        objPtr.detach();
    }
    else
    {
        object = nullptr;
    }
}

template <class T>
ObjectPtr<T>::ObjectPtr(ObjectPtr<T>&& objPtr) noexcept
    : object(objPtr.object)
    , borrowed(objPtr.borrowed)
{
    objPtr.borrowed = false;
    objPtr.object = nullptr;
}

template <class T>
ObjectPtr<T>::ObjectPtr(T*& obj)
    : object(obj)
    , borrowed(false)
{
    if (object)
        object->addRef();
}

template <class T>
ObjectPtr<T>::ObjectPtr(T*&& obj)
    : object(obj)
    , borrowed(false)
{
}

template <class T>
ObjectPtr<T>::ObjectPtr(IWeakRef* obj)
    : borrowed(false)
{
    if (obj != nullptr)
    {
        Finally final([&obj]() { obj->releaseRef(); });

        T* newIntf;
        ErrCode err = obj->getRefAs(T::Id, (void**) &newIntf);
        checkErrorInfo(err);

        object = newIntf;
    }
    else
    {
        object = nullptr;
    }
}

template <class T>
template <class U, std::enable_if_t<std::is_base_of_v<IBaseObject, U>, int>>
ObjectPtr<T>::ObjectPtr(U*& obj)
    : borrowed(false)
{
    if (obj != nullptr)
    {
        T* newIntf;

        auto* nonConst = const_cast<typename std::remove_const<U>::type*>(obj);
        ErrCode err = nonConst->queryInterface(T::Id, (void**) &newIntf);
        checkErrorInfo(err);

        object = newIntf;
    }
    else
    {
        object = nullptr;
    }
}

template <class T>
template <class U, std::enable_if_t<std::is_base_of_v<IBaseObject, U>, int>>
ObjectPtr<T>::ObjectPtr(U*&& obj)
    : borrowed(false)
{
    if (obj != nullptr)
    {
        Finally final([&obj]() { obj->releaseRef(); });

        T* newIntf;
        ErrCode err = obj->queryInterface(T::Id, (void**) &newIntf);
        checkErrorInfo(err);

        object = newIntf;
    }
    else
    {
        object = nullptr;
    }
}

template <class T>
template <typename TComPtr, std::enable_if_t<HasIID<TComPtr>::value, int>>
ObjectPtr<T>::ObjectPtr(const TComPtr& unknown)
{
    if (!unknown)
    {
        throw ArgumentNullException("Interface must not be null");
    }

    IUnknown* intf = (IUnknown*) unknown.GetInterfacePtr();

    ErrCode err = intf->queryInterface(T::Id, (void**) &object);
    checkErrorInfo(err);
}

template <class T>
ObjectPtr<T>::ObjectPtr(const wchar_t* value)
    : borrowed(false)
{
    object = String_Create(CoreTypeHelper<std::wstring>::wstringToString(value).c_str());
}

template <class T>
ObjectPtr<T>::ObjectPtr(ConstCharPtr value)
    : borrowed(false)
{
    object = String_Create(value);
}

template <class T>
template <typename U, std::enable_if_t<is_ct_conv<U>::value && !std::is_enum_v<U>, int>>
ObjectPtr<T>::ObjectPtr(const U& value)
    : borrowed(false)
{
    using CreateInterface = std::remove_pointer_t<decltype(CoreTypeHelper<U>::Create(value))>;

    if constexpr (supports_conv_from_coretype<T>::value)
    {
        auto intf = CoreTypeHelper<U>::Create(value);
        checkErrorInfo(intf->borrowInterface(T::Id, reinterpret_cast<void**>(&object)));
    }
    else if constexpr (!std::is_same_v<T, IBaseObject> && !std::is_same_v<T, CreateInterface>)
    {
        object = CoreTypeHelper<typename IntfToCoreType<T>::CoreType>::Create(value);
    }
    else
    {
        object = CoreTypeHelper<U>::Create(value);
    }
}

template <class T>
template <class U>
typename InterfaceToSmartPtr<U>::SmartPtr ObjectPtr<T>::Borrow(T*& obj)
{
    typename InterfaceToSmartPtr<U>::SmartPtr objPtr;
    objPtr.object = obj;
    objPtr.borrowed = true;
    return objPtr;
}

template <class T>
template <class U>
typename InterfaceToSmartPtr<U>::SmartPtr ObjectPtr<T>::Adopt(T* obj)
{
    typename InterfaceToSmartPtr<U>::SmartPtr objPtr;
    objPtr.object = obj;
    objPtr.borrowed = false;
    return objPtr;
}

// template <typename T>
// template <typename TComPtr, typename TInterface, typename std::enable_if<HasIID<TComPtr>::value, int>::type>
// typename InterfaceToSmartPtr<TInterface>::SmartPtr ObjectPtr<T>::FromComPtr(const TComPtr& unknown)
// {
//     if (!unknown)
//     {
//         throw ArgumentNullException("Interface must not be null");
//     }
//
//     IUnknown* intf = (IUnknown*) unknown.GetInterfacePtr();
//
//     typename InterfaceToSmartPtr<TInterface>::SmartPtr ptr;
//     ErrCode err = intf->queryInterface(TInterface::Id, (void**) &ptr);
//     checkErrorInfo(err);
//
//     return ptr;
// }

template <class T>
template <class U, class V, std::enable_if_t<std::is_base_of_v<IBaseObject, U>, int>>
typename InterfaceToSmartPtr<V>::SmartPtr ObjectPtr<T>::Borrow(U*& obj)
{
    typename InterfaceToSmartPtr<V>::SmartPtr objPtr;

    T* intf;
    auto res = obj->borrowInterface(T::Id, reinterpret_cast<void**>(&intf));
    checkErrorInfo(res);

    objPtr.object = intf;
    objPtr.borrowed = true;
    return objPtr;
}

template <typename T>
ObjectPtr<T> ObjectPtr<T>::Borrow(const ObjectPtr<T>& ptr)
{
    ObjectPtr<T> objPtr;
    objPtr.object = ptr.getObject();
    objPtr.borrowed = true;
    return objPtr;
}


template <typename T>
bool ObjectPtr<T>::equals(ObjectPtr<IBaseObject> other) const
{
    if (!object)
    {
        if (!other.assigned())
        {
            return true;
        }
        return false;
    }

    Bool eq;
    ErrCode errCode = object->equals(other, &eq);
    checkErrorInfo(errCode);

    return eq;
}

template <typename T>
inline Bool ObjectPtr<T>::Equals(const ObjectPtr<IBaseObject>& a, const ObjectPtr<IBaseObject>& b)
{
    Bool eq{};
    return (a == b) || (a != nullptr && OPENDAQ_SUCCEEDED(a->equals(b, &eq)) && eq);
}

template <class T>
T* ObjectPtr<T>::detach()
{
    T* tmpObj = object;
    object = nullptr;
    borrowed = false;
    return tmpObj;
}

template <class T>
ObjectPtr<T>::~ObjectPtr()
{
    if (object && !borrowed)
    {
        T* ptr = object;
        object = nullptr;

        ptr->releaseRef();
    }
}

template <class T>
T* ObjectPtr<T>::operator->() const
{
    if (!object)
        throw InvalidParameterException();

    return object;
}

template <class T>
ObjectPtr<T>& ObjectPtr<T>::operator=(const ObjectPtr<T>& ptr)
{
    if (object && !borrowed)
        object->releaseRef();

    borrowed = false;
    object = ptr.object;

    if (object)
        object->addRef();

    return *this;
}

template <class T>
ObjectPtr<T>& ObjectPtr<T>::operator=(ObjectPtr<T>&& ptr) noexcept
{
    if (object && !borrowed)
        object->releaseRef();

    object = ptr.object;
    borrowed = ptr.borrowed;
    ptr.object = nullptr;
    ptr.borrowed = false;
    return *this;
}

template <class T>
template <class U>
ObjectPtr<T>& ObjectPtr<T>::operator=(const ObjectPtr<U>& ptr)
{
    if (object && !borrowed)
        object->releaseRef();

    borrowed = false;
    if (ptr.assigned())
        object = ptr.template as<T>();
    else
        object = nullptr;
    return *this;
}

template <class T>
template <class U>
ObjectPtr<T>& ObjectPtr<T>::operator=(ObjectPtr<U>&& ptr) noexcept
{
    if (object && !borrowed)
        object->releaseRef();

    if (ptr.assigned())
    {
        object = ptr.template as<T>(true);
        borrowed = ptr.isBorrowed();
        ptr.detach();
    }
    else
    {
        object = nullptr;
        borrowed = false;
    }
    return *this;
}

template <class T>
ObjectPtr<T>& ObjectPtr<T>::operator=(T*& obj)
{
    if (object && !borrowed)
        object->releaseRef();
    borrowed = false;
    object = obj;
    if (object)
        object->addRef();
    return *this;
}

template <class T>
ObjectPtr<T>& ObjectPtr<T>::operator=(T*&& obj)
{
    if (object && !borrowed)
        object->releaseRef();
    borrowed = false;
    object = obj;
    return *this;
}

template <class T>
ObjectPtr<T>& ObjectPtr<T>::operator=(std::nullptr_t)
{
    if (object && !borrowed)
        object->releaseRef();
    borrowed = false;
    object = nullptr;
    return *this;
}

template <class T>
ObjectPtr<T>& ObjectPtr<T>::operator=(const wchar_t* value)
{
    if (object && !borrowed)
        object->releaseRef();
    object = String_Create(CoreTypeHelper<std::wstring>::wstringToString(value).c_str());
    borrowed = false;
    return *this;
}

template <class T>
ObjectPtr<T>& ObjectPtr<T>::operator=(const char* value)
{
    if (object && !borrowed)
        object->releaseRef();
    object = String_Create(value);
    borrowed = false;
    return *this;
}

template <typename T>
template <typename TComPtr, std::enable_if_t<HasIID<TComPtr>::value, int>>
ObjectPtr<T>& ObjectPtr<T>::operator=(const TComPtr& unknown)
{
    if (object && !borrowed)
        object->releaseRef();

    auto* intf = (IUnknown*) unknown.GetInterfacePtr();

    ErrCode err = intf->queryInterface(T::Id, (void**) &object);
    checkErrorInfo(err);
    borrowed = false;

    return *this;
}

template <class T>
template <class U, std::enable_if_t<is_ct_conv<U>::value, int>>
ObjectPtr<T>& ObjectPtr<T>::operator=(const U& value)
{
    if (object && !borrowed)
        object->releaseRef();

    using CreateInterface = std::remove_pointer_t<decltype(CoreTypeHelper<U>::Create(value))>;

    if constexpr (supports_conv_from_coretype<T>::value)
    {
        auto intf = CoreTypeHelper<U>::Create(value);
        checkErrorInfo(intf->borrowInterface(T::Id, reinterpret_cast<void**>(&object)));
    }
    else if constexpr (!std::is_same_v<T, IBaseObject> && !std::is_same_v<T, CreateInterface>)
    {
        object = CoreTypeHelper<typename IntfToCoreType<T>::CoreType>::Create(value);
    }
    else
    {
        object = CoreTypeHelper<U>::Create(value);
    }
    borrowed = false;
    return *this;
}

template <class T>
template <typename U, typename V, std::enable_if_t<is_ct_conv<U>::value && !std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int>>
ObjectPtr<T>::operator U() const
{
    if (!object)
        throw InvalidParameterException();

    return baseObjectToValue<U>(object);
}

// template <class T>
// ObjectPtr<T>::operator bool() const
// {
//     if (!object)
//     {
//         return false;
//     }
//
//     return baseObjectToValue<bool>(object);
// }

template <class T>
template <typename U, typename V, std::enable_if_t<is_ct_conv<U>::value && std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int>>
ObjectPtr<T>::operator U() const
{
    if (!object)
        throw InvalidParameterException();

    return getValueFromObject<U>(object);
}

template <class T>
template <typename U, typename V, std::enable_if_t<is_ct_conv<U>::value && !std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int>>
U ObjectPtr<T>::getValue(U defaultValue) const
{
    if (object)
        return baseObjectToValue<U>(object);

    return defaultValue;
}

template <class T>
template <typename U, typename V, std::enable_if_t<is_ct_conv<U>::value && std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int>>
U ObjectPtr<T>::getValue(U defaultValue) const
{
    if (object)
        return getValueFromObject<U>(object);

    return defaultValue;
}

template <class T>
IIterator* ObjectPtr<T>::createStartIteratorInterface() const
{
    assert(ObjectPtr<T>::object);

    ErrCode errCode;
    IIterable* iterable;
    if constexpr (std::is_same_v<T, IIterable>)
    {
        iterable = object;
    }
    else
    {
        errCode = object->borrowInterface(IIterable::Id, reinterpret_cast<void**>(&iterable));
        checkErrorInfo(errCode);        
    }

    IIterator* iterator;
    errCode = iterable->createStartIterator(&iterator);
    checkErrorInfo(errCode);

    iterator->moveNext();
    return iterator;
}

template <class T>
IIterator* ObjectPtr<T>::createEndIteratorInterface() const
{
    assert(ObjectPtr<T>::object);

    ErrCode errCode;
    IIterable* iterable;
    if constexpr (std::is_same_v<T, IIterable>)
    {
        iterable = object;
    }
    else
    {
        errCode = object->borrowInterface(IIterable::Id, reinterpret_cast<void**>(&iterable));
        checkErrorInfo(errCode);
    }

    IIterator* iterator;
    errCode = iterable->createEndIterator(&iterator);
    checkErrorInfo(errCode);

    iterator->moveNext();
    return iterator;
}

template <class T>
template <typename U,
          BinOperationType O,
          typename V,
          std::enable_if_t<is_ct_conv<U>::value && std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int>>
ObjectPtr<T> ObjectPtr<T>::internalBinOp(const U& other)
{
    return baseObjectBinOp<T, U, typename BinOperationToStdOp<O>::op>(*this, other);
}

template <class T>
template <typename U,
          BinOperationType O,
          typename V,
          std::enable_if_t<is_ct_conv<U>::value && !std::is_same_v<V, typename CoreTypeHelper<U>::Interface>, int>>
ObjectPtr<T> ObjectPtr<T>::internalBinOp(const U& other)
{
    return baseObjectBinOpDynamic<T, U, typename BinOperationToStdOp<O>::op, O>(*this, other);
}

// + operator

template <class T>
template <class U>
ObjectPtr<T> ObjectPtr<T>::operator+(const U& other)
{
    return internalBinOp<U, BinOperationType::add>(other);
}

template <class T>
template <class U>
ObjectPtr<IBaseObject> ObjectPtr<T>::operator+(const ObjectPtr<U>& other)
{
    return baseObjectBinOp<T, U, BinOperationType::add>(*this, other);
}

template <class T>
template <typename V, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int>>
ObjectPtr<T> ObjectPtr<T>::operator+(const ObjectPtr<T>& other)
{
    return baseObjectBinOp<T, BinOperationType::add>(*this, other);
}

// - operator

template <class T>
template <class U>
ObjectPtr<T> ObjectPtr<T>::operator-(const U& other)
{
    return internalBinOp<U, BinOperationType::sub>(other);
}

template <class T>
template <class U>
ObjectPtr<IBaseObject> ObjectPtr<T>::operator-(const ObjectPtr<U>& other)
{
    return baseObjectBinOp<T, U, BinOperationType::sub>(*this, other);
}

template <class T>
template <typename V, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int>>
ObjectPtr<T> ObjectPtr<T>::operator-(const ObjectPtr<T>& other)
{
    return baseObjectBinOp<T, BinOperationType::sub>(*this, other);
}

// unary - operator
template <class T>
template <typename V, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int>>
ObjectPtr<T> ObjectPtr<T>::operator-()
{
    return baseObjectUnaryOp<T, UnaryOperationType::Negate>(*this);
}

template <class T>
template <typename V, std::enable_if_t<!is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int>>
ObjectPtr<T> ObjectPtr<T>::operator-()
{
    return baseObjectUnaryOpBaseObject<T, UnaryOperationType::Negate>(*this);
}

template <class T>
T* ObjectPtr<T>::operator*() const
{
    return object;
}

// * operator

template <class T>
template <class U>
ObjectPtr<T> ObjectPtr<T>::operator*(const U& other)
{
    return internalBinOp<U, BinOperationType::mul>(other);
}

template <class T>
template <class U>
ObjectPtr<IBaseObject> ObjectPtr<T>::operator*(const ObjectPtr<U>& other)
{
    return baseObjectBinOp<T, U, BinOperationType::mul>(*this, other);
}

template <class T>
template <typename V, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int>>
ObjectPtr<T> ObjectPtr<T>::operator*(const ObjectPtr<T>& other)
{
    return baseObjectBinOp<T, BinOperationType::mul>(*this, other);
}

// / operator

template <class T>
template <class U>
ObjectPtr<T> ObjectPtr<T>::operator/(const U& other)
{
    return internalBinOp<U, BinOperationType::div>(other);
}

template <class T>
template <class U>
ObjectPtr<IBaseObject> ObjectPtr<T>::operator/(const ObjectPtr<U>& other)
{
    return baseObjectBinOp<T, U, BinOperationType::div>(*this, other);
}

template <class T>
template <typename V, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int>>
ObjectPtr<T> ObjectPtr<T>::operator/(const ObjectPtr<T>& other)
{
    return baseObjectBinOp<T, BinOperationType::div>(*this, other);
}

// || operator

template <class T>
template <class U>
ObjectPtr<T> ObjectPtr<T>::operator||(const U& other)
{
    return internalBinOp<U, BinOperationType::logOr>(other);
}

template <class T>
template <class U>
ObjectPtr<IBaseObject> ObjectPtr<T>::operator||(const ObjectPtr<U>& other)
{
    return baseObjectBinOp<T, U, BinOperationType::logOr>(*this, other);
}

template <class T>
template <typename V, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int>>
ObjectPtr<T> ObjectPtr<T>::operator||(const ObjectPtr<T>& other)
{
    return baseObjectBinOp<T, BinOperationType::logOr>(*this, other);
}

// && operator

template <class T>
template <class U>
ObjectPtr<T> ObjectPtr<T>::operator&&(const U& other)
{
    return internalBinOp<U, BinOperationType::logAnd>(other);
}

template <class T>
template <class U>
ObjectPtr<IBaseObject> ObjectPtr<T>::operator&&(const ObjectPtr<U>& other)
{
    return baseObjectBinOp<T, U, BinOperationType::logAnd>(*this, other);
}

template <class T>
template <typename V, std::enable_if_t<is_ct_conv<typename IntfToCoreType<V>::CoreType>::value, int>>
ObjectPtr<T> ObjectPtr<T>::operator&&(const ObjectPtr<T>& other)
{
    return baseObjectBinOp<T, BinOperationType::logAnd>(*this, other);
}

// ---------------

template <class T>
T* ObjectPtr<T>::getObject() const
{
    return object;
}

template <class T>
bool ObjectPtr<T>::assigned() const
{
    return (object != nullptr);
}

template <class T>
ObjectPtr<T>::operator T*() const
{
    return object;
}

template <class T>
template <class U>
U* ObjectPtr<T>::as(bool borrow) const
{
    if (!object)
        throw InvalidParameterException();

    U* intf;
    ErrCode res;
    if (borrow)
        res = object->borrowInterface(U::Id, reinterpret_cast<void**>(&intf));
    else
        res = object->queryInterface(U::Id, reinterpret_cast<void**>(&intf));

    checkErrorInfo(res);
    return intf;
}

template <class T>
template <class U>
U* ObjectPtr<T>::asOrNull(bool borrow) const
{
    if (!object)
        throw InvalidParameterException();

    U* intf;
    ErrCode res;
    if (borrow)
    {
        res = object->borrowInterface(U::Id, reinterpret_cast<void**>(&intf));
    }
    else
    {
        res = object->queryInterface(U::Id, reinterpret_cast<void**>(&intf));
    }

    if (OPENDAQ_SUCCEEDED(res))
    {
        return intf;
    }
    return nullptr;
}

template <class T>
template <class U, class Ptr>
Ptr ObjectPtr<T>::asPtr(const bool borrow) const
{
    if (!object)
        throw InvalidParameterException();

    U* intf;
    if (borrow)
    {
        auto res = object->borrowInterface(U::Id, reinterpret_cast<void**>(&intf));
        checkErrorInfo(res);

        return Ptr::Borrow(intf);
    }

    auto res = object->queryInterface(U::Id, reinterpret_cast<void**>(&intf));
    checkErrorInfo(res);

    return Ptr(std::move(intf));
}

template <class T>
template <class U, class Ptr>
Ptr ObjectPtr<T>::asPtrOrNull(bool borrow) const
{
    if (!object)
        throw InvalidParameterException();

    U* intf;
    ErrCode res;
    if (borrow)
    {
        res = object->borrowInterface(U::Id, reinterpret_cast<void**>(&intf));
        if (OPENDAQ_SUCCEEDED(res))
            return Ptr::Borrow(intf);

        return Ptr();
    }

    res = object->queryInterface(U::Id, reinterpret_cast<void**>(&intf));

    if (OPENDAQ_SUCCEEDED(res))
        return Ptr(std::move(intf));

    return Ptr();
}

template <class T>
template <class U>
bool ObjectPtr<T>::supportsInterface() const
{
    return asPtrOrNull<U>(true).assigned();
}

template <class T>
bool ObjectPtr<T>::supportsInterface(const IntfID& id) const
{
    if (!object)
        throw InvalidParameterException();

    void* intf;
    auto res = object->borrowInterface(id, &intf);
    if (OPENDAQ_SUCCEEDED(res))
        return true;

    return false;
}


template <class T>
void ObjectPtr<T>::dispose() const
{
    if (object)
        object->dispose();
}

template <class T>
void ObjectPtr<T>::release()
{
    if (object && !borrowed)
        object->releaseRef();
    borrowed = false;
    object = nullptr;
}

template <class T>
T* ObjectPtr<T>::addRefAndReturn() const
{
    if (object)
        object->addRef();
    return object;
}

template <class T>
T** ObjectPtr<T>::addressOf()
{
    return &object;
}

template <class T>             // NOLINT(google-runtime-operator)
T** ObjectPtr<T>::operator&()  // NOLINT(google-runtime-operator)
{
    if (object != nullptr)
    {
        if (!borrowed)
            object->releaseRef();
        object = nullptr;
    }

    return &object;
}

template <class T>
CoreType ObjectPtr<T>::getCoreType() const
{
    if (!object)
        throw InvalidParameterException();

    auto coreTypeObj = asPtrOrNull<ICoreType>(true);
    if (coreTypeObj.assigned())
    {
        CoreType coreType;
        checkErrorInfo(coreTypeObj->getCoreType(&coreType));
        return coreType;
    }

    return ctObject;
}

/// @cond TEMPLATE_SPECIALIZATION
template <>
inline CoreType ObjectPtr<ICoreType>::getCoreType() const
{
    if (!object)
        throw InvalidParameterException();

    CoreType coreType;
    checkErrorInfo(object->getCoreType(&coreType));

    return coreType;
}
/// @endcond

template <typename T>
ObjectPtr<IBaseObject> ObjectPtr<T>::convertTo(CoreType ct) const
{
    auto convObj = asPtr<IConvertible>(true);
    switch (ct)
    {
        case ctBool:
        {
            Bool val;
            checkErrorInfo(convObj->toBool(&val));
            return ObjectPtr<IBaseObject>(val);
        }
        case ctInt:
        {
            Int val;
            checkErrorInfo(convObj->toInt(&val));
            return ObjectPtr<IBaseObject>(val);
        }
        case ctFloat:
        {
            Float val;
            checkErrorInfo(convObj->toFloat(&val));
            return ObjectPtr<IBaseObject>(val);
        }
        case ctRatio:
        {
            Int val;
            checkErrorInfo(convObj->toInt(&val));

            IRatio* ratio;
            auto errCode = createRatio(&ratio, val, 1);
            checkErrorInfo(errCode);

            return ObjectPtr<IBaseObject>(std::move(ratio));
        }
        case ctString:
        {
            CharPtr val;
            checkErrorInfo(convObj->toString(&val));

            Finally final([&val]() {
                if (val != nullptr)
                    daqFreeMemory(val);
            });
            return ObjectPtr<IBaseObject>(val);
        }
        default:
            throw ConversionFailedException();
    }
}

template <class T>
SizeT ObjectPtr<T>::getHashCode() const
{
    if (!object)
    {
        throw InvalidParameterException();
    }

    SizeT hash;
    checkErrorInfo(object->getHashCode(&hash));

    return hash;
}

template <typename T>
ObjectPtr<IString> ObjectPtr<T>::toString() const
{
    if (!object)
    {
        throw InvalidParameterException("Wrapped object must not be null.");
    }

    CharPtr data;
    ErrCode errCode = object->toString(&data);
    checkErrorInfo(errCode);

    ObjectPtr<IString> ptr;
    errCode = createString(&ptr, data);

    daqFreeMemory(data);
    checkErrorInfo(errCode);
    return ptr;
}

template <typename T>
void ObjectPtr<T>::freeze() const
{
    if (!object)
        throw InvalidParameterException();

    auto freezableObj = asPtr<IFreezable>(true);
    checkErrorInfo(freezableObj->freeze());
}

/// @cond TEMPLATE_SPECIALIZATION
template <>
inline void ObjectPtr<IFreezable>::freeze() const
{
    if (!object)
        throw InvalidParameterException();

    checkErrorInfo(object->freeze());
}
/// @endcond

template <typename T>
Bool ObjectPtr<T>::isFrozen() const
{
    if (!object)
        throw InvalidParameterException();

    auto freezableObj = asPtr<IFreezable>(true);

    Bool frozen;
    checkErrorInfo(freezableObj->isFrozen(&frozen));

    return frozen;
}

/// @cond TEMPLATE_SPECIALIZATION
template <>
inline Bool ObjectPtr<IFreezable>::isFrozen() const
{
    if (!object)
        throw InvalidParameterException();

    Bool frozen;
    checkErrorInfo(object->isFrozen(&frozen));

    return frozen;
}
/// @endcond

template <class T>
ConstCharPtr ObjectPtr<T>::getSerializeId() const
{
    if (!object)
        throw InvalidParameterException();

    ISerializable* serializable = as<ISerializable>(true);

    ConstCharPtr id;
    ErrCode errCode = serializable->getSerializeId(&id);
    checkErrorInfo(errCode);

    return id;
}

/// @cond TEMPLATE_SPECIALIZATION
template <>
inline ConstCharPtr ObjectPtr<ISerializable>::getSerializeId() const
{
    if (!object)
        throw InvalidParameterException();

    ConstCharPtr id;
    ErrCode errCode = object->getSerializeId(&id);
    checkErrorInfo(errCode);

    return id;
}
/// @endcond

template <class T>
void ObjectPtr<T>::serialize(const ObjectPtr<ISerializer>& serializer) const
{
    if (!object)
        throw InvalidParameterException();

    ISerializable* serializable = as<ISerializable>(true);

    ErrCode errCode = serializable->serialize(serializer);
    checkErrorInfo(errCode);
}

/// @cond TEMPLATE_SPECIALIZATION
template <>
inline void ObjectPtr<ISerializable>::serialize(const ObjectPtr<ISerializer>& serializer) const
{
    if (!object)
        throw InvalidParameterException();

    checkErrorInfo(object->serialize(serializer));
}
/// @endcond

template <class T>
void ObjectPtr<T>::dispatch(const ObjectPtr<IBaseObject>& params) const
{
    if (!object)
        throw InvalidParameterException();

    auto procObj = asPtr<IProcedure>(true);
    ErrCode err = procObj->dispatch(params);
    checkErrorInfo(err);
}

/// @cond TEMPLATE_SPECIALIZATION
template <>
inline void ObjectPtr<IProcedure>::dispatch(const ObjectPtr<IBaseObject>& params) const
{
    if (!object)
        throw InvalidParameterException();

    ErrCode err = object->dispatch(params);
    checkErrorInfo(err);
}
/// @endcond

template <class T>
void ObjectPtr<T>::dispatch() const
{
    if (!object)
        throw InvalidParameterException();

    auto procObj = asPtr<IProcedure>(true);
    ErrCode err = procObj->dispatch(nullptr);
    checkErrorInfo(err);
}

/// @cond TEMPLATE_SPECIALIZATION
template <>
inline void ObjectPtr<IProcedure>::dispatch() const
{
    if (!object)
        throw InvalidParameterException();

    ErrCode err = object->dispatch(nullptr);
    checkErrorInfo(err);
}
/// @endcond

template <class T>
void ObjectPtr<T>::execute() const
{
    if (!object)
        throw InvalidParameterException();

    dispatch();
}

template <class T>
template <typename... Params>
ObjectPtr<IBaseObject> ObjectPtr<T>::call(Params... params) const
{
    if (!object)
        throw InvalidParameterException();

    ObjectPtr<IBaseObject> args;
    if constexpr (sizeof...(params) > 1)
    {
        ObjectPtr<IBaseObject> arr[] = {params...};

        ObjectPtr<IList> list(List_Create());
        for (size_t i = 0; i < sizeof...(params); i++)
            list->pushBack(arr[i]);

        args = list;
    }
    else if constexpr (sizeof...(params) == 1)
    {
        args = ObjectPtr<IBaseObject>(params...);
    }

    auto funcObj = asPtr<IFunction>(true);

    ObjectPtr<IBaseObject> result;
    ErrCode err = funcObj->call(args, &result);
    checkErrorInfo(err);

    return result;
}

template <typename T>
bool ObjectPtr<T>::isBorrowed() const
{
    return borrowed;
}

template <class T>
template <typename... Params>
void ObjectPtr<T>::execute(Params... params) const
{
    if (!object)
        throw InvalidParameterException();

    ObjectPtr<IBaseObject> arr[] = {params...};
    size_t argCount = sizeof...(params);

    if (argCount > 1)
    {
        ObjectPtr<IList> list(List_Create());
        for (size_t i = 0; i < argCount; i++)
            list->pushBack(arr[i]);

        dispatch(list);
    }
    else
    {
        dispatch(arr[0]);
    }
}

template <typename T>
ErrCode tryFreeze(T* obj)
{
    using Ptr = typename InterfaceToSmartPtr<T>::SmartPtr;

    auto ptr = Ptr::Adopt(obj);
    auto freezable = ptr.template asOrNull<IFreezable>(true);
    ErrCode errCode = freezable->freeze();

    if (OPENDAQ_FAILED(errCode))
        return errCode;

    ptr.detach();
    return errCode;
}

template <class Interface, class Impl, class... Params>
ErrCode createObjectFrozen(Interface** intf, Params... params)
{
    using Ptr = typename InterfaceToSmartPtr<Interface>::SmartPtr;

    Ptr ptr{};
    ErrCode errCode = daq::createObject<Interface, Impl>(&ptr, params...);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    auto freezable = ptr.template asOrNull<IFreezable>(true);
    errCode = freezable->freeze();

    if (OPENDAQ_SUCCEEDED(errCode))
        *intf = ptr.detach();

    return errCode;
}

template <class Intf, class Impl, typename... Params>
typename InterfaceToSmartPtr<Intf>::SmartPtr createWithImplementation(Params&&... params)
{
    static_assert(std::is_base_of_v<Intf, Impl>, "Implementation does not implement the specified interface.");

    using SmartPtr = typename InterfaceToSmartPtr<Intf>::SmartPtr;

    Intf* intf = new Impl(std::forward<Params>(params)...);
    if (static_cast<Impl*>(intf)->getRefAdded())
    {
        return SmartPtr::Adopt(intf);
    }
    return SmartPtr(intf);
}

END_NAMESPACE_OPENDAQ

namespace std
{
    template <class T>
    struct hash<daq::ObjectPtr<T>>
    {
        size_t operator() (const daq::ObjectPtr<T>& m) const { return m.getHashCode(); }
    };

    template <class T>
    struct equal_to<daq::ObjectPtr<T>>
    {
        constexpr bool operator()(const daq::ObjectPtr<T>& lhs, const daq::ObjectPtr<T>& rhs) const
        {
            return lhs == rhs;
        }
    };
}
