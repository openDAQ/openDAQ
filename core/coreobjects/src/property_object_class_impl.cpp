#include <coreobjects/property_object_class_impl.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_ptr.h>
#include <coreobjects/errors.h>
#include <coreobjects/property_internal_ptr.h>

#include <coreobjects/property_object_class_builder.h>
#include <coreobjects/property_object_internal.h>
#include <property_object_class_ptr.h>
#include <coreobjects/property_object_class_factory.h>

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

    this->customOrder = builderPtr.getPropertyOrder().toVector();
}

ErrCode PropertyObjectClassImpl::getName(IString** typeName)
{
    OPENDAQ_PARAM_NOT_NULL(typeName);

    *typeName = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getParentName(IString** parentName)
{
    OPENDAQ_PARAM_NOT_NULL(parentName);

    *parentName = this->parent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getProperty(IString* propertyName, IProperty** property)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);
    OPENDAQ_PARAM_NOT_NULL(property);

    auto res = props.find(propertyName);
    if (res == props.cend())
    {
        if (parent.assigned())
        {
            TypeManagerPtr managerPtr;
            ErrCode err = getManager(managerPtr);
            OPENDAQ_RETURN_IF_FAILED(err);

            TypePtr type;
            err = managerPtr->getType(parent, &type);
            OPENDAQ_RETURN_IF_FAILED(err);

            const auto parentClass = type.asPtrOrNull<IPropertyObjectClass>();
            if (!parentClass.assigned())
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);

            return parentClass->getProperty(propertyName, property);
        }

        StringPtr str = propertyName;
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property with name {} not found.)", str));
    }

    *property = res.value().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::hasProperty(IString* propertyName, Bool* hasProperty)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);
    OPENDAQ_PARAM_NOT_NULL(hasProperty);

    const auto res = props.find(propertyName);
    if (res == props.end())
    {
        if (parent.assigned())
        {
            TypeManagerPtr managerPtr;
            ErrCode err = getManager(managerPtr);
            OPENDAQ_RETURN_IF_FAILED(err);

            TypePtr type;
            err = managerPtr->getType(parent, &type);
            OPENDAQ_RETURN_IF_FAILED(err);

            const auto parentClass = type.asPtrOrNull<IPropertyObjectClass>();
            if (!parentClass.assigned())
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDTYPE);

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
        OPENDAQ_RETURN_IF_FAILED(err);

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
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_MANAGER_NOT_ASSIGNED, "Property object manager not assigned.");
    }

    managerPtr = manager.getRef();

    if (!managerPtr.assigned())
    {
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_MANAGER_NOT_ASSIGNED, "Property object manager not assigned.");
    }
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::getWithNormalOrder(Bool includeInherited, IList** list)
{
    ListPtr<IProperty> properties;

    if (includeInherited)
    {
        ErrCode errCode = getInheritedProperties(properties);
        OPENDAQ_RETURN_IF_FAILED(errCode);
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
        OPENDAQ_RETURN_IF_FAILED(errCode);
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
    OPENDAQ_PARAM_NOT_NULL(properties);

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
    OPENDAQ_RETURN_IF_FAILED(errCode);

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
                return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOT_SERIALIZABLE,
                                     std::string("Property \"" + propName + "\" does not implement ISerializable."));
            }

            OPENDAQ_RETURN_IF_FAILED(errCode);

            errCode = serializableProp->serialize(serializer);
            OPENDAQ_RETURN_IF_FAILED(errCode);
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
    OPENDAQ_RETURN_IF_FAILED(errCode);

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
    return daqTry(
        [&serialized, &context, &factoryCallback, &obj]
        {
            TypeManagerPtr typeManager;
            if (context)
            {
                context->queryInterface(ITypeManager::Id, reinterpret_cast<void**>(&typeManager));
            }

            const auto serializedPtr = SerializedObjectPtr::Borrow(serialized);

            const auto name = serializedPtr.readString("name");
            PropertyObjectClassBuilderPtr builder = PropertyObjectClassBuilder(typeManager, name);

            if (serializedPtr.hasKey("parent"))
            {
                const auto parent = serializedPtr.readString("parent");
                builder.setParentName(parent);
            }

            const auto properties = serializedPtr.readSerializedObject("properties");
            const auto keys = properties.getKeys();

            for (const auto& key : keys)
            {
                const PropertyPtr prop = properties.readObject(key, context);
                builder.addProperty(prop);
            }

            PropertyObjectClassPtr serilizedObj = builder.build();

            if (typeManager.assigned())
            {
                typeManager.addType(serilizedObj);
            }
            *obj = serilizedObj.detach();
        });
}

ErrCode PropertyObjectClassImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassImpl::toString(CharPtr* str)
{
    OPENDAQ_PARAM_NOT_NULL(str);

    std::ostringstream stream;
    stream << "PropertyObjectClass {" << name << "}";

    return daqDuplicateCharPtr(stream.str().c_str(), str);
}

ErrCode PropertyObjectClassImpl::clone(IPropertyObjectClass** cloned, ITypeManager* typeManager)
{
    OPENDAQ_PARAM_NOT_NULL(cloned);

    return daqTry([&]
    {
        TypeManagerPtr managerPtr(typeManager);
        if (!managerPtr.assigned())
            managerPtr = this->manager.getRef();

        auto builder = PropertyObjectClassBuilder(managerPtr, this->name);
        builder.setParentName(this->parent);

        for (const auto& [_, prop] : this->props)
            builder.addProperty(prop.asPtr<IPropertyInternal>(true).clone());

        builder.setPropertyOrder(ListPtr<IString>::FromVector(this->customOrder));

        PropertyObjectClassPtr clonedObj = builder.build();
        *cloned = clonedObj.detach();
    });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyObjectClass,
    IPropertyObjectClass, createPropertyObjectClassFromBuilder,
    IPropertyObjectClassBuilder*, builder
)

END_NAMESPACE_OPENDAQ
