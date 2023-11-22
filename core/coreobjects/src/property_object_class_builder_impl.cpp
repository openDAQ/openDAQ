#include <coreobjects/property_object_class_builder_impl.h>
#include <coretypes/type_manager_factory.h>
#include <coreobjects/property_ptr.h>
#include <coreobjects/errors.h>
#include <coreobjects/property_internal_ptr.h>
#include <coreobjects/property_object_class_factory.h>

#include <utility>

BEGIN_NAMESPACE_OPENDAQ

PropertyObjectClassBuilderImpl::PropertyObjectClassBuilderImpl(StringPtr name)
    : name(std::move(name))
    , props(Dict<IString, IProperty>())
    , customOrder(List<IString>())
{ 
}

PropertyObjectClassBuilderImpl::PropertyObjectClassBuilderImpl(const TypeManagerPtr& manager, StringPtr name)
    : PropertyObjectClassBuilderImpl(std::move(name))
{
    this->manager = manager;
}

ErrCode PropertyObjectClassBuilderImpl::setName(IString* className)
{
    this->name = className;
    return OPENDAQ_SUCCESS;
}


inline ErrCode PropertyObjectClassBuilderImpl::setParentName(IString* parentName)
{
    this->parent = parentName;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::addProperty(IProperty* property)
{
    if (property == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;


    return wrapHandler([this, &property]()
    {
        auto p = PropertyPtr::Borrow(property);

		if (hasDuplicateReferences(p))
			return this->makeErrorInfo(OPENDAQ_ERR_INVALIDVALUE, "Reference property references a property that is already referenced by another.");

        if (props.hasKey(p.getName()))
            return makeErrorInfo(OPENDAQ_ERR_ALREADYEXISTS, fmt::format(R"(Property with name {} already exists)", p.getName()));
        props.set(p.getName(), p);

        return OPENDAQ_SUCCESS;
    });
}

ErrCode PropertyObjectClassBuilderImpl::removeProperty(IString* propertyName)
{
    if (propertyName == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return wrapHandler([this, &propertyName]()
    {
        if (!props.hasKey(propertyName))
        {
            return makeErrorInfo(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property with name '{}' not  found.)", StringPtr::Borrow(propertyName)));
        }

        props.remove(propertyName);
        return OPENDAQ_SUCCESS;
    });
}

ErrCode PropertyObjectClassBuilderImpl::setPropertyOrder(IList* orderedPropertyNames)
{
    customOrder.clear();
    if (orderedPropertyNames != nullptr)
    {
        for (auto&& propName : ListPtr<IString>::Borrow(orderedPropertyNames))
        {
            customOrder.pushBack(propName);
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::getName(IString** className)
{
    if (!className)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *className = this->name;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::getParentName(IString** parentName)
{
    if (!parentName)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *parentName = this->parent;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::getProperties(IDict** properties)
{
    if (!properties)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *properties = this->props;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::getPropertyOrder(IList** orderedPropertyNames)
{
    if (!orderedPropertyNames)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *orderedPropertyNames = this->customOrder;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::getManager(ITypeManager** manager)
{
    if (!manager)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    if (this->manager.assigned() && this->manager.getRef().assigned())
    {    
       *manager = this->manager.getRef();
       return OPENDAQ_SUCCESS;
    }

    return OPENDAQ_ERR_ARGUMENT_NULL;
}

ErrCode PropertyObjectClassBuilderImpl::build(IPropertyObjectClass** propertyObjectClass)
{
    if (propertyObjectClass == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *propertyObjectClass = PropertyObjectClassFromBuildParams(packBuildParams()).detach();
    return OPENDAQ_SUCCESS;
}

bool PropertyObjectClassBuilderImpl::hasDuplicateReferences(const PropertyPtr& prop) const
{
    if (const auto refEval = prop.asPtr<IPropertyInternal>().getReferencedPropertyUnresolved(); refEval.assigned())
    {
        const auto refNames = refEval.getPropertyReferences();
        std::unordered_set<std::string> refNamesSet;
        for (auto refName : refNames)
            refNamesSet.insert(refName);
        
        for (auto ownProp : getProperties())
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

ListPtr<IProperty> PropertyObjectClassBuilderImpl::getProperties() const
{
    ListPtr<IProperty> properties = List<IProperty>();;

    if (parent.assigned() && manager.assigned())
    {
        const auto managerPtr = manager.getRef();
        if (managerPtr.assigned())
        {
            const auto parentClass = managerPtr.getType(parent).asPtrOrNull<IPropertyObjectClass>();
            if (!parentClass.assigned())
                throw InvalidTypeException{"Type with name {} is not a property object class", parent};

            properties = parentClass.getProperties(True);
        }
    }

    for (const auto& prop : props)
    {
        properties.unsafePushBack(prop.second);
    }

    return properties;
}

DictPtr<IString, IBaseObject> PropertyObjectClassBuilderImpl::packBuildParams()
{
    auto buildParams = Dict<IString, IBaseObject>();
    buildParams.set("name", name);
    buildParams.set("parent", parent);
    buildParams.set("props", props);
    buildParams.set("customOrder", customOrder);
    if (manager.assigned() && manager.getRef().assigned())
        buildParams.set("manager", manager.getRef());
    else
        buildParams.set("manager", nullptr);

    return buildParams;
}


OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(
    LIBRARY_FACTORY,
    PropertyObjectClassBuilder,
    IPropertyObjectClassBuilder,
    IString*,
    name
    )

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyObjectClassBuilder,
    IPropertyObjectClassBuilder, createPropertyObjectClassBuilderWithManager,
    ITypeManager*, manager,
    IString*, name
)

END_NAMESPACE_OPENDAQ
