#include <coreobjects/errors.h>
#include <coreobjects/unit_builder_impl.h>
#include <coreobjects/unit_builder_ptr.h>
#include <coreobjects/unit_ptr.h>
#include <utility>

BEGIN_NAMESPACE_OPENDAQ

UnitBuilderImpl::UnitBuilderImpl()
    : UnitBuilderImpl(-1, "", "", "")
{
}

UnitBuilderImpl::UnitBuilderImpl(const UnitPtr& unitToCopy)
    : UnitBuilderImpl(unitToCopy.getId(), unitToCopy.getSymbol(), unitToCopy.getName(), unitToCopy.getQuantity())
{
}

UnitBuilderImpl::UnitBuilderImpl(Int id, StringPtr symbol, StringPtr name, StringPtr quantity)
    : id(id)
    , symbol(std::move(symbol))
    , name(std::move(name))
    , quantity(std::move(quantity))
{
}

ErrCode UnitBuilderImpl::build(IUnit** unit)
{
    OPENDAQ_PARAM_NOT_NULL(unit);

    const auto builderPtr = this->borrowPtr<UnitBuilderPtr>();

    return daqTry([&]()
    {
        *unit = UnitFromBuilder(builderPtr).detach();
        return OPENDAQ_SUCCESS;
    });
}

ErrCode UnitBuilderImpl::setId(Int id)
{
    this->id = id;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::getId(Int* id)
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = this->id;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::setSymbol(IString* symbol)
{
    this->symbol = symbol;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::getSymbol(IString** symbol)
{
    OPENDAQ_PARAM_NOT_NULL(symbol);

    *symbol = this->symbol.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::getName(IString** name)
{
    OPENDAQ_PARAM_NOT_NULL(name);

    *name = this->name.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::setQuantity(IString* quantity)
{
    this->quantity = quantity;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::getQuantity(IString** quantity)
{
    OPENDAQ_PARAM_NOT_NULL(quantity);
    
    *quantity = this->quantity.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C" ErrCode PUBLIC_EXPORT createUnitBuilder(IUnitBuilder** objTmp)
{
    return daq::createObject<IUnitBuilder, UnitBuilderImpl>(objTmp);
}

extern "C" ErrCode PUBLIC_EXPORT createUnitBuilderFromExisting(IUnitBuilder** objTmp, IUnit* unitToCopy)
{
    return daq::createObject<IUnitBuilder, UnitBuilderImpl>(objTmp, unitToCopy);
}

END_NAMESPACE_OPENDAQ
