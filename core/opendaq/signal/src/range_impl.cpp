#include <opendaq/range_impl.h>
#include <opendaq/range_ptr.h>
#include <opendaq/signal_exceptions.h>
#include <coretypes/validation.h>
#include <opendaq/range_factory.h>

BEGIN_NAMESPACE_OPENDAQ

namespace detail
{
    static const StructTypePtr rangeStructType = RangeStructType();
}

RangeImpl::RangeImpl(NumberPtr lowValue, NumberPtr highValue)
    : GenericStructImpl<IRange, IStruct>(detail::rangeStructType,
                                         Dict<IString, IBaseObject>({{"lowValue", std::move(lowValue)}, {"highValue", std::move(highValue)}}))
    , low(this->fields.get("lowValue"))
    , high(this->fields.get("highValue"))
{
    if (low > high)
        throw RangeBoundariesInvalidException{};
}

ErrCode RangeImpl::getLowValue(INumber** value)
{
    if (!value)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *value = low.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode RangeImpl::getHighValue(INumber** value)
{
    if (!value)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *value = high.addRefAndReturn();
    return OPENDAQ_SUCCESS;
}

ErrCode RangeImpl::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
        return this->makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = false;
    if (other == nullptr)
        return OPENDAQ_SUCCESS;

    RangePtr rangeOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IRange>();
    if (rangeOther == nullptr)
        return OPENDAQ_SUCCESS;

    *equals = rangeOther.getHighValue() == high &&
              rangeOther.getLowValue() == low;

    return OPENDAQ_SUCCESS;
}

ErrCode RangeImpl::serialize(ISerializer* serializer)
{
    OPENDAQ_PARAM_NOT_NULL(serializer);

    serializer->startTaggedObject(this);
    {
        serializer->key("low");
        low.serialize(serializer);

        serializer->key("high");
        high.serialize(serializer);
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode RangeImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr RangeImpl::SerializeId()
{
    return "Range";
}

ErrCode RangeImpl::Deserialize(ISerializedObject* serialized, IBaseObject*, IFunction*, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);
    NumberPtr low, high;
    const auto lowType = serializedObj.getType("low");
    switch (lowType)
    {
        case ctInt:
            low = serializedObj.readInt("low");
            break;
        case ctFloat:
            low = serializedObj.readFloat("low");
            break;
        default:
            throw InvalidTypeException();
    }
    const auto highType = serializedObj.getType("high");
    switch (highType)
    {
        case ctInt:
            high = serializedObj.readInt("high");
            break;
        case ctFloat:
            high = serializedObj.readFloat("high");
            break;
        default:
            throw InvalidTypeException();
    }

    return createObject<IRange, RangeImpl>(reinterpret_cast<IRange**>(obj), low, high);
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Range, INumber*, lowValue, INumber*, highValue)

END_NAMESPACE_OPENDAQ
