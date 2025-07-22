#include <coreobjects/errors.h>
#include <coreobjects/property_internal_ptr.h>
#include <coreobjects/property_object_class_builder_impl.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_ptr.h>
#include <coretypes/type_manager_factory.h>
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

ErrCode PropertyObjectClassBuilderImpl::build(IPropertyObjectClass** propertyObjectClass)
{
    OPENDAQ_PARAM_NOT_NULL(propertyObjectClass);

    const auto builderPtr = this->borrowPtr<PropertyObjectClassBuilderPtr>();

    const ErrCode errCode = daqTry([&]()
    {
        *propertyObjectClass = PropertyObjectClassFromBuilder(builderPtr).detach();
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode);
    return errCode;
}


ErrCode PropertyObjectClassBuilderImpl::setName(IString* className)
{
    this->name = className;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::getName(IString** className)
{
    OPENDAQ_PARAM_NOT_NULL(className);

    *className = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

inline ErrCode PropertyObjectClassBuilderImpl::setParentName(IString* parentName)
{
    this->parent = parentName;
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::getParentName(IString** parentName)
{
    OPENDAQ_PARAM_NOT_NULL(parentName);

    *parentName = this->parent.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::addProperty(IProperty* property)
{
    OPENDAQ_PARAM_NOT_NULL(property);

    const ErrCode errCode = daqTry([this, &property]()
    {
        auto p = PropertyPtr::Borrow(property);

		if (hasDuplicateReferences(p))
			return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_INVALIDVALUE, "Reference property references a property that is already referenced by another.");

        if (props.hasKey(p.getName()))
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ALREADYEXISTS, fmt::format(R"(Property with name {} already exists)", p.getName()));
        props.set(p.getName(), p);

        auto defaultValue = p.asPtr<IPropertyInternal>().getDefaultValueUnresolved();
        if (auto freezable = defaultValue.asPtrOrNull<IFreezable>(); freezable.assigned())
        {
            if (!freezable.isFrozen())
                freezable.freeze();
        }

        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to add property");
    return errCode;
}

ErrCode PropertyObjectClassBuilderImpl::getProperties(IDict** properties)
{
    OPENDAQ_PARAM_NOT_NULL(properties);

    *properties = this->props.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::removeProperty(IString* propertyName)
{
    OPENDAQ_PARAM_NOT_NULL(propertyName);

    const ErrCode errCode = wrapHandler([this, &propertyName]()
    {
        if (!props.hasKey(propertyName))
        {
            return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_NOTFOUND, fmt::format(R"(Property with name '{}' not found.)", StringPtr::Borrow(propertyName)));
        }

        props.remove(propertyName);
        return OPENDAQ_SUCCESS;
    });
    OPENDAQ_RETURN_IF_FAILED(errCode, "Failed to remove property");
    return errCode;
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

ErrCode PropertyObjectClassBuilderImpl::getPropertyOrder(IList** orderedPropertyNames)
{
    OPENDAQ_PARAM_NOT_NULL(orderedPropertyNames);

    *orderedPropertyNames = this->customOrder.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode PropertyObjectClassBuilderImpl::getManager(ITypeManager** manager)
{
    OPENDAQ_PARAM_NOT_NULL(manager);

    if (this->manager.assigned())
        *manager = this->manager.getRef().addRefAndReturn();
    else
        *manager = nullptr;
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
    ListPtr<IProperty> properties = List<IProperty>();

    if (parent.assigned() && manager.assigned())
    {
        const auto managerPtr = manager.getRef();
        if (managerPtr.assigned())
        {
            const auto parentClass = managerPtr.getType(parent).asPtrOrNull<IPropertyObjectClass>();
            if (!parentClass.assigned())
                DAQ_THROW_EXCEPTION(InvalidTypeException, "Type with name {} is not a property object class", parent);

            properties = parentClass.getProperties(True);
        }
    }

    for (const auto& prop : props)
    {
        properties.unsafePushBack(prop.second);
    }

    return properties;
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
