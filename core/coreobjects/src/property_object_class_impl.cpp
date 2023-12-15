#include <coreobjects/property_object_class_impl.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_ptr.h>
#include <coreobjects/errors.h>
#include <coreobjects/property_internal_ptr.h>

#include "property_object_class_ptr.h"

BEGIN_NAMESPACE_OPENDAQ

PropertyObjectClassImpl::PropertyObjectClassImpl(IPropertyObjectClassBuilder* builder)
{
    const auto builderPtr = PropertyObjectClassBuilderPtr::Borrow(builder);
    this->name = builderPtr.getName();
    this->parent = builderPtr.getParentName();
    this->manager = builderPtr.getManager();

    const DictPtr<IString, IProperty> props = builderPtr.getProperties();
    for (const auto& [name, prop] : props)
        this->props.emplace(name, prop);

    const ListPtr<IString> customOrder = builderPtr.getPropertyOrder();
    for (const auto& name : customOrder)
        this->customOrder.push_back(name);
}


ErrCode PropertyObjectClassImpl::getName(IString** typeName)
{
    if (typeName == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *typeName = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getParentName(IString** parentName)
{
    if (parentName == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *parentName = this->parent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getProperty(IString* propertyName, IProperty** property)
{
    if (propertyName == nullptr || property == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    auto res = props.find(propertyName);
    if (res == props.cend())
    {
        if (parent.assigned())
        {
            TypeManagerPtr managerPtr;
            ErrCode err = getManager(managerPtr);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            TypePtr type;
            err = managerPtr->getType(parent, &type);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            const auto parentClass = type.asPtrOrNull<IPropertyObjectClass>();
            if (!parentClass.assigned())
                return OPENDAQ_ERR_INVALIDTYPE;

            return parentClass->getProperty(propertyName, property);
        }

        StringPtr str = propertyName;
        return makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property with name {} not  found.)", str));
    }

    *property = res.value().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::hasProperty(IString* propertyName, Bool* hasProperty)
{
    if (propertyName == nullptr || hasProperty == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    const auto res = props.find(propertyName);
    if (res == props.end())
    {
        if (parent.assigned())
        {
            TypeManagerPtr managerPtr;
            ErrCode err = getManager(managerPtr);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            TypePtr type;
            err = managerPtr->getType(parent, &type);
            if (OPENDAQ_FAILED(err))
            {
                return err;
            }

            const auto parentClass = type.asPtrOrNull<IPropertyObjectClass>();
            if (!parentClass.assigned())
                return OPENDAQ_ERR_INVALIDTYPE;

            return parentClass->hasProperty(propertyName, hasProperty);
        }

        *hasProperty = false;
        return OPENDAQ_SUCCESS;
    }

    *hasProperty = true;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getInheritedProperties(ListPtr<IProperty>& properties) const
{
    if (parent.assigned())
    {
        TypeManagerPtr managerPtr;
        ErrCode err = getManager(managerPtr);
        if (OPENDAQ_FAILED(err))
        {
            return err;
        }

        const auto getParentProps = [&]()
        {
            const auto parentClass = managerPtr.getType(parent).asPtr<IPropertyObjectClass>();
            properties = parentClass.getProperties(True);
        };

        return wrapHandler(getParentProps);
    }

    properties = List<IProperty>();        
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getManager(TypeManagerPtr& managerPtr) const
{
    if (!manager.assigned())
    {
        return makeErrorInfo(OPENDAQ_ERR_MANAGER_NOT_ASSIGNED, "Property object manager not assigned.");
    }

    managerPtr = manager.getRef();

    if (!managerPtr.assigned())
    {
        return makeErrorInfo(OPENDAQ_ERR_MANAGER_NOT_ASSIGNED, "Property object manager not assigned.");
    }
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getWithNormalOrder(Bool includeInherited, IList** list)
{
    ListPtr<IProperty> properties;

    if (includeInherited)
    {
        ErrCode errCode = getInheritedProperties(properties);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }
    else
    {
        properties = List<IProperty>();
    }

    for (auto& prop : props)
    {
        properties.unsafePushBack(prop.second);
    }

    *list = properties.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getWithCustomOrder(Bool includeInherited, IList** list)
{
    ListPtr<IProperty> properties;

    if (includeInherited)
    {
        ErrCode errCode = getInheritedProperties(properties);
        if (OPENDAQ_FAILED(errCode))
            return errCode;
    }
    else
    {
        properties = List<IProperty>();
    }

    PropertyOrderedMap lookup;
    lookup.reserve(properties.getCount() + props.size());

    // User might rearrange inherited properties
    for (const auto& prop : properties)
    {
        lookup.insert_or_assign(prop.getName(), prop);
    }

    for (auto& prop : props)
    {
        lookup.insert_or_assign(prop.first, prop.second);
    }

    properties.clear();

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

    *list = properties.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getProperties(Bool includeInherited, IList** properties)
{
    if (properties == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (customOrder.empty())
    {
        return getWithNormalOrder(includeInherited, properties);
    }
    return getWithCustomOrder(includeInherited, properties);
}

ErrCode PropertyObjectClassImpl::serializeProperties(ISerializer* serializer)
{
    serializer->key("properties");
    serializer->startObject();

    ListPtr<IProperty> properties;
    ErrCode errCode = getProperties(false, &properties);
    if (OPENDAQ_FAILED(errCode))
        return errCode;

    for (auto prop : properties)
    {
        StringPtr propName = prop.getName();
        serializer->keyStr(propName);

        if (prop.assigned())
        {
            ISerializable* serializableProp;
            errCode = prop->borrowInterface(ISerializable::Id, reinterpret_cast<void**>(&serializableProp));

            if (errCode == OPENDAQ_ERR_NOINTERFACE)
            {
                return makeErrorInfo(OPENDAQ_ERR_NOT_SERIALIZABLE,
                                     std::string("Property \"" + propName + "\" does not implement ISerializable."));
            }

            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }

            errCode = serializableProp->serialize(serializer);
            if (OPENDAQ_FAILED(errCode))
            {
                return errCode;
            }
        }
        else
        {
            serializer->writeNull();
        }
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

bool PropertyObjectClassImpl::hasDuplicateReferences(const PropertyPtr& prop)
{
    if (const auto refEval = prop.asPtr<IPropertyInternal>().getReferencedPropertyUnresolved(); refEval.assigned())
    {
        const auto refNames = refEval.getPropertyReferences();
        std::unordered_set<std::string> refNamesSet;
        for (auto refName : refNames)
            refNamesSet.insert(refName);

        const auto thisPtr = this->borrowPtr<PropertyObjectClassPtr>();
        for (auto ownProp : thisPtr.getProperties(true))
        {
            if (auto refEvalOwn = ownProp.asPtr<IPropertyInternal>().getReferencedPropertyUnresolved(); refEvalOwn.assigned())
            {
                auto refNamesOwn = refEvalOwn.getPropertyReferences();
                for (auto refPropName : refNamesOwn)
                {
                    if (refNamesSet.count(refPropName))
                        return true;
                }
            }
        }
    }

    return false;
}

ErrCode PropertyObjectClassImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);

    if (name.assigned())
    {
        serializer->key("name");
        name.serialize(serializer);
    }

    if (parent.assigned())
    {
        serializer->key("parent");
        parent.serialize(serializer);
    }

    ErrCode errCode = serializeProperties(serializer);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr PropertyObjectClassImpl::SerializeId()
{
    return "PropertyObjectClass";
}

ErrCode PropertyObjectClassImpl::Deserialize(ISerializedObject* serialized,
                                             IBaseObject* context,
                                             IFunction* factoryCallback,
                                             IBaseObject** obj)
{
    StringPtr name;
    ErrCode errCode = serialized->readString(String("name"), &name);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    IPropertyObjectClassBuilder* propClassObj;
    errCode = createPropertyObjectClassBuilder(&propClassObj, name);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }
    

    StringPtr parent;
    errCode = serialized->readString(String("parent"), &parent);

    if (errCode != OPENDAQ_ERR_NOTFOUND && OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    if (errCode != OPENDAQ_ERR_NOTFOUND)
    {
        propClassObj->setParentName(parent);
    }

    SerializedObjectPtr properties;
    errCode = serialized->readSerializedObject(String("properties"), &properties);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    ListPtr<IString> keys;
    errCode = properties->getKeys(&keys);
    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    for (const auto& key : keys)
    {
        IString* propName;
        errCode = key->borrowInterface(IString::Id, reinterpret_cast<void**>(&propName));

        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        BaseObjectPtr baseProp;
        errCode = properties->readObject(propName, context, factoryCallback, &baseProp);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        IProperty* prop;
        errCode = baseProp->borrowInterface(IProperty::Id, reinterpret_cast<void**>(&prop));
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }

        errCode = propClassObj->addProperty(prop);
        if (OPENDAQ_FAILED(errCode))
        {
            return errCode;
        }
    }

    *obj = propClassObj;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::toString(CharPtr* str)
{
    if (str == nullptr)
    {
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Parameter must not be null");
    }

    std::ostringstream stream;
    stream << "PropertyObjectClass {" << name << "}";

    return daqDuplicateCharPtr(stream.str().c_str(), str);
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyObjectClass,
    IPropertyObjectClass, createPropertyObjectClassFromBuilder,
    IPropertyObjectClassBuilder*, builder
)

END_NAMESPACE_OPENDAQ
