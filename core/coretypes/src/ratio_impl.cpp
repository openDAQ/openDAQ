#include <coretypes/ratio_impl.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/impl.h>
#include <coretypes/float.h>
#include <cmath>
#include <coretypes/ctutils.h>
#include <coretypes/ratio_factory.h>

BEGIN_NAMESPACE_OPENDAQ

RatioImpl::RatioImpl(Int numerator, Int denominator)
    : GenericStructImpl<daq::IRatio, daq::IStruct, daq::IConvertible, daq::IComparable>(
          RatioStructType(), Dict<IString, IInteger>({{"numerator", numerator}, {"denominator", denominator}}))
{
    this->numerator = this->fields.get("numerator");
    this->denominator = this->fields.get("denominator");

    if (this->denominator == 0)
    {
        throw InvalidParameterException("Denominator can not be 0");
    }
}

ErrCode RatioImpl::toString(CharPtr* str)
{
    std::ostringstream s;
    s << numerator << "/" << denominator;
    return daqDuplicateCharPtr(s.str().c_str(), str);
}

ErrCode RatioImpl::getNumerator(Int* numerator)
{
    if (numerator == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *numerator = this->numerator;

    return OPENDAQ_SUCCESS;
}

ErrCode RatioImpl::getDenominator(Int* denominator)
{
    if (denominator == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *denominator = this->denominator;

    return OPENDAQ_SUCCESS;
}

ErrCode RatioImpl::simplify(IRatio** simplifiedRatio)
{
    if (simplifiedRatio == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    Int num = numerator;
    Int den = denominator;
    daq::simplify(num, den);

    *simplifiedRatio = Ratio(num, den).detach();
    return OPENDAQ_SUCCESS;
}

ErrCode RatioImpl::toFloat(Float* val)
{
    if (val == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    *val = numerator / static_cast<Float>(denominator);
    return OPENDAQ_SUCCESS;
}

ErrCode RatioImpl::toInt(Int* val)
{
    if (val == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    Float realValue = -1;
    ErrCode err = toFloat(&realValue);

    if (OPENDAQ_FAILED(err))
    {
        return err;
    }

    *val = static_cast<Int>(round(realValue));
    return OPENDAQ_SUCCESS;
}

ErrCode RatioImpl::toBool(Bool* val)
{
    if (val == nullptr)
    {
        return OPENDAQ_ERR_ARGUMENT_NULL;
    }

    Float realValue = -1;
    ErrCode err = toFloat(&realValue);

    if (OPENDAQ_FAILED(err))
    {
        return err;
    }

    *val = static_cast<Bool>(static_cast<bool>(realValue));
    return OPENDAQ_SUCCESS;
}

ErrCode RatioImpl::getCoreType(CoreType* coreType)
{
    if (coreType == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = ctRatio;
    return OPENDAQ_SUCCESS;
}

ErrCode RatioImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    {
        serializer->key("num");
        serializer->writeInt(numerator);

        serializer->key("den");
        serializer->writeInt(denominator);
    }
    serializer->endObject();

    return OPENDAQ_SUCCESS;
}

ErrCode RatioImpl::getSerializeId(ConstCharPtr* id) const
{
    *id = SerializeId();

    return OPENDAQ_SUCCESS;
}

ConstCharPtr RatioImpl::SerializeId()
{
    return "Ratio";
}

ErrCode RatioImpl::Deserialize(ISerializedObject* serialized, IBaseObject* /*context*/, IFunction* /*factoryCallback*/, IBaseObject** obj)
{
    SerializedObjectPtr serializedObj = SerializedObjectPtr::Borrow(serialized);
    Int num = serializedObj.readInt("num");
    Int den = serializedObj.readInt("den");

    return createObject<IRatio, RatioImpl, Int, Int>(reinterpret_cast<IRatio**>(obj), num, den);
}

ErrCode RatioImpl::equals(IBaseObject *other, Bool* equal) const
{
    if (equal == nullptr)
    {
        return daq::makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null.", nullptr);
    }

    *equal = false;
    if (!other)
    {
        return OPENDAQ_SUCCESS;
    }

    RatioPtr ratioOther = BaseObjectPtr::Borrow(other).asPtrOrNull<IRatio>();
    if (ratioOther == nullptr)
        return OPENDAQ_SUCCESS;

    Int num = numerator;
    Int den = denominator;
    daq::simplify(num, den);

    const RatioPtr simplifiedOther = ratioOther.simplify();

    if (num != simplifiedOther.getNumerator())
        return OPENDAQ_SUCCESS;

    if (den != simplifiedOther.getDenominator())
        return OPENDAQ_SUCCESS;

    *equal = true;
    return OPENDAQ_SUCCESS;
}

ErrCode RatioImpl::compareTo(IBaseObject* obj)
{
    if (obj == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    IConvertible* objConvIntf;
    Float objFloatValue;

    auto err = obj->borrowInterface(IConvertible::Id, reinterpret_cast<void**>(&objConvIntf));
    if (OPENDAQ_FAILED(err))
        return err;

    err = objConvIntf->toFloat(&objFloatValue);
    if (OPENDAQ_FAILED(err))
        return err;

    Float thisFloatValue;
    err = toFloat(&thisFloatValue);
    if (OPENDAQ_FAILED(err))
        return err;

    const auto diff = thisFloatValue - objFloatValue;

    if (diff > 0)
        err = OPENDAQ_GREATER;
    else if (diff < 0)
        err = OPENDAQ_LOWER;
    else
        err = OPENDAQ_EQUAL;

    return err;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, Ratio, Int, num, Int, den)

END_NAMESPACE_OPENDAQ
