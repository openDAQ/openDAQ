/*
 * Copyright 2022-2024 openDAQ d.o.o.
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
#include <coreobjects/core_event_args_factory.h>
#include <coreobjects/end_update_event_args_factory.h>
#include <coreobjects/eval_value_ptr.h>
#include <coreobjects/exceptions.h>
#include <coreobjects/object_keys.h>
#include <coreobjects/ownable_ptr.h>
#include <coreobjects/permission_manager_factory.h>
#include <coreobjects/permission_manager_internal_ptr.h>
#include <coreobjects/permission_mask_builder_factory.h>
#include <coreobjects/permissions_builder_factory.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_internal_ptr.h>
#include <coreobjects/property_object.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_internal_ptr.h>
#include <coreobjects/property_object_protected.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/property_ptr.h>
#include <coreobjects/property_value_event_args_factory.h>
#include <coretypes/cloneable.h>
#include <coretypes/coretypes.h>
#include <coretypes/enumeration_factory.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/updatable.h>
#include <coretypes/validation.h>
#include <tsl/ordered_map.h>
#include <map>
#include <thread>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

using PropertyOrderedMap = tsl::ordered_map<StringPtr, PropertyPtr, StringHash, StringEqualTo>;

struct PropertyNameInfo
{
    StringPtr name;
    Int index{};
};

namespace object_utils
{
    struct NullMutex
    {
        void lock() {}
        void unlock() noexcept {}
        bool try_lock() { return true; }
    };
}

class RecursiveConfigLockGuard : public std::enable_shared_from_this<RecursiveConfigLockGuard>
{
public:
    virtual ~RecursiveConfigLockGuard() = default;
};

template <typename TMutex>
class GenericRecursiveConfigLockGuard : public RecursiveConfigLockGuard
{
public:
    GenericRecursiveConfigLockGuard(TMutex* lock, std::thread::id* threadId, int* depth)
        : id(threadId)
        , depth(depth)
        , lock(std::lock_guard(*lock))
    {
        assert(this->id != nullptr);
        assert(this->depth != nullptr);

        *id = std::this_thread::get_id();
        ++(*this->depth);
    }

    ~GenericRecursiveConfigLockGuard() override
    {
        --(*depth);
        if (*depth == 0)
            *id = std::thread::id();
    }

private:
    std::thread::id* id;
    int* depth;
    std::lock_guard<TMutex> lock;
};

template <typename PropObjInterface, typename... Interfaces>
class GenericPropertyObjectImpl : public ImplementationOfWeak<PropObjInterface,
                                                              IOwnable,
                                                              IFreezable,
                                                              ISerializable,
                                                              IUpdatable,
                                                              IPropertyObjectProtected,
                                                              IPropertyObjectInternal,
                                                              Interfaces...>
{
public:
    explicit GenericPropertyObjectImpl();
    explicit GenericPropertyObjectImpl(const TypeManagerPtr& manager,
                                       const StringPtr& className,
                                       const ProcedurePtr& triggerCoreEvent = nullptr);

    virtual ErrCode INTERFACE_FUNC getClassName(IString** className) override;

    virtual ErrCode INTERFACE_FUNC setPropertyValue(IString* propertyName, IBaseObject* value) override;
    virtual ErrCode INTERFACE_FUNC setPropertyValueNoLock(IString* propertyName, IBaseObject* value) override;
    virtual ErrCode INTERFACE_FUNC getPropertyValue(IString* propertyName, IBaseObject** value) override;
    virtual ErrCode INTERFACE_FUNC getPropertyValueNoLock(IString* propertyName, IBaseObject** value) override;
    virtual ErrCode INTERFACE_FUNC getPropertySelectionValue(IString* propertyName, IBaseObject** value) override;
    virtual ErrCode INTERFACE_FUNC getPropertySelectionValueNoLock(IString* propertyName, IBaseObject** value) override;
    virtual ErrCode INTERFACE_FUNC clearPropertyValue(IString* propertyName) override;
    virtual ErrCode INTERFACE_FUNC clearPropertyValueNoLock(IString* propertyName) override;

    virtual ErrCode INTERFACE_FUNC hasProperty(IString* propertyName, Bool* hasProperty) override;
    virtual ErrCode INTERFACE_FUNC getProperty(IString* propertyName, IProperty** property) override;
    virtual ErrCode INTERFACE_FUNC addProperty(IProperty* property) override;
    virtual ErrCode INTERFACE_FUNC removeProperty(IString* propertyName) override;

    virtual ErrCode INTERFACE_FUNC getVisibleProperties(IList** properties) override;
    virtual ErrCode INTERFACE_FUNC getAllProperties(IList** properties) override;

    virtual ErrCode INTERFACE_FUNC setPropertyOrder(IList* orderedPropertyNames) override;

    virtual ErrCode INTERFACE_FUNC getOnPropertyValueWrite(IString* propertyName, IEvent** event) override;
    virtual ErrCode INTERFACE_FUNC getOnPropertyValueRead(IString* propertyName, IEvent** event) override;
    virtual ErrCode INTERFACE_FUNC getOnAnyPropertyValueWrite(IEvent** event) override;
    virtual ErrCode INTERFACE_FUNC getOnAnyPropertyValueRead(IEvent** event) override;

    virtual ErrCode INTERFACE_FUNC beginUpdate() override;
    virtual ErrCode INTERFACE_FUNC endUpdate() override;
    virtual ErrCode INTERFACE_FUNC getUpdating(Bool* updating) override;

    virtual ErrCode INTERFACE_FUNC getOnEndUpdate(IEvent** event) override;
    virtual ErrCode INTERFACE_FUNC getPermissionManager(IPermissionManager** permissionManager) override;

    // IPropertyObjectInternal
    virtual ErrCode INTERFACE_FUNC checkForReferences(IProperty* property, Bool* isReferenced) override;
    virtual ErrCode INTERFACE_FUNC checkForReferencesNoLock(IProperty* property, Bool* isReferenced) override;
    virtual ErrCode INTERFACE_FUNC enableCoreEventTrigger() override;
    virtual ErrCode INTERFACE_FUNC disableCoreEventTrigger() override;
    virtual ErrCode INTERFACE_FUNC getCoreEventTrigger(IProcedure** trigger) override;
    virtual ErrCode INTERFACE_FUNC setCoreEventTrigger(IProcedure* trigger) override;
    virtual ErrCode INTERFACE_FUNC clone(IPropertyObject** cloned) override;
    virtual ErrCode INTERFACE_FUNC setPath(IString* path) override;
    virtual ErrCode INTERFACE_FUNC isUpdating(Bool* updating) override;
    virtual ErrCode INTERFACE_FUNC hasUserReadAccess(IBaseObject* userContext, Bool* hasAccessOut) override;

    // IUpdatable
    virtual ErrCode INTERFACE_FUNC updateInternal(ISerializedObject* obj, IBaseObject* context) override;
    virtual ErrCode INTERFACE_FUNC update(ISerializedObject* obj, IBaseObject* config) override;
    virtual ErrCode INTERFACE_FUNC serializeForUpdate(ISerializer* serializer) override;
    virtual ErrCode INTERFACE_FUNC updateEnded(IBaseObject* context) override;

    // ISerializable
    virtual ErrCode INTERFACE_FUNC serialize(ISerializer* serializer) override;
    virtual ErrCode INTERFACE_FUNC getSerializeId(ConstCharPtr* id) const override;

    virtual ErrCode INTERFACE_FUNC toString(CharPtr* str) override;

    static ConstCharPtr SerializeId();
    static ErrCode Deserialize(ISerializedObject* serialized, IBaseObject* context, IFunction* factoryCallback, IBaseObject** obj);

    // IOwnable
    virtual ErrCode INTERFACE_FUNC setOwner(IPropertyObject* newOwner) override;

    [[nodiscard]]
    WeakRefPtr<IPropertyObject> getOwner() const;

    // IFreezable
    virtual ErrCode INTERFACE_FUNC freeze() override;
    virtual ErrCode INTERFACE_FUNC isFrozen(Bool* isFrozen) const override;

    // IPropertyObjectProtected
    virtual ErrCode INTERFACE_FUNC setProtectedPropertyValue(IString* propertyName, IBaseObject* value) override;
    virtual ErrCode INTERFACE_FUNC clearProtectedPropertyValue(IString* propertyName) override;

    using PropertyValueEventEmitter = EventEmitter<PropertyObjectPtr, PropertyValueEventArgsPtr>;
    using EndUpdateEventEmitter = EventEmitter<PropertyObjectPtr, EndUpdateEventArgsPtr>;

    void configureClonedMembers(const std::unordered_map<StringPtr, PropertyValueEventEmitter>& valueWriteEvents,
                                const std::unordered_map<StringPtr, PropertyValueEventEmitter>& valueReadEvents,
                                const EndUpdateEventEmitter& endUpdateEvent,
                                const ProcedurePtr& triggerCoreEvent,
                                const PropertyOrderedMap& localProperties,
                                const std::unordered_map<StringPtr, BaseObjectPtr, StringHash, StringEqualTo>& propValues,
                                const std::vector<StringPtr>& customOrder,
                                const PermissionManagerPtr& permissionManager);

    friend class AddressInfoImpl;
    friend class ServerCapabilityConfigImpl;

protected:
    struct UpdatingAction
    {
        bool setValue;
        bool protectedAccess;
        BaseObjectPtr value;
    };

    // Using vector to preserve write order when the same property is changed twice within an update
    using UpdatingActions = std::vector<std::pair<std::string, UpdatingAction>>;

    // Gets a lock for the configuration of the object. Can be used to lock the sync mutex in a function
    // that is called during a property value read/write event to prevent deadlocks. The lock behaves
    // similarly to a recursive mutex.
    std::unique_ptr<RecursiveConfigLockGuard> getRecursiveConfigLock();

    // Gets a lock to be used in the data acquisition loop, or in other performance-critical parts of
    // a module implementation. The lock is not recursive in comparison to the config lock and should
    // be used with caution to prevent deadlocks.
    std::lock_guard<std::mutex> getAcquisitionLock();

    std::unique_lock<std::mutex> getUniqueLock();

    bool frozen;
    WeakRefPtr<IPropertyObject> owner;
    std::vector<StringPtr> customOrder;
    PropertyObjectPtr objPtr;
    int updateCount;
    UpdatingActions updatingPropsAndValues;
    bool coreEventMuted;
    WeakRefPtr<ITypeManager> manager;
    PropertyOrderedMap localProperties;
    StringPtr path;
    PermissionManagerPtr permissionManager;

    void internalDispose(bool) override;
    ErrCode setPropertyValueInternal(IString* name, IBaseObject* value, bool triggerEvent, bool protectedAccess, bool batch, bool isUpdating = false);
    ErrCode clearPropertyValueInternal(IString* name, bool protectedAccess, bool batch, bool isUpdating = false);
    ErrCode getPropertyValueInternal(IString* propertyName, IBaseObject** value);
    ErrCode getPropertySelectionValueInternal(IString* propertyName, IBaseObject** value);
    ErrCode checkForReferencesInternal(IProperty* property, Bool* isReferenced);

    // Serialization

    ErrCode serializePropertyValues(ISerializer* serializer);
    ErrCode serializeLocalProperties(ISerializer* serializer);

    virtual ErrCode serializeCustomValues(ISerializer* serializer, bool forUpdate);
    virtual ErrCode serializePropertyValue(const StringPtr& name, const ObjectPtr<IBaseObject>& value, ISerializer* serializer);

    static void DeserializePropertyValues(const SerializedObjectPtr& serialized,
                                          const BaseObjectPtr& context,
                                          const FunctionPtr& factoryCallback,
                                          PropertyObjectPtr& propObjPtr);

    static void DeserializeLocalProperties(const SerializedObjectPtr& serialized,
                                           const BaseObjectPtr& context,
                                           const FunctionPtr& factoryCallback,
                                           PropertyObjectPtr& propObjPtr);

    // Child property handling - Used when a property is queried in the "parent.child" format
    bool isChildProperty(const StringPtr& name, StringPtr& childName, StringPtr& subName) const;

    // Update

    ErrCode setPropertyFromSerialized(const StringPtr& propName,
                                      const PropertyObjectPtr& propObj,
                                      const SerializedObjectPtr& serialized);

    virtual void endApplyUpdate();
    virtual void beginApplyUpdate();
    virtual void beginApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating);
    virtual void endApplyProperties(const UpdatingActions& propsAndValues, bool parentUpdating);
    bool isParentUpdating();
    virtual void onUpdatableUpdateEnd(const BaseObjectPtr& context);

    template <class F>
    static PropertyObjectPtr DeserializePropertyObject(const SerializedObjectPtr& serialized,
                                                       const BaseObjectPtr& context,
                                                       const FunctionPtr& factoryCallback,
                                                       F&& f);

    // Does not bind property to object and does not look up reference property
    PropertyPtr getUnboundProperty(const StringPtr& name);
    PropertyPtr getUnboundPropertyOrNull(const StringPtr& name) const;

    virtual void callBeginUpdateOnChildren();
    virtual void callEndUpdateOnChildren();

    virtual PropertyObjectPtr getPropertyObjectParent();

    // Adds the value to the local list of values (`propValues`)
    bool writeLocalValue(const StringPtr& name, const BaseObjectPtr& value);
    virtual void cloneAndSetChildPropertyObject(const PropertyPtr& prop);
    void configureClonedObj(const StringPtr& objPropName, const PropertyObjectPtr& obj);

    ErrCode beginUpdateInternal(bool deep);
    ErrCode endUpdateInternal(bool deep);
    ErrCode getUpdatingInternal(Bool* updating);

private:
    // Mutex that is locked in the getRecursiveConfigLock and getAcquisitionLock methods. Those should
    // be used instead of locking this mutex directly unless a different type of lock is needed.
    std::mutex sync;

    StringPtr className;
    PropertyObjectClassPtr objectClass;
    
    const std::string AnyReadEventName = "DAQ_AnyReadEvent";
    const std::string AnyWriteEventName = "DAQ_AnyWriteEvent";

    std::unordered_map<StringPtr, PropertyValueEventEmitter> valueWriteEvents;
    std::unordered_map<StringPtr, PropertyValueEventEmitter> valueReadEvents;
    EndUpdateEventEmitter endUpdateEvent;
    ProcedurePtr triggerCoreEvent;

    object_utils::NullMutex nullSync;
    std::thread::id externalCallThreadId{};
    int externalCallDepth = 0;

    std::unordered_map<StringPtr, BaseObjectPtr, StringHash, StringEqualTo> propValues;

    void triggerCoreEventInternal(const CoreEventArgsPtr& args);

    // Gets the property, as well as its value. Gets the referenced property, if the property is a refProp
    ErrCode getPropertyAndValueInternal(const StringPtr& name, BaseObjectPtr& value, PropertyPtr& property, bool triggerEvent = true);
    ErrCode getPropertiesInternal(Bool includeInvisible, Bool bind, IList** list);

    // Gets the property value, if stored in local value dictionary (propValues)
    // Parses brackets, if the property is a list
    ErrCode readLocalValue(const StringPtr& name, BaseObjectPtr& value) const;
    PropertyNameInfo getPropertyNameInfo(const StringPtr& name) const;

    // Checks if the value is a container type, or base `IPropertyObject`. Only such values can be set in `setProperty`
    ErrCode checkContainerType(const PropertyPtr& prop, const BaseObjectPtr& value);

    // Checks if the property is a struct type, and checks its fields for type/name compatibility
    ErrCode checkStructType(const PropertyPtr& prop, const BaseObjectPtr& value);

    // Checks if the property is a enumeration type and checks for type/name compatibility
    ErrCode checkEnumerationType(const PropertyPtr& prop, const BaseObjectPtr& value);

    // Checks if value is a correct key into the list/dictionary of selection values
    ErrCode checkSelectionValues(const PropertyPtr& prop, const BaseObjectPtr& value);

    // Called when `setPropertyValue` successfully sets a new value
    [[maybe_unused]]
    BaseObjectPtr callPropertyValueWrite(const PropertyPtr& prop,
                                         const BaseObjectPtr& newValue,
                                         PropertyEventType changeType,
                                         bool isUpdating);

    // Called at the end of `getPropertyValue`
    BaseObjectPtr callPropertyValueRead(const PropertyPtr& prop, const BaseObjectPtr& readValue);

    // Checks if property and value type match. If not, attempts to convert the value
    ErrCode checkPropertyTypeAndConvert(const PropertyPtr& prop, BaseObjectPtr& value);

    // Sets `this` as owner of `value`, if `value` is ownable
    void setOwnerToPropertyValue(const BaseObjectPtr& value);

    // Gets the index integer value between two square brackets
    int parseIndex(char const* lBracket) const;

    // Gets the property name without the index as the `propName` output parameter
    // Returns the index in the form of [index], eg. [0]
    ConstCharPtr getPropNameWithoutIndex(const StringPtr& name, StringPtr& propName) const;

    // Child property handling - Used when a property is queried in the "parent.child" format
    ErrCode getChildPropertyValue(const StringPtr& childName, const StringPtr& subName, BaseObjectPtr& value);

    PropertyPtr checkForRefPropAndGetBoundProp(PropertyPtr& prop, bool* isReferenced = nullptr) const;

    // Checks whether the property is a reference property that references an already referenced property
    bool hasDuplicateReferences(const PropertyPtr& prop);

    // Coercion/Validation
    void coercePropertyWrite(const PropertyPtr& prop, ObjectPtr<IBaseObject>& valuePtr) const;
    void validatePropertyWrite(const PropertyPtr& prop, ObjectPtr<IBaseObject>& valuePtr) const;
    void coerceMinMax(const PropertyPtr& prop, ObjectPtr<IBaseObject>& valuePtr);
    Bool checkIsReferenced(const StringPtr& referencedPropName, const PropertyInternalPtr& prop);

    // update
    ErrCode updateObjectProperties(const PropertyObjectPtr& propObj,
                                   const SerializedObjectPtr& serialized,
                                   const ListPtr<IProperty>& props);

    bool hasUserReadAccess(const BaseObjectPtr& userContext, const BaseObjectPtr& obj);
};

using PropertyObjectImpl = GenericPropertyObjectImpl<IPropertyObject>;

template <class PropObjInterface, class... Interfaces>
GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::GenericPropertyObjectImpl()
    : frozen(false)
    , updateCount(0)
    , coreEventMuted(true)
    , path("")
    , permissionManager(PermissionManager())
    , className(nullptr)
    , objectClass(nullptr)
{
    this->internalAddRef();
    objPtr = this->template borrowPtr<PropertyObjectPtr>();

    this->permissionManager.setPermissions(
        PermissionsBuilder().assign("everyone", PermissionMaskBuilder().read().write().execute()).build());

    PropertyValueEventEmitter readEmitter;
    PropertyValueEventEmitter writeEmitter;
    valueReadEvents.emplace(AnyReadEventName, readEmitter);
    valueWriteEvents.emplace(AnyWriteEventName, writeEmitter);
}

template <typename PropObjInterface, typename... Interfaces>
GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::GenericPropertyObjectImpl(const TypeManagerPtr& manager,
                                                                                      const StringPtr& className,
                                                                                      const ProcedurePtr& triggerCoreEvent)
    : GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::GenericPropertyObjectImpl()
{
    this->triggerCoreEvent = triggerCoreEvent;
    this->manager = manager;

    if (className.assigned() && className != "")
    {
        this->className = className;
        if (!manager.assigned())
            throw ManagerNotAssignedException{};

        const TypePtr type = manager.getType(className);

        if (!type.assigned())
            throw NotFoundException{"Class with name {} is not available in module manager", className};

        const auto objClass = type.asPtrOrNull<IPropertyObjectClass>();
        if (!objClass.assigned())
            throw InvalidTypeException{"Type with name {} is not a property object class", className};

        objectClass = objClass;

        for (const auto& prop : objectClass.getProperties(true))
            cloneAndSetChildPropertyObject(prop);
    }
}

template <class PropObjInterface, class... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::internalDispose(bool)
{
    for (auto& item : propValues)
    {
        if (item.second.assigned())
        {
            OwnablePtr ownablePtr = item.second.template asPtrOrNull<IOwnable>(true);
            if (ownablePtr.assigned())
                ownablePtr.setOwner(nullptr);
        }
    }
    propValues.clear();

    owner.release();
    className.release();
    objectClass.release();
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getClassName(IString** className)
{
    if (className == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (this->className.assigned())
    {
        *className = this->className.addRefAndReturn();
    }
    else
    {
        *className = String("").detach();
    }

    return OPENDAQ_SUCCESS;
}

#if defined(__GNUC__) && __GNUC__ >= 12
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif

template <class PropObjInterface, class... Interfaces>
bool GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::isChildProperty(const StringPtr& name,
                                                                                 StringPtr& childName,
                                                                                 StringPtr& subName) const
{
    auto strName = name.getCharPtr();
    auto propName = strchr(strName, '.');
    if (propName != nullptr)
    {
        childName = String(strName, propName - strName);
        subName = String(propName + 1);
        return true;
    }

    return false;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getChildPropertyValue(const StringPtr& childName,
                                                                                          const StringPtr& subName,
                                                                                          BaseObjectPtr& value)
{
    PropertyPtr prop;
    StringPtr name;

    auto err = daqTry([&]() -> auto {
        prop = getUnboundProperty(childName);

        prop = checkForRefPropAndGetBoundProp(prop);
        name = prop.getName();
        return OPENDAQ_SUCCESS;
    });

    if (OPENDAQ_FAILED(err))
    {
        return err;
    }

    if (!prop.assigned())
    {
        return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property "{}" does not exist)", name));
    }

    BaseObjectPtr childProp;
    err = getPropertyValueInternal(name, &childProp);
    if (OPENDAQ_FAILED(err))
    {
        return err;
    }

    return daqTry([&]() -> auto {
        const auto childPropAsPropertyObject = childProp.template asPtr<IPropertyObject, PropertyObjectPtr>(true);
        value = childPropAsPropertyObject.getPropertyValue(subName);
        return OPENDAQ_SUCCESS;
    });
}

#if defined(__GNUC__) && __GNUC__ >= 12
    #pragma GCC diagnostic pop
#endif

template <class PropObjInterface, class... Interfaces>
BaseObjectPtr GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::callPropertyValueWrite(const PropertyPtr& prop,
                                                                                                 const BaseObjectPtr& newValue,
                                                                                                 PropertyEventType changeType,
                                                                                                 bool isUpdating)
{
    if (!prop.assigned())
    {
        return newValue;
    }

    auto args = PropertyValueEventArgs(prop, newValue, changeType, isUpdating);

    if (!localProperties.count(prop.getName()))
    {
        const PropertyValueEventEmitter propEvent{prop.asPtr<IPropertyInternal>().getClassOnPropertyValueWrite()};
        if (propEvent.hasListeners())
        {
            propEvent(objPtr, args);
        }
    }

    const auto name = prop.getName();
    if (valueWriteEvents.find(name) != valueWriteEvents.end())
    {
        if (valueWriteEvents[name].hasListeners())
        {
            valueWriteEvents[name](objPtr, args);
        }
    }

    if (valueWriteEvents[AnyWriteEventName].hasListeners())
    {
        valueWriteEvents[AnyWriteEventName](objPtr, args);
    }

    const auto argsValue = args.getValue();
    if (argsValue != newValue)
    {
        setPropertyValueInternal(name, argsValue, false, true, false);
    }

    BaseObjectPtr valuePtr;
    PropertyPtr propPtr;
    getPropertyAndValueInternal(name, valuePtr, propPtr, false);
    return valuePtr;
}

template <typename PropObjInterface, typename... Interfaces>
BaseObjectPtr GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::callPropertyValueRead(const PropertyPtr& prop,
                                                                                                const BaseObjectPtr& readValue)
{
    if (!prop.assigned())
    {
        return readValue;
    }

    auto args = PropertyValueEventArgs(prop, readValue, PropertyEventType::Read, False);

    if (!localProperties.count(prop.getName()))
    {
        const PropertyValueEventEmitter propEvent{prop.asPtr<IPropertyInternal>().getClassOnPropertyValueRead()};
        if (propEvent.hasListeners())
        {
            propEvent(objPtr, args);
        }
    }

    const auto name = prop.getName();
    if (valueReadEvents.find(name) != valueReadEvents.end())
    {
        if (valueReadEvents[name].hasListeners())
        {
            valueReadEvents[name](objPtr, args);
        }
    }

    if (valueReadEvents[AnyReadEventName].hasListeners())
    {
        valueReadEvents[AnyReadEventName](objPtr, args);
    }

    return args.getValue();
}

template <class PropObjInterface, class... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::coercePropertyWrite(const PropertyPtr& prop,
                                                                                     ObjectPtr<IBaseObject>& valuePtr) const
{
    if (prop.assigned() && valuePtr.assigned())
    {
        const auto coercer = prop.asPtr<IPropertyInternal>().getCoercerNoLock();
        if (coercer.assigned())
        {
            try
            {
                valuePtr = coercer.coerceNoLock(objPtr, valuePtr);
            }
            catch (const DaqException&)
            {
                throw;
            }
            catch (...)
            {
                throw CoerceFailedException{};
            }
        }
    }
}

template <class PropObjInterface, class... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::validatePropertyWrite(const PropertyPtr& prop,
                                                                                       ObjectPtr<IBaseObject>& valuePtr) const
{
    if (prop.assigned() && valuePtr.assigned())
    {
        const auto validator = prop.asPtr<IPropertyInternal>().getValidatorNoLock();
        if (validator.assigned())
        {
            try
            {
                validator.validateNoLock(objPtr, valuePtr);
            }
            catch (const DaqException&)
            {
                throw;
            }
            catch (...)
            {
                throw ValidateFailedException{};
            }
        }
    }
}

template <class PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::coerceMinMax(const PropertyPtr& prop, ObjectPtr<IBaseObject>& valuePtr)
{
    if (!prop.assigned() || !valuePtr.assigned())
        return;

    const auto propInternal = prop.asPtr<IPropertyInternal>();
    const auto min = propInternal.getMinValueNoLock();
    if (min.assigned())
    {
        try
        {
            if (valuePtr < min)
                valuePtr = min;
        }
        catch (...)
        {
        }
    }

    const auto max = propInternal.getMaxValueNoLock();
    if (max.assigned())
    {
        try
        {
            if (valuePtr > max)
                valuePtr = max;
        }
        catch (...)
        {
        }
    }
}

template <class PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setProtectedPropertyValue(IString* propertyName, IBaseObject* value)
{
    auto lock = getRecursiveConfigLock();
    return setPropertyValueInternal(propertyName, value, true, true, updateCount > 0);
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setPropertyValue(IString* propertyName, IBaseObject* value)
{
    auto lock = getRecursiveConfigLock();
    return setPropertyValueInternal(propertyName, value, true, false, updateCount > 0);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setPropertyValueNoLock(IString* propertyName, IBaseObject* value)
{
    return setPropertyValueInternal(propertyName, value, true, false, updateCount > 0);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkContainerType(const PropertyPtr& prop, const BaseObjectPtr& value)
{
    if (!value.assigned())
        return OPENDAQ_SUCCESS;

    auto coreType = value.getCoreType();
    if (coreType == ctObject)
    {
        auto inspect = value.asPtrOrNull<IInspectable>();
        if (inspect.assigned() && !inspect.getInterfaceIds().empty())
        {
            return inspect.getInterfaceIds()[0] == IPropertyObject::Id;
        }

        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDTYPE, "Only base Property Object object-type values are allowed");
    }

    auto iterate = [this](const IterablePtr<IBaseObject>& it, CoreType type) {
        for (const auto& key : it)
        {
            auto ct = key.getCoreType();

            if (ct != type)
            {
                return false;
            }

            if (ct == ctObject)
            {
                auto inspect = key.asPtrOrNull<IInspectable>();
                if (inspect.assigned() && !inspect.getInterfaceIds().empty())
                {
                    return inspect.getInterfaceIds()[0] == IPropertyObject::Id;
                }
            }
        }
        return true;
    };

    const auto propInternal = prop.asPtr<IPropertyInternal>();
    if (coreType == ctDict)
    {
        const auto dict = value.asPtr<IDict>();
        const auto keyType = propInternal.getKeyTypeNoLock();
        const auto itemType = propInternal.getItemTypeNoLock();

        IterablePtr<IBaseObject> it;
        dict->getKeys(&it);
        if (!iterate(it, keyType))
        {
            return this->makeErrorInfo(OPENDAQ_ERR_INVALIDTYPE, "Invalid dictionary key type");
        }

        dict->getValues(&it);
        if (!iterate(it, itemType))
        {
            return this->makeErrorInfo(OPENDAQ_ERR_INVALIDTYPE, "Invalid dictionary item type");
        }
    }

    if (coreType == ctList)
    {
        const auto itemType = propInternal.getItemTypeNoLock();

        if (itemType != ctUndefined && !iterate(value, itemType))
        {
            return this->makeErrorInfo(OPENDAQ_ERR_INVALIDTYPE, "Invalid list item type");
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkStructType(const PropertyPtr& prop, const BaseObjectPtr& value)
{
    if (prop.getValueType() != ctStruct)
        return OPENDAQ_SUCCESS;

    auto structPtr = value.asPtrOrNull<IStruct>();
    if (!structPtr.assigned())
        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Set value is not a struct");

    StructTypePtr structType = prop.asPtr<IPropertyInternal>().getStructTypeNoLock();
    StructTypePtr valueStructType = structPtr.getStructType();

    if (structType != valueStructType)
        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Set value StructureType is different from the default.");

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkEnumerationType(const PropertyPtr& prop,
                                                                                         const BaseObjectPtr& value)
{
    const auto propInternal = prop.asPtr<IPropertyInternal>();
    if (propInternal.getValueTypeNoLock() != ctEnumeration)
        return OPENDAQ_SUCCESS;

    auto enumerationPtr = value.asPtrOrNull<IEnumeration>();
    if (!enumerationPtr.assigned())
        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Set value is not an enumeration");

    auto propEnumerationPtr = propInternal.getDefaultValueNoLock().asPtrOrNull<IEnumeration>();
    if (!propEnumerationPtr.assigned())
        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Property default value is not an enumeration");

    EnumerationTypePtr valueEnumerationType = enumerationPtr.getEnumerationType();
    EnumerationTypePtr propEnumerationType = propEnumerationPtr.getEnumerationType();

    if (propEnumerationType != valueEnumerationType)
        return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE, "Set value EnumerationType is different from the default.");

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkSelectionValues(const PropertyPtr& prop,
                                                                                         const BaseObjectPtr& value)
{
    const auto selectionValues = prop.asPtr<IPropertyInternal>().getSelectionValuesNoLock();
    if (selectionValues.assigned())
    {
        const SizeT key = value;
        const auto list = selectionValues.asPtrOrNull<IList>();
        if (list.assigned() && key < list.getCount())
            return OPENDAQ_SUCCESS;

        const auto dict = selectionValues.asPtrOrNull<IDict>();
        if (dict.assigned() && dict.hasKey(value))
            return OPENDAQ_SUCCESS;

        return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, "Value is not a key/index of selection values.");
    }

    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setPropertyValueInternal(IString* name,
                                                                                             IBaseObject* value,
                                                                                             bool triggerEvent,
                                                                                             bool protectedAccess,
                                                                                             bool batch,
                                                                                             bool isUpdating)
{
    if (name == nullptr || value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    try
    {
        auto propName = StringPtr::Borrow(name);
        auto valuePtr = BaseObjectPtr::Borrow(value);

        if (batch)
        {
            updatingPropsAndValues.emplace_back(std::make_pair(propName, UpdatingAction{true, protectedAccess, valuePtr}));
            return OPENDAQ_SUCCESS;
        }

        StringPtr childName;
        StringPtr subName;
        const auto isChildProp = isChildProperty(propName, childName, subName);
        if (isChildProp)
        {
            propName = childName;
        }

        PropertyPtr prop = getUnboundProperty(propName);
        prop = checkForRefPropAndGetBoundProp(prop);

        if (!prop.assigned())
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property "{}" not found.)", propName));
        }

        propName = prop.getName();
        const auto propInternal = prop.asPtr<IPropertyInternal>();

        if (!protectedAccess)
        {
            if (propInternal.getReadOnlyNoLock() && !isChildProp)
            {
                return OPENDAQ_ERR_ACCESSDENIED;
            }
        }

        if (isChildProp)
        {
            BaseObjectPtr childProp;
            const ErrCode err = getPropertyValueInternal(propName, &childProp);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            if (protectedAccess)
            {
                const auto childPropAsPropertyObject = childProp.template asPtr<IPropertyObjectProtected>(true);
                childPropAsPropertyObject.setProtectedPropertyValue(subName, valuePtr);
            }
            else
            {
                const auto childPropAsPropertyObject = childProp.template asPtr<IPropertyObject, PropertyObjectPtr>(true);
                childPropAsPropertyObject.setPropertyValue(subName, valuePtr);
            }
        }
        else
        {
            // TODO: If function type, check if return value is correct type.
            if (!protectedAccess)
            {
                if (propInternal.getReadOnlyNoLock() || propInternal.getValueTypeNoLock() == ctObject)
                {
                    return OPENDAQ_ERR_ACCESSDENIED;
                }
            }

            ErrCode err = checkPropertyTypeAndConvert(prop, valuePtr);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            err = checkContainerType(prop, valuePtr);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            err = checkSelectionValues(prop, valuePtr);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            err = checkStructType(prop, valuePtr);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            err = checkEnumerationType(prop, valuePtr);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            coercePropertyWrite(prop, valuePtr);
            validatePropertyWrite(prop, valuePtr);
            coerceMinMax(prop, valuePtr);

            const auto ct = propInternal.getValueTypeNoLock();
            if (ct == ctList || ct == ctDict)
            {
                BaseObjectPtr clonedValue;
                err = valuePtr.asPtr<ICloneable>()->clone(&clonedValue);
                if (OPENDAQ_FAILED(err))
                    return err;

                valuePtr = clonedValue.detach();
            }
            else if (ct == ctObject)
            {
                configureClonedObj(propName, valuePtr);
            }

            if (!writeLocalValue(propName, valuePtr))
                return OPENDAQ_IGNORED;

            setOwnerToPropertyValue(valuePtr);
            if (triggerEvent)
            {
                const auto newVal = callPropertyValueWrite(prop, valuePtr, PropertyEventType::Update, isUpdating);
                if (!isUpdating)
                    triggerCoreEventInternal(CoreEventArgsPropertyValueChanged(objPtr, propName, newVal, path));
            }
        }

        return OPENDAQ_SUCCESS;
    }
    catch (const DaqException& e)
    {
        return this->makeErrorInfo(e.getErrCode(), e.what());
    }
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkPropertyTypeAndConvert(const PropertyPtr& prop,
                                                                                                BaseObjectPtr& value)
{
    if (!prop.assigned() || !value.assigned())
        return OPENDAQ_SUCCESS;

    if (value.supportsInterface<IEvalValue>())
    {
        return OPENDAQ_SUCCESS;
    }

    try
    {
        const auto propInternal = prop.asPtr<IPropertyInternal>();
        const auto propCoreType = propInternal.getValueTypeNoLock();
        const auto valueCoreType = value.getCoreType();

        if (propCoreType != valueCoreType)
        {
            if (propCoreType == ctEnumeration)
            {
                const auto enumVal = propInternal.getDefaultValueNoLock().asPtrOrNull<IEnumeration>();
                if (!enumVal.assigned())
                    return this->makeErrorInfo(OPENDAQ_ERR_INVALIDSTATE,
                                               fmt::format(R"(Default value of enumeration property {} is not assigned)", prop.getName()));

                const auto type = enumVal.getEnumerationType();
                const Int intVal = value.convertTo(ctInt);
                value = EnumerationWithIntValueAndType(type, intVal);
            }
            else
                value = value.convertTo(propCoreType);
        }
    }
    catch (const DaqException& e)
    {
        return this->makeErrorInfo(e.getErrCode(), "Value type is different than Property type and conversion failed");
    }

    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
bool GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::writeLocalValue(const StringPtr& name, const BaseObjectPtr& value)
{
    auto it = propValues.find(name);
    if (it != propValues.end())
    {
        if (it->second == value)
            return false;
        it->second = value;
    }
    else
    {
        bool shouldWrite = true;
        try
        {
            shouldWrite = objPtr.getProperty(name).template asPtr<IPropertyInternal>().getDefaultValueNoLock() != value;
        }
        catch (...)
        {
        }

        if (shouldWrite)
            propValues.emplace(name, value);
        else
            return false;
    }

    return true;
}

template <class PropObjInterface, class... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setOwnerToPropertyValue(const BaseObjectPtr& value)
{
    if (!value.assigned())
        return;

    auto ownablePtr = value.asPtrOrNull<IOwnable>(true);
    if (ownablePtr.assigned())
    {
        try
        {
            ownablePtr.setOwner(this->template borrowThis<GenericPropertyObjectPtr, IPropertyObject>());
        }
        catch (const DaqException& e)
        {
            this->makeErrorInfo(e.getErrCode(), "Failed to set owner to property value");
            throw;
        }
    }
}

template <class PropObjInterface, class... Interfaces>
PropertyPtr GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getUnboundProperty(const StringPtr& name)
{
    const auto res = localProperties.find(name);
    if (res == localProperties.end())
    {
        if (objectClass == nullptr)
            throw NotFoundException(fmt::format(R"(Property with name {} does not exist.)", name));

        return objectClass.getProperty(name);
    }

    return res->second;
}

template <class PropObjInterface, class... Interfaces>
PropertyPtr GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getUnboundPropertyOrNull(const StringPtr& name) const
{
    const auto res = localProperties.find(name);
    if (res != localProperties.cend())
        return res->second;

    if (objectClass == nullptr)
        return nullptr;

    PropertyPtr property;
    const auto errCode = objectClass->getProperty(name, &property);
    if (errCode == OPENDAQ_ERR_NOTFOUND)
    {
        this->clearErrorInfo();
        return nullptr;
    }

    checkErrorInfo(errCode);
    return property;
}

template <class PropObjInterface, class... Interfaces>
PropertyPtr GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkForRefPropAndGetBoundProp(PropertyPtr& prop,
                                                                                                       bool* isReferenced) const
{
    if (!prop.assigned())
    {
        return prop;
    }

    PropertyInternalPtr boundProp = prop.asPtr<IPropertyInternal>().cloneWithOwner(objPtr);
    auto refProp = boundProp.getReferencedPropertyNoLock();
    if (refProp.assigned())
    {
        CoreType ct = refProp.getCoreType();

        if (ct != ctObject)
            throw std::invalid_argument("Invalid reference to property");

        if (isReferenced)
            *isReferenced = true;

        return checkForRefPropAndGetBoundProp(refProp);
    }

    if (isReferenced)
        *isReferenced = false;
    return boundProp;
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::cloneAndSetChildPropertyObject(const PropertyPtr& prop)
{
    const auto propPtrInternal = prop.asPtr<IPropertyInternal>();
    if (propPtrInternal.assigned() && propPtrInternal.getValueTypeUnresolved() == ctObject && prop.getDefaultValue().assigned())
    {
        const auto defaultValue = prop.getDefaultValue();
        const auto inspect = defaultValue.asPtrOrNull<IInspectable>();
        if (inspect.assigned() && !inspect.getInterfaceIds().empty() && !(inspect.getInterfaceIds()[0] == IPropertyObject::Id))
            throw InvalidTypeException{"Only base Property Object object-type values are allowed"};

        const auto propName = prop.getName();
        const auto cloneable = defaultValue.asPtrOrNull<IPropertyObjectInternal>();

        if (!cloneable.assigned())
            return;

        const PropertyObjectPtr cloned = cloneable.clone();
        writeLocalValue(propName, cloned);
        configureClonedObj(propName, cloned);
    }
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::configureClonedObj(const StringPtr& objPropName,
                                                                                    const PropertyObjectPtr& obj)
{
    obj.getPermissionManager().asPtr<IPermissionManagerInternal>().setParent(permissionManager);

    const auto objInternal = obj.asPtrOrNull<IPropertyObjectInternal>();
    if (!coreEventMuted && objInternal.assigned())
    {
        const auto childPath = path != "" ? path + "." + objPropName : objPropName;
        objInternal.setPath(childPath);
        objInternal.setCoreEventTrigger(triggerCoreEvent);
        objInternal.enableCoreEventTrigger();
    }
}

template <typename PropObjInterface, typename... Interfaces>
bool GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::hasDuplicateReferences(const PropertyPtr& prop)
{
    auto refEval = prop.asPtr<IPropertyInternal>().getReferencedPropertyUnresolved();
    if (refEval.assigned())
    {
        auto refNames = refEval.getPropertyReferences();
        for (auto refPropName : refNames)
        {
            if (objPtr.hasProperty(refPropName) && objPtr.getProperty(refPropName).getIsReferenced())
                return true;
        }
    }

    return false;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::readLocalValue(const StringPtr& name, BaseObjectPtr& value) const
{
    PropertyNameInfo info = getPropertyNameInfo(name);

    const auto it = propValues.find(info.name);
    if (it != propValues.cend())
    {
        if (info.index != -1)
        {
            if (it->second.getCoreType() != ctList)
            {
                return this->makeErrorInfo(OPENDAQ_ERR_INVALIDPARAMETER, "Could not access the index as the value is not a list.");
            }

            ListPtr<IBaseObject> list = it->second;
            if (info.index >= (int) list.getCount())
            {
                return this->makeErrorInfo(OPENDAQ_ERR_OUTOFRANGE, "The index parameter is out of bounds of the list.");
            }
            value = list[std::size_t(info.index)];
        }
        else
        {
            value = it->second;
        }
        return OPENDAQ_SUCCESS;
    }

    return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property value "{}" not found)", name));
}

template <class PropObjInterface, class... Interfaces>
int GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::parseIndex(char const* lBracket) const
{
    auto last = strchr(lBracket, ']');
    if (last != nullptr)
    {
        char* end;
        int index = strtol(lBracket + 1, &end, 10);

        if (end != last)
        {
            throw InvalidParameterException("Could not parse the property index.");
        }

        return index;
    }
    throw InvalidParameterException("No matching ] found.");
}

#if defined(__GNUC__) && __GNUC__ >= 12
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif

template <class PropObjInterface, class... Interfaces>
PropertyNameInfo GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertyNameInfo(const StringPtr& name) const
{
    PropertyNameInfo nameInfo;

    auto propNameData = name.getCharPtr();
    auto first = strchr(propNameData, '[');
    if (first != nullptr)
    {
        nameInfo.index = parseIndex(first);
        nameInfo.name = String(propNameData, first - propNameData);
    }
    else
    {
        nameInfo.index = -1;
        nameInfo.name = name;
    }

    return nameInfo;
}

template <class PropObjInterface, class... Interfaces>
ConstCharPtr GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropNameWithoutIndex(const StringPtr& name,
                                                                                                 StringPtr& propName) const
{
    auto propNameData = name.getCharPtr();
    auto first = strchr(propNameData, '[');

    if (first == nullptr)
    {
        propName = String(propNameData);
    }
    else
    {
        propName = String(propNameData, first - propNameData);
    }
    return first;
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::triggerCoreEventInternal(const CoreEventArgsPtr& args)
{
    if (!coreEventMuted && triggerCoreEvent.assigned())
        triggerCoreEvent(args);
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertyAndValueInternal(const StringPtr& name,
                                                                                                BaseObjectPtr& value,
                                                                                                PropertyPtr& property,
                                                                                                bool triggerEvent)
{
    StringPtr propName;
    ConstCharPtr bracket = getPropNameWithoutIndex(name, propName);

    property = getUnboundPropertyOrNull(propName);

    if (!property.assigned())
    {
        return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property "{}" does not exist)", propName));
    }

    bool isRef;
    property = checkForRefPropAndGetBoundProp(property, &isRef);

    // TODO: Extract this to own function
    if (bracket != nullptr)
    {
        if (isRef)
        {
            propName = property.getName() + std::string(bracket);
        }
        else
        {
            propName = name;
        }
    }
    else if (isRef)
    {
        propName = property.getName();
    }

    ErrCode res = readLocalValue(propName, value);

    if (res != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(res))
    {
        return res;
    }
    daqClearErrorInfo();

    if (res == OPENDAQ_ERR_NOTFOUND)
    {
        this->clearErrorInfo();
        const auto propInternal = property.asPtr<IPropertyInternal>();
        res = propInternal->getDefaultValueNoLock(&value);

        if (OPENDAQ_FAILED(res) || !value.assigned())
        {
            value = nullptr;
            daqClearErrorInfo();
            return OPENDAQ_SUCCESS;
        }

        CoreType coreType = value.getCoreType();
        if (coreType == ctList && bracket != nullptr)
        {
            int index = parseIndex(bracket);
            ListPtr<IBaseObject> list = value;
            if (index >= static_cast<int>(list.getCount()))
            {
                return this->makeErrorInfo(OPENDAQ_ERR_OUTOFRANGE, "The index parameter is out of bounds of the list.");
            }
            value = list[std::size_t(index)];
        }
    }

    CoreType coreType = value.getCoreType();
    if (coreType == ctList || coreType == ctDict)
    {
        BaseObjectPtr clonedValue;
        value.asPtr<ICloneable>()->clone(&clonedValue);
        value = clonedValue.detach();
    }

    if (triggerEvent)
        value = callPropertyValueRead(property, value);
    return OPENDAQ_SUCCESS;
}

#if defined(__GNUC__) && __GNUC__ >= 12
#pragma GCC diagnostic pop
#endif

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertyValue(IString* propertyName, IBaseObject** value)
{
    auto lock = getRecursiveConfigLock();
    return getPropertyValueInternal(propertyName, value);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertyValueNoLock(IString* propertyName, IBaseObject** value)
{
    return getPropertyValueInternal(propertyName, value);
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertySelectionValue(IString* propertyName, IBaseObject** value)
{
    auto lock = getRecursiveConfigLock();
    return getPropertySelectionValueInternal(propertyName, value);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertySelectionValueNoLock(IString* propertyName,
                                                                                                    IBaseObject** value)
{
    return getPropertySelectionValueInternal(propertyName, value);
}

template <class PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::clearProtectedPropertyValue(IString* propertyName)
{
    auto lock = getRecursiveConfigLock();
    return clearPropertyValueInternal(propertyName, true, updateCount > 0);
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::configureClonedMembers(
    const std::unordered_map<StringPtr, PropertyValueEventEmitter>& valueWriteEvents,
    const std::unordered_map<StringPtr, PropertyValueEventEmitter>& valueReadEvents,
    const EndUpdateEventEmitter& endUpdateEvent,
    const ProcedurePtr& triggerCoreEvent,
    const PropertyOrderedMap& localProperties,
    const std::unordered_map<StringPtr, BaseObjectPtr, StringHash, StringEqualTo>& propValues,
    const std::vector<StringPtr>& customOrder,
    const PermissionManagerPtr& permissionManager)
{
    this->valueWriteEvents = valueWriteEvents;
    this->valueReadEvents = valueReadEvents;
    this->endUpdateEvent = endUpdateEvent;
    this->triggerCoreEvent = triggerCoreEvent;
    this->localProperties = localProperties;
    this->customOrder = customOrder;

    BaseObjectPtr permissionManagerClone;
    permissionManager.asPtr<ICloneable>()->clone(&permissionManagerClone);
    this->permissionManager = permissionManagerClone;

    for (const auto& val : propValues)
    {
        const auto ct = val.second.getCoreType();
        if (ct == ctList || ct == ctDict)
        {
            if (const auto cloneable = val.second.asPtrOrNull<ICloneable>(); cloneable.assigned())
            {
                BaseObjectPtr obj;
                const ErrCode err = cloneable->clone(&obj);
                if (OPENDAQ_FAILED(err) || !obj.assigned())
                    continue;

                this->propValues.insert(std::make_pair(val.first, obj));
            }
        }
        else if (ct == ctObject)
        {
            if (const auto cloneable = val.second.asPtrOrNull<IPropertyObjectInternal>(); cloneable.assigned())
            {
                PropertyObjectPtr obj;
                const ErrCode err = cloneable->clone(&obj);
                if (OPENDAQ_FAILED(err) || !obj.assigned())
                    continue;

                auto it = this->propValues.find(val.first);
                if (it != this->propValues.end())
                    it->second = obj;
                else
                    this->propValues.insert(std::make_pair(val.first, obj));
            }
        }
        else
        {
            this->propValues.insert(val);
        }
    }
}

template <typename PropObjInterface, typename... Interfaces>
std::unique_ptr<RecursiveConfigLockGuard> GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getRecursiveConfigLock()
{
    if (externalCallThreadId != std::thread::id() && externalCallThreadId == std::this_thread::get_id())
        return std::make_unique<GenericRecursiveConfigLockGuard<object_utils::NullMutex>>(&nullSync, &externalCallThreadId, &externalCallDepth);

    return std::make_unique<GenericRecursiveConfigLockGuard<std::mutex>>(&sync, &externalCallThreadId, &externalCallDepth);
}

template <typename PropObjInterface, typename... Interfaces>
std::lock_guard<std::mutex> GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getAcquisitionLock()
{
    return std::lock_guard(sync);
}

template <typename PropObjInterface, typename ... Interfaces>
std::unique_lock<std::mutex> GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getUniqueLock()
{
    return std::unique_lock(sync);
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::clearPropertyValue(IString* propertyName)
{
    auto lock = getRecursiveConfigLock();
    return clearPropertyValueInternal(propertyName, false, updateCount > 0);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::clearPropertyValueNoLock(IString* propertyName)
{
    return clearPropertyValueInternal(propertyName, false, updateCount > 0);
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::clearPropertyValueInternal(IString* name,
                                                                                               bool protectedAccess,
                                                                                               bool batch,
                                                                                               bool isUpdating)
{
    if (name == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    try
    {
        auto propName = StringPtr::Borrow(name);

        if (batch)
        {
            updatingPropsAndValues.emplace_back(std::make_pair(propName, UpdatingAction{false, protectedAccess, nullptr}));
            return OPENDAQ_SUCCESS;
        }

        StringPtr childName;
        StringPtr subName;
        const auto isChildProp = isChildProperty(propName, childName, subName);
        if (isChildProp)
        {
            propName = childName;
        }

        PropertyPtr prop = getUnboundPropertyOrNull(propName);
        prop = checkForRefPropAndGetBoundProp(prop);

        if (!prop.assigned())
        {
            return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property "{}" does not exist)", propName));
        }

        propName = prop.getName();
        const auto propInternal = prop.asPtr<IPropertyInternal>();

        if (!protectedAccess)
        {
            if (propInternal.getReadOnlyNoLock() && !isChildProp)
            {
                return OPENDAQ_ERR_ACCESSDENIED;
            }
        }

        if (isChildProp)
        {
            BaseObjectPtr childProp;
            const ErrCode err = getPropertyValueInternal(propName, &childProp);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            if (protectedAccess)
            {
                const auto childPropAsPropertyObject = childProp.template asPtr<IPropertyObjectProtected>(true);
                childPropAsPropertyObject.clearProtectedPropertyValue(subName);
            }
            else
            {
                const auto childPropAsPropertyObject = childProp.template asPtr<IPropertyObject, PropertyObjectPtr>(true);
                childPropAsPropertyObject.clearPropertyValue(subName);
            }
        }
        else
        {
            auto it = propValues.find(prop.getName());
            if (it == propValues.end())
            {
                return OPENDAQ_IGNORED;
            }

            if (it->second.assigned())
            {
                const auto ownable = it->second.template asPtrOrNull<IOwnable>(true);
                if (ownable.assigned())
                    ownable.setOwner(nullptr);
            }

            propValues.erase(it);
            cloneAndSetChildPropertyObject(prop);

            const auto val = callPropertyValueWrite(prop, nullptr, PropertyEventType::Clear, isUpdating);
            if (!isUpdating)
                triggerCoreEventInternal(CoreEventArgsPropertyValueChanged(objPtr, propName, val, path));
        }
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertyValueInternal(IString* propertyName, IBaseObject** value)
{
    if (propertyName == nullptr || value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    try
    {
        auto propName = StringPtr::Borrow(propertyName);
        BaseObjectPtr valuePtr;
        ErrCode err;

        StringPtr childName;
        StringPtr subName;

        if (isChildProperty(propName, childName, subName))
        {
            err = getChildPropertyValue(childName, subName, valuePtr);
        }
        else
        {
            PropertyPtr prop;
            err = getPropertyAndValueInternal(propName, valuePtr, prop);
        }

        if (OPENDAQ_SUCCEEDED(err))
            *value = valuePtr.detach();

        return err;
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertySelectionValueInternal(IString* propertyName,
                                                                                                      IBaseObject** value)
{
    if (propertyName == nullptr || value == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    try
    {
        const auto propName = StringPtr::Borrow(propertyName);
        BaseObjectPtr valuePtr;
        PropertyPtr prop;

        getPropertyAndValueInternal(propName, valuePtr, prop);

        if (!prop.assigned())
            throw NotFoundException(R"(Selection property "{}" not found)", propName);

        const auto propInternal = prop.asPtr<IPropertyInternal>();
        auto values = propInternal.getSelectionValuesNoLock();
        if (!values.assigned())
            throw InvalidPropertyException(R"(Selection property "{}" has no selection values assigned)", propName);

        auto valuesList = values.asPtrOrNull<IList, ListPtr<IBaseObject>>(true);
        if (!valuesList.assigned())
        {
            auto valuesDict = values.asPtrOrNull<IDict, DictPtr<IBaseObject, IBaseObject>>(true);
            if (!valuesDict.assigned())
            {
                throw InvalidPropertyException(R"(Selection property "{}" values is not a list or dictionary)", propName);
            }

            valuePtr = valuesDict.get(valuePtr);
        }
        else
        {
            valuePtr = valuesList.getItemAt(valuePtr);
        }

        if (propInternal.getItemTypeNoLock() != valuePtr.getCoreType())
        {
            return this->makeErrorInfo(OPENDAQ_ERR_INVALIDTYPE, "List item type mismatch");
        }

        *value = valuePtr.detach();
        return OPENDAQ_SUCCESS;
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return errorFromException(e);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getProperty(IString* propertyName, IProperty** property)
{
    if (propertyName == nullptr || property == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return daqTry([&]() -> auto {
        StringPtr childName;
        StringPtr subName;
        StringPtr propName = propertyName;

        const auto isChildProp = isChildProperty(propName, childName, subName);

        PropertyPtr prop;

        if (isChildProp)
        {
            propName = childName;
            BaseObjectPtr childProp;
            const ErrCode err = getPropertyValueInternal(propName, &childProp);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            const auto childPropAsPropertyObject = childProp.template asPtr<IPropertyObject, PropertyObjectPtr>(true);
            prop = childPropAsPropertyObject.getProperty(subName);
        }
        else
        {
            prop = getUnboundProperty(propName);
            prop = prop.asPtr<IPropertyInternal>().cloneWithOwner(objPtr);
        }

        auto freezable = prop.template asPtrOrNull<IFreezable>();
        if (freezable.assigned())
        {
            freezable.freeze();
        }

        *property = prop.detach();
        return OPENDAQ_SUCCESS;
    });
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::addProperty(IProperty* property)
{
    if (property == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    return daqTry([&]() -> auto {
        const PropertyPtr propPtr = property;
        StringPtr propName = propPtr.getName();
        if (!propName.assigned())
            return this->makeErrorInfo(OPENDAQ_ERR_INVALIDVALUE, "Property does not have an assigned name.");

        if (hasDuplicateReferences(propPtr))
            return this->makeErrorInfo(OPENDAQ_ERR_INVALIDVALUE,
                                       "Reference property references a property that is already referenced by another.");

        propPtr.asPtr<IOwnable>().setOwner(objPtr);

        const auto res = localProperties.insert(std::make_pair(propName, propPtr));
        if (!res.second)
            return this->makeErrorInfo(OPENDAQ_ERR_ALREADYEXISTS, fmt::format(R"(Property with name {} already exists.)", propName));

        auto readEvent = propPtr.asPtr<IPropertyInternal>().getClassOnPropertyValueRead();
        if (readEvent.getListenerCount())
        {
            PropertyValueEventEmitter emitter;
            valueReadEvents.emplace(propName, emitter);
            for (const auto& listener : readEvent.getListeners())
                emitter.addHandler(listener);
        }

        auto writeEvent = propPtr.asPtr<IPropertyInternal>().getClassOnPropertyValueWrite();
        if (writeEvent.getListenerCount())
        {
            PropertyValueEventEmitter emitter;
            valueWriteEvents.emplace(propName, emitter);
            for (const auto& listener : writeEvent.getListeners())
                emitter.addHandler(listener);
        }

        cloneAndSetChildPropertyObject(propPtr);
        triggerCoreEventInternal(CoreEventArgsPropertyAdded(objPtr, propPtr, path));

        return OPENDAQ_SUCCESS;
    });
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::removeProperty(IString* propertyName)
{
    if (propertyName == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    if (frozen)
    {
        return OPENDAQ_ERR_FROZEN;
    }

    if (localProperties.find(propertyName) == localProperties.cend())
    {
        StringPtr namePtr = propertyName;
        return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property "{}" does not exist)", namePtr));
    }

    localProperties.erase(propertyName);
    if (propValues.find(propertyName) != propValues.cend())
    {
        propValues.erase(propertyName);
    }

    triggerCoreEventInternal(CoreEventArgsPropertyRemoved(objPtr, propertyName, path));

    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getVisibleProperties(IList** properties)
{
    return getPropertiesInternal(false, true, properties);
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getAllProperties(IList** properties)
{
    return getPropertiesInternal(true, true, properties);
}

template <class PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setPropertyOrder(IList* orderedPropertyNames)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    customOrder.clear();
    if (orderedPropertyNames != nullptr)
    {
        for (auto&& propName : ListPtr<IString>::Borrow(orderedPropertyNames))
        {
            customOrder.emplace_back(propName);
        }
    }

    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertiesInternal(Bool includeInvisible,
                                                                                          Bool bind,
                                                                                          IList** list)
{
    if (list == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (!includeInvisible && !bind)
        return OPENDAQ_ERR_INVALIDPARAMETER;

    std::vector<PropertyPtr> allProperties;
    if (objectClass.assigned())
    {
        auto propList = objectClass.getProperties(True);
        for (const auto& prop : propList)
            allProperties.push_back(prop);
    }

    for (const auto& prop : localProperties)
        allProperties.push_back(prop.second);

    PropertyOrderedMap lookup;
    for (auto& prop : allProperties)
    {
        if (!bind)
        {
            lookup.insert({prop.getName(), prop});
            continue;
        }

        auto boundProp = prop.asPtr<IPropertyInternal>().cloneWithOwner(objPtr);
        if (!includeInvisible && boundProp.getIsReferenced())
        {
            continue;
        }

        try
        {
            if (!includeInvisible && !boundProp.getVisible())
            {
                continue;
            }

            auto freezable = boundProp.template asPtrOrNull<IFreezable>();
            if (freezable.assigned())
            {
                freezable.freeze();
            }

            lookup.insert_or_assign(boundProp.getName(), boundProp);
        }
        catch (const NotFoundException&)
        {
            return OPENDAQ_ERR_NOTFOUND;
        }
        catch (const CalcFailedException&)
        {
        }
        catch (const NoInterfaceException&)
        {
        }
    }

    auto properties = List<IProperty>();
    if (!customOrder.empty())
    {
        // Add properties with explicit order
        for (auto& propName : customOrder)
        {
            const auto iter = lookup.find(propName);
            if (iter != lookup.cend())
            {
                properties.unsafePushBack(iter->second);
                lookup.erase(iter);
            }
        }

        // Add the rest of without set order
        for (auto& prop : lookup)
        {
            properties.unsafePushBack(prop.second);
        }
    }
    else
    {
        for (auto& prop : lookup)
        {
            properties.unsafePushBack(prop.second);
        }
    }

    *list = properties.detach();
    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getOnPropertyValueWrite(IString* propertyName, IEvent** event)
{
    if (event == nullptr || propertyName == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    Bool hasProp;
    StringPtr name = propertyName;

    ErrCode err = this->hasProperty(name, &hasProp);
    if (OPENDAQ_FAILED(err))
    {
        return err;
    }

    if (!hasProp)
    {
        return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property "{}" does not exist)", name));
    }

    if (valueWriteEvents.find(name) == valueWriteEvents.end())
    {
        PropertyValueEventEmitter emitter;
        valueWriteEvents.emplace(name, emitter);
    }

    *event = valueWriteEvents[name].addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getOnPropertyValueRead(IString* propertyName, IEvent** event)
{
    if (event == nullptr || propertyName == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    Bool hasProp;
    StringPtr name = propertyName;

    ErrCode err = this->hasProperty(name, &hasProp);
    if (OPENDAQ_FAILED(err))
    {
        return err;
    }

    if (!hasProp)
    {
        return this->makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property "{}" does not exist)", name));
    }

    if (valueReadEvents.find(name) == valueReadEvents.end())
    {
        PropertyValueEventEmitter emitter;
        valueReadEvents.emplace(name, emitter);
    }

    *event = valueReadEvents[name].addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename ... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getOnAnyPropertyValueWrite(IEvent** event)
{
    if (event == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *event = valueWriteEvents[AnyWriteEventName].addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename ... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getOnAnyPropertyValueRead(IEvent** event)
{
    if (event == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *event = valueReadEvents[AnyReadEventName].addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::beginUpdate()
{
    auto lock = getRecursiveConfigLock();
    return beginUpdateInternal(true);
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::callBeginUpdateOnChildren()
{
    for (const auto& item : propValues)
    {
        const auto value = item.second;
        if (value.assigned())
        {
            const auto propObj = value.template asPtrOrNull<IPropertyObject>(true);
            if (propObj.assigned())
            {
                propObj.beginUpdate();
            }
        }
    }
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::callEndUpdateOnChildren()
{
    for (const auto& item : propValues)
    {
        const auto value = item.second;
        if (value.assigned())
        {
            const auto propObj = value.template asPtrOrNull<IPropertyObject>(true);
            if (propObj.assigned())
            {
                propObj.endUpdate();
            }
        }
    }
}

template <typename PropObjInterface, typename... Interfaces>
PropertyObjectPtr GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPropertyObjectParent()
{
    if (owner.assigned())
        return owner.getRef();

    return nullptr;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::beginUpdateInternal(bool deep)
{
    if (frozen)
        return OPENDAQ_ERR_FROZEN;

    updateCount++;

    if (deep)
        return daqTry([this] { callBeginUpdateOnChildren(); });

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::endUpdateInternal(bool deep)
{
    if (updateCount == 0)
        return OPENDAQ_ERR_INVALIDSTATE;

    const auto newUpdateCount = --updateCount;

    if (newUpdateCount == 0)
    {
        const auto errCode = daqTry([this] {
            beginApplyUpdate();
            return OPENDAQ_SUCCESS;
        });

        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    if (deep)
    {
        const auto errCode = daqTry([this] {
            callEndUpdateOnChildren();
        });

        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }

    if (newUpdateCount == 0)
    {
        return daqTry([this] {
            endApplyUpdate();
            return OPENDAQ_SUCCESS;
        });
    }

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getUpdatingInternal(Bool* updating)
{
    OPENDAQ_PARAM_NOT_NULL(updating);

    *updating = updateCount > 0 ? True : False;
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::beginApplyUpdate()
{
    beginApplyProperties(updatingPropsAndValues, isParentUpdating());
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::endApplyUpdate()
{
    auto ignoredProps = List<IString>();
    for (auto& item : updatingPropsAndValues)
    {
        StringPtr name = item.first;
        ErrCode err;
        if (item.second.setValue)
        {
            err = setPropertyValueInternal(name, item.second.value, true, item.second.protectedAccess, false, true);
        }
        else
        {
            err = clearPropertyValueInternal(name, item.second.protectedAccess, false, true);
        }

        if (err == OPENDAQ_IGNORED)
        {
            ignoredProps.pushBack(name);
        }
        else
        {
            PropertyPtr prop;
            getPropertyAndValueInternal(name, item.second.value, prop);
        }

        checkErrorInfo(err);
    }

    for (const auto& propName : ignoredProps)
    {
        auto it = std::find_if(updatingPropsAndValues.begin(),
                               updatingPropsAndValues.end(),
                               [propName](const std::pair<std::string, UpdatingAction>& val)
                               {
                                   return propName == val.first;
                               });
        if (it != updatingPropsAndValues.end())
            updatingPropsAndValues.erase(it);
    }

    endApplyProperties(updatingPropsAndValues, isParentUpdating());
    updatingPropsAndValues.clear();
}

template <typename PropObjInterface, typename... Interfaces>
bool GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::isParentUpdating()
{
    bool parentUpdating;
    const auto parent = getPropertyObjectParent();
    if (parent.assigned())
        parentUpdating = parent.template asPtr<IPropertyObjectInternal>(true).isUpdating();
    else
        parentUpdating = false;

    return parentUpdating;
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::onUpdatableUpdateEnd(const BaseObjectPtr& /* context */)
{
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::endUpdate()
{
    auto lock = getRecursiveConfigLock();
    return endUpdateInternal(true);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getUpdating(Bool* updating)
{
    auto lock = getRecursiveConfigLock();
    return getUpdatingInternal(updating);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getOnEndUpdate(IEvent** event)
{
    if (event == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *event = endUpdateEvent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getPermissionManager(IPermissionManager** permissionManager)
{
    OPENDAQ_PARAM_NOT_NULL(permissionManager);

    *permissionManager = this->permissionManager.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
Bool GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkIsReferenced(const StringPtr& referencedPropName,
                                                                                   const PropertyInternalPtr& prop)
{
    if (const auto refProp = prop.getReferencedPropertyUnresolved(); refProp.assigned())
    {
        for (auto propName : refProp.getPropertyReferences())
        {
            if (propName == referencedPropName)
            {
                return true;
            }
        }
    }

    return false;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkForReferencesInternal(IProperty* property, Bool* isReferenced)
{
    OPENDAQ_PARAM_NOT_NULL(isReferenced);
    *isReferenced = false;

    try
    {
        const auto propPtr = PropertyPtr::Borrow(property);
        const auto name = propPtr.getName();

        if (objectClass.assigned())
        {
            for (const auto& prop : objectClass.getProperties(True))
            {
                if (*isReferenced = checkIsReferenced(name, prop); *isReferenced)
                    return OPENDAQ_SUCCESS;
            }
        }

        for (const auto& prop : localProperties)
        {
            if (*isReferenced = checkIsReferenced(name, prop.second); *isReferenced)
                return OPENDAQ_SUCCESS;
        }
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkForReferencesNoLock(IProperty* property, Bool* isReferenced)
{
    return checkForReferencesInternal(property, isReferenced);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::checkForReferences(IProperty* property, Bool* isReferenced)
{
    auto lock = getRecursiveConfigLock();
    return checkForReferencesInternal(property, isReferenced);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::enableCoreEventTrigger()
{
    coreEventMuted = false;

    for (auto& item : propValues)
    {
        if (item.second.assigned() && item.second.supportsInterface(IPropertyObject::Id))
        {
            configureClonedObj(item.first, item.second);
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::disableCoreEventTrigger()
{
    coreEventMuted = true;

    for (auto& item : propValues)
    {
        if (item.second.assigned())
        {
            const auto objInternal = item.second.template asPtrOrNull<IPropertyObjectInternal>();
            if (objInternal.assigned())
                objInternal.disableCoreEventTrigger();
        }
    }

    for (const auto& item : localProperties)
    {
        if (item.second.assigned())
        {
            const auto propInternal = item.second.template asPtr<IPropertyInternal>();
            if (propInternal.getValueTypeUnresolved() == ctObject)
            {
                const auto defaultVal = item.second.getDefaultValue();
                if (defaultVal.assigned())
                {
                    const auto objInternal = defaultVal.template asPtrOrNull<IPropertyObjectInternal>();
                    if (objInternal.assigned())
                        objInternal.disableCoreEventTrigger();
                }
            }
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getCoreEventTrigger(IProcedure** trigger)
{
    OPENDAQ_PARAM_NOT_NULL(trigger);

    *trigger = this->triggerCoreEvent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setCoreEventTrigger(IProcedure* trigger)
{
    this->triggerCoreEvent = trigger;
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::clone(IPropertyObject** cloned)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    const auto managerRef = manager.assigned() ? manager.getRef() : nullptr;
    PropertyObjectPtr obj = createWithImplementation<IPropertyObject, PropertyObjectImpl>(managerRef, this->className);

    return daqTry([this, &obj, &cloned]()
    {
        auto implPtr = static_cast<PropertyObjectImpl*>(obj.getObject());
        implPtr->configureClonedMembers(valueWriteEvents,
                                        valueReadEvents,
                                        endUpdateEvent,
                                        triggerCoreEvent,
                                        localProperties,
                                        propValues,
                                        customOrder,
                                        permissionManager);

        *cloned = obj.detach();
        return OPENDAQ_SUCCESS;
    });
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setPath(IString* path)
{
    OPENDAQ_PARAM_NOT_NULL(path);

    if (this->path == "")
        this->path = path;
    else
        return OPENDAQ_IGNORED;

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::isUpdating(Bool* updating)
{
    OPENDAQ_PARAM_NOT_NULL(updating);

    *updating = updateCount > 0 ? True : False;
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::hasUserReadAccess(IBaseObject* userContext, Bool* hasAccessOut)
{
    OPENDAQ_PARAM_NOT_NULL(hasAccessOut);
    const auto self = this->template borrowPtr<PropertyObjectPtr>();
    *hasAccessOut = hasUserReadAccess(userContext, self);
    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::serializeCustomValues(ISerializer* /*serializer*/, bool /*forUpdate*/)
{
    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::serializePropertyValue(const StringPtr& name,
                                                                                           const ObjectPtr<IBaseObject>& value,
                                                                                           ISerializer* serializer)
{
    if (value.assigned())
    {
        ISerializable* serializableValue;
        ErrCode errCode = value->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializableValue));

        if (errCode == OPENDAQ_ERR_NOINTERFACE)
        {
            return OPENDAQ_SUCCESS;
        }

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        errCode = serializer->keyStr(name);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        errCode = serializableValue->serialize(serializer);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }
    else
    {
        ErrCode errCode = serializer->keyStr(name);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        errCode = serializer->writeNull();
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::serializePropertyValues(ISerializer* serializer)
{
    auto serializerPtr = SerializerPtr::Borrow(serializer);

    const int numOfSerializablePropertyValues =
        std::count_if(propValues.begin(), propValues.end(), [](const std::pair<StringPtr, BaseObjectPtr>& keyValue) {
            return keyValue.second.supportsInterface<ISerializable>();
        });

    if (numOfSerializablePropertyValues == 0)
        return OPENDAQ_SUCCESS;

    serializer->key("propValues");
    serializer->startObject();
    {
        std::map<StringPtr, BaseObjectPtr> sorted(propValues.begin(), propValues.end());

        // Serialize properties with explicit order
        for (auto&& propName : customOrder)
        {
            auto propValue = sorted.find(propName);
            if (propValue != sorted.cend())
            {
                if (!hasUserReadAccess(serializerPtr.getUser(), propValue->second))
                    continue;

                ErrCode err = serializePropertyValue(propValue->first, propValue->second, serializer);
                if (OPENDAQ_FAILED(err))
                {
                    return err;
                }
                sorted.erase(propValue);
            }
        }

        // Serialize the rest of without set order
        for (auto&& propValue : sorted)
        {
            if (!hasUserReadAccess(serializerPtr.getUser(), propValue.second))
                continue;

            ErrCode err = serializePropertyValue(propValue.first, propValue.second, serializer);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }
        }
    }

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::serializeLocalProperties(ISerializer* serializer)
{
    return daqTry([&serializer, this]
    {
        if (localProperties.size() == 0)
            return OPENDAQ_NOTFOUND;

        auto serializerPtr = SerializerPtr::Borrow(serializer);

        checkErrorInfo(serializer->key("properties"));
        checkErrorInfo(serializer->startList());
        for (const auto& prop : localProperties)
        {
            if (!hasUserReadAccess(serializerPtr.getUser(), prop.second.getDefaultValue()))
                continue;

            prop.second.serialize(serializer);
        }
        checkErrorInfo(serializer->endList());

        return OPENDAQ_SUCCESS;
    });
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::serialize(ISerializer* serializer)
{
    auto serializerPtr = SerializerPtr::Borrow(serializer);
    Bool hasAccess = false;
    ErrCode serializeErrCode = hasUserReadAccess(serializerPtr.getUser(), &hasAccess);

    if (OPENDAQ_FAILED(serializeErrCode))
        return serializeErrCode;
    if (!hasAccess)
        return OPENDAQ_ERR_ACCESSDENIED;

    serializer->startTaggedObject(this);

    SERIALIZE_PROP_PTR(className)

    if (frozen)
    {
        serializer->key("frozen");
        serializer->writeBool(frozen);
    }

    serializeErrCode = serializeCustomValues(serializer, false);
    if (OPENDAQ_FAILED(serializeErrCode))
    {
        return serializeErrCode;
    }

    serializeErrCode = serializePropertyValues(serializer);
    if (OPENDAQ_FAILED(serializeErrCode))
    {
        return serializeErrCode;
    }

    serializeErrCode = serializeLocalProperties(serializer);
    if (OPENDAQ_FAILED(serializeErrCode))
    {
        return serializeErrCode;
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
template <class F>
PropertyObjectPtr GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::DeserializePropertyObject(
    const SerializedObjectPtr& serialized, const BaseObjectPtr& context, const FunctionPtr& factoryCallback, F&& f)
{
    StringPtr className;
    if (serialized.hasKey("className"))
        className = serialized.readString("className");

    Bool isFrozen{};
    if (serialized.hasKey("frozen"))
        isFrozen = serialized.readBool("frozen");

    PropertyObjectPtr propObjPtr = f(serialized, context, className);

    DeserializeLocalProperties(serialized, context, factoryCallback, propObjPtr);

    DeserializePropertyValues(serialized, context, factoryCallback, propObjPtr);

    if (isFrozen)
    {
        const auto freezable = propObjPtr.asPtrOrNull<IFreezable>(true);
        if (freezable.assigned())
            freezable.freeze();
    }

    return propObjPtr;
}

// static
template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::Deserialize(ISerializedObject* serialized,
                                                                                IBaseObject* context,
                                                                                IFunction* factoryCallback,
                                                                                IBaseObject** obj)
{
    OPENDAQ_PARAM_NOT_NULL(serialized);
    OPENDAQ_PARAM_NOT_NULL(obj);

    return daqTry([&serialized, &context, &factoryCallback, &obj]() {
        *obj = DeserializePropertyObject(serialized,
                                         context,
                                         factoryCallback,
                                         [](const SerializedObjectPtr&, const BaseObjectPtr& context, const StringPtr& className) {
                                             const TypeManagerPtr objManager =
                                                 context.assigned() ? context.asOrNull<ITypeManager>() : nullptr;
                                             if (objManager.assigned())
                                                 return PropertyObject(objManager, className);
                                             return PropertyObject();
                                         })
                   .detach();
    });
}

template <class PropObjInterface, class... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::DeserializePropertyValues(const SerializedObjectPtr& serialized,
                                                                                           const BaseObjectPtr& context,
                                                                                           const FunctionPtr& factoryCallback,
                                                                                           PropertyObjectPtr& propObjPtr)
{
    const auto hasKeyStr = String("propValues");

    if (!serialized.hasKey(hasKeyStr))
        return;

    const auto propValues = serialized.readSerializedObject("propValues");

    const auto keys = propValues.getKeys();

    const auto protectedPropObjPtr = propObjPtr.asPtr<IPropertyObjectProtected>(true);

    for (const auto& key : keys)
    {
        const auto propValue = propValues.readObject(key, context, factoryCallback);
        protectedPropObjPtr.setProtectedPropertyValue(key, propValue);
    }
}

template <class PropObjInterface, class... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::DeserializeLocalProperties(const SerializedObjectPtr& serialized,
                                                                                            const BaseObjectPtr& context,
                                                                                            const FunctionPtr& /*factoryCallback*/,
                                                                                            PropertyObjectPtr& propObjPtr)
{
    const auto keyStr = String("properties");

    const auto hasKey = serialized.hasKey(keyStr);

    if (!IsTrue(hasKey))
        return;

    const auto propertyList = serialized.readSerializedList(keyStr);

    for (SizeT i = 0; i < propertyList.getCount(); i++)
    {
        const PropertyPtr prop = propertyList.readObject(context);
        const auto propName = prop.getName();

        if (!propObjPtr.hasProperty(propName))
        {
            propObjPtr.addProperty(prop);
        }
    }
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::toString(CharPtr* str)
{
    if (str == nullptr)
    {
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Parameter must not be null");
    }

    std::ostringstream stream;
    stream << "PropertyObject";

    if (className.assigned())
        stream << " {" << className << "}";

    return daqDuplicateCharPtr(stream.str().c_str(), str);
}

template <class PropObjInterface, class... Interfaces>
ConstCharPtr GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::SerializeId()
{
    return "PropertyObject";
}

template <class PropObjInterface, class... Interfaces>
WeakRefPtr<IPropertyObject> GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::getOwner() const
{
    return owner;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setOwner(IPropertyObject* newOwner)
{
    this->owner = newOwner;

    const PermissionManagerPtr parentManager = this->owner.assigned() ? this->owner.getRef().getPermissionManager() : nullptr;
    this->permissionManager.template asPtr<IPermissionManagerInternal>(true).setParent(parentManager);

    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::freeze()
{
    if (frozen)
        return OPENDAQ_IGNORED;

    frozen = true;
    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::isFrozen(Bool* isFrozen) const
{
    if (isFrozen == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *isFrozen = frozen;
    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::hasProperty(IString* propertyName, Bool* hasProperty)
{
    if (hasProperty == nullptr || propertyName == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    if (localProperties.find(propertyName) != localProperties.cend())
    {
        *hasProperty = true;
        return OPENDAQ_SUCCESS;
    }

    if (objectClass.assigned())
    {
        try
        {
            *hasProperty = objectClass.hasProperty(propertyName);
            if (*hasProperty)
                return OPENDAQ_SUCCESS;
        }
        catch (...)
        {
        }
    }

    *hasProperty = false;
    return OPENDAQ_SUCCESS;
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::setPropertyFromSerialized(const StringPtr& propName,
                                                                                              const PropertyObjectPtr& propObj,
                                                                                              const SerializedObjectPtr& serialized)
{
    if (!serialized.assigned())
    {
        return propObj->clearPropertyValue(propName);
    }

    BaseObjectPtr propValue;

    switch (serialized.getType(propName))
    {
        case ctBool:
            propValue = serialized.readBool(propName);
            break;
        case ctInt:
            propValue = serialized.readInt(propName);
            break;
        case ctFloat:
            propValue = serialized.readFloat(propName);
            break;
        case ctString:
            propValue = serialized.readString(propName);
            break;
        case ctList:
            propValue = serialized.readList<IBaseObject>(propName, manager.assigned() ? manager.getRef() : nullptr);
            break;
        case ctDict:
        case ctRatio:
        case ctStruct:
        case ctObject:
        {
            const auto obj = propObj.getPropertyValue(propName);
            if (const auto updatable = obj.asPtrOrNull<IUpdatable>(); updatable.assigned())
            {
                const auto serializedNestedObj = serialized.readSerializedObject(propName);
                return updatable->update(serializedNestedObj, manager.assigned() ? manager.getRef() : nullptr);
            }

            propValue = serialized.readObject(propName);
            break;
        }
        case ctProc:
        case ctBinaryData:
        case ctFunc:
        case ctComplexNumber:
        case ctEnumeration:
        case ctUndefined:
            return OPENDAQ_SUCCESS;
    }

    return propObj.as<IPropertyObjectProtected>(true)->setProtectedPropertyValue(propName, propValue);
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::endApplyProperties(const UpdatingActions& propsAndValues,
                                                                                    bool parentUpdating)
{
    auto list = List<IString>();
    auto dict = Dict<IString, IBaseObject>();
    for (const auto& item : propsAndValues)
    {
        list.pushBack(item.first);
        dict.set(item.first, item.second.value);
    }

    if (endUpdateEvent.hasListeners())
    {
        auto args = EndUpdateEventArgs(list, parentUpdating);
        endUpdateEvent(objPtr, args);
    }

    if (dict.getCount() > 0)
        triggerCoreEventInternal(CoreEventArgsPropertyObjectUpdateEnd(objPtr, dict, path));
}

template <typename PropObjInterface, typename... Interfaces>
void GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::beginApplyProperties(const UpdatingActions& /* propsAndValues */,
                                                                                      bool /* parentUpdating */)
{
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::updateObjectProperties(const PropertyObjectPtr& propObj,
                                                                                           const SerializedObjectPtr& serialized,
                                                                                           const ListPtr<IProperty>& props)
{
    SerializedObjectPtr serializedProps;

    if (serialized.hasKey("propValues"))
        serializedProps = serialized.readSerializedObject("propValues");

    beginUpdate();
    Finally finally([this]() { endUpdate(); });

    for (const auto& prop : props)
    {
        const auto propName = prop.getName();

        const auto propInternal = prop.asPtrOrNull<IPropertyInternal>(true);
        if (propInternal.assigned())
        {
            if (propInternal.getReferencedPropertyUnresolved().assigned())
                continue;
            const auto valueTypeUnresolved = propInternal.getValueTypeUnresolved();
            if (valueTypeUnresolved == CoreType::ctFunc || valueTypeUnresolved == CoreType::ctProc)
                continue;
        }

        if (!serializedProps.assigned() || !serializedProps.hasKey(propName))
        {
            const auto err = propObj.as<IPropertyObjectProtected>(true)->clearProtectedPropertyValue(propName);
            if (!OPENDAQ_SUCCEEDED(err) && err != OPENDAQ_ERR_INVALID_OPERATION)
            {
                return err;
            }

            continue;
        }

        const auto err = setPropertyFromSerialized(propName, propObj, serializedProps);

        if (!OPENDAQ_SUCCEEDED(err))
        {
            return err;
        }
    }

    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
bool GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::hasUserReadAccess(const BaseObjectPtr& userContext,
                                                                                   const BaseObjectPtr& obj)
{
    if (!obj.assigned())
        return true;

    auto objPtr = obj.asPtrOrNull<IPropertyObject>();
    if (!objPtr.assigned())
        return true;

    auto userContextPtr = BaseObjectPtr::Borrow(userContext);
    if (!userContextPtr.assigned())
        return true;

    auto userPtr = userContextPtr.asPtrOrNull<IUser>();
    if (!userPtr.assigned())
        return true;

    return objPtr.getPermissionManager().isAuthorized(userPtr, Permission::Read);
}

template <class PropObjInterface, class... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::updateInternal(ISerializedObject* obj, IBaseObject* /* context */)
{
    if (obj == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    // Don't fail the upgrade if frozen just skip it
    // TODO: Check if upgrade should be allowed
    if (frozen)
        return OPENDAQ_IGNORED;

    const auto serialized = SerializedObjectPtr::Borrow(obj);

    try
    {
        ListPtr<IProperty> allProps;
        checkErrorInfo(getPropertiesInternal(True, False, &allProps));

        return updateObjectProperties(this->thisInterface(), serialized, allProps);
    }
    catch (const DaqException& e)
    {
        return errorFromException(e);
    }
    catch (const std::exception& e)
    {
        return errorFromException(e);
    }
    catch (...)
    {
        return OPENDAQ_ERR_GENERALERROR;
    }
}

template <class PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::update(ISerializedObject* obj, IBaseObject* /* config */)
{
    return updateInternal(obj, nullptr);
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::serializeForUpdate(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    SERIALIZE_PROP_PTR(className)

    if (frozen)
    {
        serializer->key("frozen");
        serializer->writeBool(frozen);
    }

    ErrCode errCode = serializeCustomValues(serializer, true);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    errCode = serializePropertyValues(serializer);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

template <typename PropObjInterface, typename... Interfaces>
ErrCode GenericPropertyObjectImpl<PropObjInterface, Interfaces...>::updateEnded(IBaseObject* context)
{
    auto contextPtr = BaseObjectPtr::Borrow(context);
    return daqTry([this, &contextPtr] { onUpdatableUpdateEnd(contextPtr); });
}

OPENDAQ_REGISTER_DESERIALIZE_FACTORY(PropertyObjectImpl)

END_NAMESPACE_OPENDAQ
