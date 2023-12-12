#include <coretypes/stringobject_impl.h>
#include <coretypes/errors.h>
#include <coretypes/impl.h>
#include <cstring>

BEGIN_NAMESPACE_OPENDAQ

StringImpl::StringImpl(ConstCharPtr data, SizeT length)
    : hashCode(0)
    , hashCalculated(false)
{
    if (data == nullptr)
    {
        this->str = nullptr;
    }
    else
    {
        this->str = new char[length + 1];

        memcpy(this->str, data, length);
        this->str[length] = '\0';
    }
}

StringImpl::StringImpl(ConstCharPtr str)
    : StringImpl(str, str == nullptr ? 0 : strlen(str))
{
}

StringImpl::~StringImpl()
{
    if (str != nullptr)
    {
        delete[] str;
        str = nullptr;
    }
}

ErrCode StringImpl::getHashCode(SizeT* hashCode)
{
    if (str == nullptr)
    {
        *hashCode = 0;
        return OPENDAQ_SUCCESS;
    }

    if (!hashCalculated)
    {
        uint32_t h = 0, high;
        char* s = str;

        while (*s)
        {
            h = (h << 4) + *s++;
            if ((high = h & 0xF0000000))
                h ^= high >> 24;
            h &= ~high;
        }
        this->hashCode = h;
        hashCalculated = true;
    }

    *hashCode = this->hashCode;
    return OPENDAQ_SUCCESS;
}

ErrCode StringImpl::equals(IBaseObject* other, Bool* equal) const
{
    if (equal == nullptr)
    {
        return daq::makeErrorInfo(OPENDAQ_ERR_ARGUMENT_NULL, "Equal output parameter must not be null.", nullptr);
    }

    if (other == nullptr)
    {
        *equal = false;
        return OPENDAQ_SUCCESS;
    }

    *equal = false;
    IString* otherString;

    if (OPENDAQ_SUCCEEDED(other->borrowInterface(IString::Id, reinterpret_cast<void**>(&otherString))))
    {
        ConstCharPtr otherValue;
        auto err = otherString->getCharPtr(&otherValue);

        if (OPENDAQ_FAILED(err))
        {
            return OPENDAQ_SUCCESS;
        }

        if (otherValue == nullptr)
        {
            *equal = str == nullptr;
        }
        else
        {
            *equal = strcmp(str, otherValue) == 0;
        }
    }

    return OPENDAQ_SUCCESS;
}

ErrCode StringImpl::getCharPtr(ConstCharPtr* value)
{
    *value = str;
    return OPENDAQ_SUCCESS;
}

ErrCode StringImpl::getLength(SizeT* size)
{
    if (str != nullptr)
        *size = strlen(str);
    else
        *size = 0;

    return OPENDAQ_SUCCESS;
}

ErrCode StringImpl::toString(CharPtr* str)
{
    if (str == nullptr)
        return OPENDAQ_ERR_ARGUMENT_NULL;

    return daqDuplicateCharPtr(this->str, str);
}

ErrCode StringImpl::toFloat(Float* val)
{
    try
    {
        *val = std::stod(std::string(str));
        return OPENDAQ_SUCCESS;
    }
    catch (const std::exception&)
    {
        return OPENDAQ_ERR_CONVERSIONFAILED;
    }
}

ErrCode StringImpl::toInt(Int* val)
{
    try
    {
        *val = std::stoll(std::string(str));
        return OPENDAQ_SUCCESS;
    }
    catch (const std::exception&)
    {
        return OPENDAQ_ERR_CONVERSIONFAILED;
    }
}

ErrCode StringImpl::toBool(Bool* val)
{
    if (str == nullptr || strlen(str) == 0)
        *val = False;
#if defined(_WIN32)
    else if (_stricmp("True", str) == 0)
#else
    else if (strcasecmp("True", str) == 0)
#endif
        *val = True;
    else
    {
        Int intVal;
        ErrCode errCode = toInt(&intVal);
        if (OPENDAQ_SUCCEEDED(errCode))
            *val = intVal != 0 ? True : False;
        else
            *val = False;
    }
    return OPENDAQ_SUCCESS;
}

ErrCode StringImpl::getCoreType(CoreType* coreType)
{
    *coreType = ctString;
    return OPENDAQ_SUCCESS;
}

ErrCode StringImpl::compareTo(IBaseObject* obj)
{
    if (obj == nullptr)
        return  OPENDAQ_LOWER;

    ConstCharPtr otherValue = nullptr;
    CharPtr otherValueOwned = nullptr;

    IString* otherString = nullptr;
    ErrCode err = obj->borrowInterface(IString::Id, reinterpret_cast<void**>(&otherString));
    if (OPENDAQ_FAILED(err))
    {
        err = obj->toString(&otherValueOwned);
        if (OPENDAQ_FAILED(err))
            return err;
        otherValue = otherValueOwned;
    }
    else
    {
        err = otherString->getCharPtr(&otherValue);
        if (OPENDAQ_FAILED(err))
            return err;
    }

    int r = strcmp(str, otherValue);
    if (r > 0)
        err = OPENDAQ_GREATER;
    else if (r < 0)
        err = OPENDAQ_LOWER;
    else
        err = OPENDAQ_EQUAL;

    if (otherValueOwned)
        daqFreeMemory(otherValueOwned);

    return err;
}

ErrCode StringImpl::getSerializeId(ConstCharPtr* /*id*/) const
{
    return OPENDAQ_ERR_NOTIMPLEMENTED;
}

ErrCode StringImpl::serialize(ISerializer* serializer)
{
    SizeT length;
    ErrCode errCode = getLength(&length);

    if (OPENDAQ_FAILED(errCode))
    {
        return errCode;
    }

    serializer->writeString(str, length);

    return OPENDAQ_SUCCESS;
}

OPENDAQ_DEFINE_CLASS_FACTORY(LIBRARY_FACTORY, String, ConstCharPtr, str)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC_OBJ(
    LIBRARY_FACTORY, StringImpl, IString, createStringN,
    ConstCharPtr, str,
    SizeT, length
)

END_NAMESPACE_OPENDAQ
