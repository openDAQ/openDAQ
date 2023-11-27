#include <coreobjects/unit_factory.h>
#include <coretypes/validation.h>
#include <opendaq/dimension_builder_impl.h>
#include <opendaq/dimension_factory.h>
#include <opendaq/dimension_ptr.h>
#include <opendaq/dimension_rule_ptr.h>
#include <opendaq/range_ptr.h>
#include <opendaq/signal_errors.h>

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

ErrCode DimensionBuilderImpl::build(IDimension** dimension)
{
    OPENDAQ_PARAM_NOT_NULL(dimension);

    const auto builderPtr = this->borrowPtr<DimensionBuilderPtr>();

    return daqTry(
        [&]()
        {
            *dimension = DimensionFromBuilder(builderPtr).detach();
            return OPENDAQ_SUCCESS;
        });
}

ErrCode DimensionBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionBuilderImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionBuilderImpl::setUnit(IUnit* unit)
{
    this->unit = unit;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionBuilderImpl::getUnit(IUnit** unit)
{
    OPENDAQ_PARAM_NOT_NULL(unit);

    *unit = this->unit.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionBuilderImpl::setRule(IDimensionRule* rule)
{
    this->rule = rule;
    return OPENDAQ_SUCCESS;
}

ErrCode DimensionBuilderImpl::getRule(IDimensionRule** rule)
{
    OPENDAQ_PARAM_NOT_NULL(rule);

    *rule = this->rule.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE(LIBRARY_FACTORY, DimensionBuilder, IDimensionBuilder)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, DimensionBuilder, IDimensionBuilder, createDimensionBuilderFromExisting,
    IDimension*, dimensionToCopy
)

END_NAMESPACE_OPENDAQ
