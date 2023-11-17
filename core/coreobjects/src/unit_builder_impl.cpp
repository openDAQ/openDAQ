#include <coreobjects/unit_builder_impl.h>
#include <coreobjects/unit_ptr.h>
#include <coreobjects/errors.h>
#include <utility>
#include <coreobjects/unit_builder_ptr.h>

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

ErrCode UnitBuilderImpl::setId(Int id)
{
    this->id = id;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::setSymbol(IString* symbol)
{
    this->symbol = symbol;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::setName(IString* name)
{
    this->name = name;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::setQuantity(IString* quantity)
{
    this->quantity = quantity;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::getId(Int* id)
{
    if (!id)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *id = this->id;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::getSymbol(IString** symbol)
{
    if (!symbol)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *symbol = this->symbol;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::getName(IString** name)
{
    if (!name)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *name = this->name;
    return OPENDAQ_SUCCESS;
}

ErrCode UnitBuilderImpl::getQuantity(IString** quantity)
{
    if (!quantity)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *quantity = this->quantity;
    return OPENDAQ_SUCCESS;
}


ErrCode UnitBuilderImpl::build(IUnit** unit)
{
    if (unit == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return daqTry([&]()
    {
        *unit = Unit(symbol, id, name, quantity).detach();
        return OPENDAQ_SUCCESS;
    });
}


/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C"
ErrCode PUBLIC_EXPORT createUnitBuilder(IUnitBuilder** objTmp)
{
    return daq::createObject<IUnitBuilder, UnitBuilderImpl>(objTmp);
}

extern "C" 
ErrCode PUBLIC_EXPORT createUnitBuilderFromExisting(IUnitBuilder** objTmp, IUnit* unitToCopy)
{
    return daq::createObject<IUnitBuilder, UnitBuilderImpl>(objTmp, unitToCopy);
}

END_NAMESPACE_OPENDAQ
