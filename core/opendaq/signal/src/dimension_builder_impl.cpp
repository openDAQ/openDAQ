#include <opendaq/dimension_builder_impl.h>
#include <opendaq/signal_errors.h>
#include <opendaq/range_ptr.h>
#include <opendaq/dimension_ptr.h>
#include <opendaq/dimension_rule_ptr.h>
#include <coreobjects/unit_factory.h>
#include <coretypes/validation.h>
#include <opendaq/dimension_factory.h>

BEGIN_NAMESPACE_OPENDAQ
    DimensionBuilderImpl::DimensionBuilderImpl()
    : unit(nullptr)
    , rule(nullptr)
{
}

DimensionBuilderImpl::DimensionBuilderImpl(const DimensionPtr& copy)
    : name(copy.getName())
    , unit(copy.getUnit())
    , rule(copy.getRule())
{
}

ErrCode DimensionBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionBuilderImpl::setUnit(IUnit* unit)
{
    this->unit = unit;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionBuilderImpl::setRule(IDimensionRule* rule)
{
    this->rule = rule;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionBuilderImpl::build(IDimension** dimension)
{
    OPENDAQ_PARAM_NOT_NULL(dimension);

    return daqTry(
        [&]()
        {
            auto dimensionObj = Dimension(rule, unit, name);
            *dimension = dimensionObj.detach();
            return OPENDAQ_SUCCESS;
        });
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DimensionBuilder, IDimensionBuilder)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, DimensionBuilder, IDimensionBuilder, createDimensionBuilderFromExisting,
    IDimension*, dimensionToCopy
)

END_NAMESPACE_OPENDAQ
