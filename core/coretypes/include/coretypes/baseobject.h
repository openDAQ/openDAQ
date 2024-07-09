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
#include <coretypes/common.h>
#include <coretypes/factory.h>

BEGIN_NAMESPACE_OPENDAQ

static constexpr IntfID UnknownGuid = { 0x00000000, 0x0000, 0x0000, { { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } } };

// ReSharper disable CppPolymorphicClassWithNonVirtualPublicDestructor

/*!
 * @addtogroup types_base_concepts
 * @{
 */

/*!
 * Enables clients to get pointers to other interfaces on a given object through the `queryInterface`
 * method, and manage the existence of the object through the `addRef` and `releaseRef` methods. All other
 * interfaces are inherited, directly or indirectly, from IUnknown.
 *
 * IUnknown on Windows OS is compatible with Microsoft COM IUnknown interface.
 */
struct IUnknown
{
    /// @privatesection
    DEFINE_EXTERNAL_INTFID(UnknownGuid)

    using Actual = IUnknown;
    /// @publicsection

    /*!
     * @brief Returns another interface which is supported by the object and increments the reference count.
     * @param intfID Interface ID of requested interface.
     * @param[out] obj Pointer to the new interface.
     * @return OPENDAQ_SUCCESS if succeeded, error code otherwise.
     *
     * Provides a fundamental mechanism by which an object can express the functionality it provides.
     *
     * Every interface is derived from IUnknown, so every interface has an implementation of `queryInterface`.
     * Regardless of the implementation, this method queries an object using the ID of the interface to which
     * the caller wants a pointer. If the object supports that interface, `queryInterface` retrieves a pointer
     * to the interface, while also calling `addRef`. Otherwise, it returns the OPENDAQ_ERR_NOINTERFACE error code.
     *
     * Example:
     * @code
     * IUnknown* obj = ...;
     *
     * IBaseObject* baseObj;
     * auto errCode = obj->queryInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObj)), OPENDAQ_SUCCESS);
     * if (OPENDAQ_FAILED(errCode))
     *     throw SomeException();
     * // ... do something with baseObj
     * baseObj->releaseRef();
     * @endcode
     */
    virtual ErrCode INTERFACE_FUNC queryInterface(const IntfID& intfID, void** obj) = 0;

    /*!
     * @brief Increments the reference count for an interface on an object.
     * @return Reference count after the increment.
     *
     * You should call this method whenever you make a copy of an interface pointer.
     *
     * An object uses a per-interface reference-counting mechanism to ensure that the object does not outlive
     * references to it. You use `addRef` to stabilize a copy of an interface pointer. You can also call it
     * when the life of a cloned pointer must extend beyond the lifetime of the original pointer. The cloned
     * pointer must be released by calling `releaseRef` on it.
     */
    virtual int INTERFACE_FUNC addRef() = 0;

    /*!
     * @brief Decrements the reference count for an interface on an object.
     * @return Reference count after the decrement.
     *
     * Call this method when you no longer need to use an interface pointer.
     *
     * When the reference count on an object reaches zero, `releaseRef` causes the interface pointer to free
     * itself. When the released pointer is the only (formerly) outstanding reference to an object
     * the implementation must free the object.
     */
    virtual int INTERFACE_FUNC releaseRef() = 0;
};

/*!
 * Extends `IUnknown` by providing additional methods for borrowing interfaces, hashing, and equality
 * comparison. All openDAQ objects implement `IBaseObject` interface or its descendants.
 * Hashing and equality comparison provides the ability to use the object as an element in dictionaries
 * and lists. Classes that implement any interface derived from `IBaseObject` should be derived
 * from `ImplementationOf` class, which provides the default implementation of `IBaseObject` interface
 * methods.
 *
 * Available factories:
 * @code
 * // Creates a new BaseObject. Throws exception if not successful.
 * IBaseObject* BaseObject_Create()
 *
 * // Creates a new BaseObject. Returns error code if not successful.
 * ErrCode createBaseObject(IBaseObject** obj)
 * @endcode
 */
struct IBaseObject : public IUnknown
{
    /// @privatesection
    DEFINE_INTFID("IBaseObject")

    using Actual = IBaseObject;
    /// @publicsection

    /*!
     * @brief Returns another interface which is supported by the object without incrementing the reference count.
     * @param intfID Interface ID of requested interface.
     * @param[out] obj Pointer to the new interface.
     * @return OPENDAQ_SUCCESS if succeeded, error code otherwise.
     *
     * This method is similar to `queryInterface`, however, it does not increment the reference count.
     * Use this method if you need to get another interface to the object and the lifetime of the new interface
     * is shorter than the lifetime of the original interface.
     *
     * Typical scenario is when an interface is passed as a parameter to a function and in this function
     * you need another interface to the object, but only within the scope of this function call. The object
     * will not be destroyed before the function is exited, because the caller is responsible for holding
     * the reference to the object.
     *
     * Example:
     * @code
     * IUnknown* obj = ...;
     *
     * IBaseObject* baseObj;
     * auto errCode = obj->borrowInterface(IBaseObject::Id, reinterpret_cast<void**>(&baseObj)), OPENDAQ_SUCCESS);
     * if (OPENDAQ_FAILED(errCode))
     *     throw SomeException();
     * // ... do something with baseObj
     * // do not call baseObj->releaseRef()
     * @endcode
     */
    virtual ErrCode INTERFACE_FUNC borrowInterface(const IntfID& intfID, void** obj) const = 0;

    /*!
     * @brief Disposes all references held by the object.
     * @return OPENDAQ_SUCCESS if succeeded, error code if failed.
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
    virtual ErrCode INTERFACE_FUNC dispose() = 0;

    /*!
     * @brief Returns hash code of the object.
     * @param[out] hashCode Hash code.
     * @return OPENDAQ_SUCCESS if succeeded, error code otherwise.
     *
     * The object should calculate and return the hash code. For reference objects, it is usually
     * calculated from pointer address. For value objects, such as numbers and strings, it is
     * calculated from value.
     *
     * `ImplementationOf` object which provides default implementation of `IBaseObject` uses
     * pointer address to calculate hash code.
     *
     * If the object calculates the hash code from some value stored in the object, it is
     * mandatory to make this value (or the object) immutable. The hash code of the object should
     * not change. If a dictionary stores the object as a key, it will corrupt the hash table.
     */
    virtual ErrCode INTERFACE_FUNC getHashCode(SizeT* hashCode) = 0;

    /*!
     * @brief Compares object to another object for equality.
     * @param other Interface to another object for comparison.
     * @param equal Value indicating if the @c other is equivalent to this one.
     *
     * For reference objects, it compares the address of the object. For value objects, such as numbers and
     * strings, it compares values.
     *
     * `ImplementationOf` which provides default implementation of `IBaseObject` uses
     * pointer address to compare for equality.
     */
    virtual ErrCode INTERFACE_FUNC equals(IBaseObject* other, Bool* equal) const = 0;

    /*!
     * @brief Returns a string representation of the object.
     * @param[out] str String representation of the object.
     * @return OPENDAQ_SUCCESS if succeeded, error code otherwise.
     *
     * Call this method to convert the object to its string representation.
     *
     * `ImplementationOf` object which provides default implementation of `IBaseObject` uses
     * pointer address for string representation.
     *
     * Return value `str` should be de-allocated by the caller using `daqFreeMemory` function.
     */
    virtual ErrCode INTERFACE_FUNC toString(CharPtr* str) = 0;
};

/*!@}*/

// ReSharper restore CppPolymorphicClassWithNonVirtualPublicDestructor

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, BaseObject)

END_NAMESPACE_OPENDAQ
