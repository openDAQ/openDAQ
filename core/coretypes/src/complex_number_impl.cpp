#include <coretypes/complex_number_impl.h>
#include <coretypes/impl.h>
#include <coretypes/coretype_traits.h>
#include <coretypes/coretype_utils.h>
#include <coretypes/complex_number_factory.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, ComplexNumber, const Float, real, const Float, imaginary)

template <typename T>
static void hashCombine(size_t& seed, const T& value)
{
    std::hash<T> hasher;
    seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

ComplexNumberImpl::ComplexNumberImpl(const Float real, const Float imaginary)
    : GenericStructImpl<IComplexNumber, IStruct, IComparable, IConvertible>(
        ComplexNumberStructType(),
        Dict<IString, IFloat>({{"real", real}, {"imaginary", imaginary}}))
{
    value.real = this->fields.get("real");
    value.imaginary = this->fields.get("imaginary");
}

ErrCode ComplexNumberImpl::getValue(ComplexFloat64* value)
{
    if (value == nullptr)
        return OPENDAQ_ERR_NOTIMPLEMENTED;

    *value = this->value;
    return OPENDAQ_SUCCESS;
}

ErrCode ComplexNumberImpl::equalsValue(const ComplexFloat64 value, Bool* equals)
{
    if (equals == nullptr)
        return makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = this->value == value;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::getReal(Float* real)
{
    if (real == nullptr)
        return OPENDAQ_ERR_NOTIMPLEMENTED;

    *real = this->value.real;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::getImaginary(Float* imaginary)
{
    if (imaginary == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *imaginary = this->value.imaginary;
    return OPENDAQ_SUCCESS;
}

// IBaseObject

ErrCode INTERFACE_FUNC ComplexNumberImpl::getHashCode(SizeT* hashCode)
{
    size_t hash = 0;
    hashCombine(hash, this->value.real);
    hashCombine(hash, this->value.imaginary);
    *hashCode = hash;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::equals(IBaseObject* other, Bool* equals) const
{
    if (equals == nullptr)
    {
        return daq::makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null.", nullptr);
    }

    *equals = false;

    IComplexNumber* obj = nullptr;
    if (OPENDAQ_SUCCEEDED(other->borrowInterface(IComplexNumber::Id, reinterpret_cast<void**>(&obj))))
    {
        ComplexFloat64 value{};
        obj->getValue(&value);

        *equals = value == this->value;
    }

    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::toString(CharPtr* str)
{
    if (str == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    std::ostringstream os;
    CoreTypeHelper<ComplexFloat64>::Print(os, value);
    return daqDuplicateCharPtr(os.str().c_str(), str);
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::toFloat(Float* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *val = value.real;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::toInt(Int* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *val = Int(value.real);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::toBool(Bool* val)
{
    if (val == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *val = value.real != 0 || value.imaginary != 0;
    return OPENDAQ_SUCCESS;
}

// ICoreType

ErrCode INTERFACE_FUNC ComplexNumberImpl::getCoreType(CoreType* coreType)
{
    if (coreType == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *coreType = ctComplexNumber;
    return OPENDAQ_SUCCESS;
}

// IComparable

ErrCode INTERFACE_FUNC ComplexNumberImpl::compareTo(IBaseObject* other)
{
    if (other == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    IComplexNumber* obj = nullptr;
    ErrCode err = other->borrowInterface(IComplexNumber::Id, reinterpret_cast<void**>(&obj));
    if (err)
        return err;

    ComplexFloat64 otherValue;
    this->getValue(&otherValue);

    if (value > otherValue)
        return  OPENDAQ_GREATER;
    if (value < otherValue)
        return  OPENDAQ_LOWER;
    return  OPENDAQ_EQUAL;
}

// ISerializable

ErrCode INTERFACE_FUNC ComplexNumberImpl::serialize(ISerializer* serializer)
{
    serializer->startTaggedObject(this);
    serializer->key("real");
    serializer->writeFloat(value.real);
    serializer->key("imaginary");
    serializer->writeFloat(value.imaginary);
    serializer->endObject();
    return OPENDAQ_SUCCESS;
}

ErrCode ComplexNumberImpl::Deserialize(ISerializedObject* serializedObj,
                                       IBaseObject* /*context*/,
                                       IFunction* /*factoryCallback*/,
                                       IBaseObject** obj)
{
    if (obj == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    ComplexFloat64 complex;
    coretype_utils::read(serializedObj, complex);
    ObjectPtr<IComplexNumber> complexObj = ComplexNumber_Create(complex.real, complex.imaginary);
    *obj = complexObj.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::getSerializeId(ConstCharPtr* id) const
{
    if (id == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr ComplexNumberImpl::SerializeId()
{
    return "ComplexNumber";
}

END_NAMESPACE_OPENDAQ
