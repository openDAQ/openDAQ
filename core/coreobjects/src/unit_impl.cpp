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
    params.set("id", builderPtr.getId());
    params.set("symbol", builderPtr.getSymbol());
    params.set("name", builderPtr.getName());
    params.set("quantity", builderPtr.getQuantity());

    return params;
}

UnitImpl::UnitImpl(Int id, StringPtr symbol, StringPtr name, StringPtr quantity)
    : GenericStructImpl<IUnit, IStruct>(
        detail::unitStructType,
        Dict<IString, IBaseObject>(
            {{"id", id}, {"symbol", std::move(symbol)}, {"name", std::move(name)}, {"quantity", std::move(quantity)}}))
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

    *id = this->fields.get("id");
    return OPENDAQ_SUCCESS;
}

ErrCode UnitImpl::getSymbol(IString** symbol)
{
    if (!symbol)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *symbol = this->fields.get("symbol").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode UnitImpl::getName(IString** name)
{
    if (!name)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *name = this->fields.get("name").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode UnitImpl::getQuantity(IString** quantity)
{
    if (!quantity)
        return OPENDAQ_ERR_ARGUMENT_NULL;
    
    *quantity = this->fields.get("quantity").asPtr<IString>().addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode UnitImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        const StringPtr symbol = this->fields.get("symbol");
        if (symbol.assigned())
        {
            serializer->key("symbol");
            serializer->writeString(symbol.getCharPtr(), symbol.getLength());
        }
        
        const Int id = this->fields.get("id");
        if (id != -1)
        {
            serializer->key("id");
            serializer->writeInt(id);
        }
        
        const StringPtr name = this->fields.get("name");
        if (name.assigned())
        {
            serializer->key("name");
            serializer->writeString(name.getCharPtr(), name.getLength());
        }
        
        const StringPtr quantity = this->fields.get("quantity");
        if (quantity.assigned())
        {
            serializer->key("quantity");
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
    ErrCode err = serializedObj->readString(String("symbol"), &symbol);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    Int id = -1;
    err = serializedObj->readInt(String("id"), &id);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    StringPtr name;
    err = serializedObj->readString(String("name"), &name);
    if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_NOTFOUND)
        return err;

    StringPtr quantity;
    err = serializedObj->readString(String("quantity"), &quantity);
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
