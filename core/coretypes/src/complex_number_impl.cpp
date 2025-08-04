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
        Dict<IString, IFloat>({{"Real", real}, {"Imaginary", imaginary}}))
{
    value.real = this->fields.get("Real");
    value.imaginary = this->fields.get("Imaginary");
}

ErrCode ComplexNumberImpl::getValue(ComplexFloat64* value)
{
    OPENDAQ_PARAM_NOT_NULL(value);

    *value = this->value;
    return OPENDAQ_SUCCESS;
}

ErrCode ComplexNumberImpl::equalsValue(const ComplexFloat64 value, Bool* equals)
{
    if (equals == nullptr)
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equals out-parameter must not be null");

    *equals = this->value == value;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::getReal(Float* real)
{
    OPENDAQ_PARAM_NOT_NULL(real);

    *real = this->value.real;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::getImaginary(Float* imaginary)
{
    OPENDAQ_PARAM_NOT_NULL(imaginary);

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
        return DAQ_MAKE_ERROR_INFO(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null.");
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
    OPENDAQ_PARAM_NOT_NULL(str);

    std::ostringstream os;
    CoreTypeHelper<ComplexFloat64>::Print(os, value);
    return daqDuplicateCharPtr(os.str().c_str(), str);
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::toFloat(Float* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    *val = value.real;
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::toInt(Int* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    *val = Int(value.real);
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::toBool(Bool* val)
{
    OPENDAQ_PARAM_NOT_NULL(val);

    *val = value.real != 0 || value.imaginary != 0;
    return OPENDAQ_SUCCESS;
}

// ICoreType

ErrCode INTERFACE_FUNC ComplexNumberImpl::getCoreType(CoreType* coreType)
{
    OPENDAQ_PARAM_NOT_NULL(coreType);

    *coreType = ctComplexNumber;
    return OPENDAQ_SUCCESS;
}

// IComparable

ErrCode INTERFACE_FUNC ComplexNumberImpl::compareTo(IBaseObject* other)
{
    OPENDAQ_PARAM_NOT_NULL(other);

    IComplexNumber* obj = nullptr;
    ErrCode err = other->borrowInterface(IComplexNumber::Id, reinterpret_cast<void**>(&obj));
    if (OPENDAQ_FAILED(err))
    {
        if (err == OPENDAQ_ERR_NOINTERFACE)
            return DAQ_MAKE_ERROR_INFO(err);
        return DAQ_EXTEND_ERROR_INFO(err);
    }

    ComplexFloat64 otherValue;
    this->getValue(&otherValue);

    if (value > otherValue)
        return OPENDAQ_GREATER;
    if (value < otherValue)
        return OPENDAQ_LOWER;
    return OPENDAQ_EQUAL;
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
    OPENDAQ_PARAM_NOT_NULL(obj);

    ComplexFloat64 complex;
    coretype_utils::read(serializedObj, complex);
    ObjectPtr<IComplexNumber> complexObj = ComplexNumber_Create(complex.real, complex.imaginary);
    *obj = complexObj.detach();
    return OPENDAQ_SUCCESS;
}

ErrCode INTERFACE_FUNC ComplexNumberImpl::getSerializeId(ConstCharPtr* id) const
{
    OPENDAQ_PARAM_NOT_NULL(id);

    *id = SerializeId();
    return OPENDAQ_SUCCESS;
}

ConstCharPtr ComplexNumberImpl::SerializeId()
{
    return "ComplexNumber";
}

END_NAMESPACE_OPENDAQ
