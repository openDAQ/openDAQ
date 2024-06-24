#include <coreobjects/unit_impl.h>
#include <coreobjects/unit_ptr.h>
#include <coreobjects/errors.h>
#include <utility>
#include <coreobjects/unit_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr unitStructType = UnitStructType();
}

DictPtr<IString, IBaseObject> UnitImpl::PackBuilder(IUnitBuilder* unitBuilder)
{
    const auto builderPtr = UnitBuilderPtr::Borrow(unitBuilder);
    auto params = Dict<IString, IBaseObject>();
    params.set("Id", builderPtr.getId());
    params.set("Symbol", builderPtr.getSymbol());
    params.set("Name", builderPtr.getName());
    params.set("Quantity", builderPtr.getQuantity());

    return params;
}

UnitImpl::UnitImpl(Int id, StringPtr symbol, StringPtr name, StringPtr quantity)
    : GenericStructImpl<IUnit, IStruct>(
        detail::unitStructType,
        Dict<IString, IBaseObject>(
            {{"Id", id}, {"Symbol", std::move(symbol)}, {"Name", std::move(name)}, {"Quantity", std::move(quantity)}}))
{
}

UnitImpl::UnitImpl(IUnitBuilder* unitBuilder)
    : GenericStructImpl<IUnit, IStruct>(
            detail::unitStructType, PackBuilder(unitBuilder))
{
}


ErrCode UnitImpl::getId(Int* id)
{
    if (!id)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *id = this->fields.get("Id");
    return OPENDAQ_SUCCESS;
}

ErrCode UnitImpl::getSymbol(IString** symbol)
{
    if (!symbol)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *symbol = this->fields.get("Symbol").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode UnitImpl::getName(IString** name)
{
    if (!name)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *name = this->fields.get("Name").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode UnitImpl::getQuantity(IString** quantity)
{
    if (!quantity)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *quantity = this->fields.get("Quantity").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode UnitImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        const StringPtr symbol = this->fields.get("Symbol");
        if (symbol.assigned())
        {
            serializer->key("Symbol");
            serializer->writeString(symbol.getCharPtr(), symbol.getLength());
        }
        
        const Int id = this->fields.get("Id");
        if (id != -1)
        {
            serializer->key("Id");
            serializer->writeInt(id);
        }
        
        const StringPtr name = this->fields.get("Name");
        if (name.assigned())
        {
            serializer->key("Name");
            serializer->writeString(name.getCharPtr(), name.getLength());
        }
        
        const StringPtr quantity = this->fields.get("Quantity");
        if (quantity.assigned())
        {
            serializer->key("Quantity");
            serializer->writeString(quantity.getCharPtr(), quantity.getLength());
        }
    }

    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode UnitImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr UnitImpl::SerializeId()
{
    return "Unit";
}

ErrCode UnitImpl::Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);

    StringPtr symbol;
    ErrCode err = serializedObj->readString(String("Symbol"), &symbol);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    Int id = -1;
    err = serializedObj->readInt(String("Id"), &id);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    StringPtr name;
    err = serializedObj->readString(String("Name"), &name);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    StringPtr quantity;
    err = serializedObj->readString(String("Quantity"), &quantity);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    return createObject<IUnit, UnitImpl, Int, StringPtr, StringPtr, StringPtr>(reinterpret_cast<IUnit**>(obj), id, symbol, name, quantity);
}

/////////////////////
////
//// FACTORIES
////
////////////////////

extern "C"
daq::ErrCode PUBLIC_EXPORT createUnit(IUnit** objTmp,
                                      Int id,
                                      IString* symbol,
                                      IString* name,
                                      IString* quantity)
{
    return daq::createObject<IUnit, UnitImpl>(objTmp, id, symbol, name, quantity);
}

extern "C"
daq::ErrCode PUBLIC_EXPORT createUnitFromBuilder(IUnit** objTmp, IUnitBuilder* unitBuilder)
{
    return daq::createObject<IUnit, UnitImpl>(objTmp, unitBuilder);
}


END_NAMESPACE_OPENDAQ
