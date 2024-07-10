/*
 * Copyright 2022-2024 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <coretypes/common.h>
#include <coretypes/convertible.h>
#include <coretypes/coretype.h>
#include <coretypes/ctutils.h>
#include <coretypes/integer.h>
#include <coretypes/float.h>
#include <coretypes/boolean.h>
#include <coretypes/stringobject.h>
#include <coretypes/complex_number.h>
#include <coretypes/number.h>
#include "ctutils.h"
#include <sstream>
#include <codecvt>
#include <locale>
#include <functional>

BEGIN_NAMESPACE_OPENDAQ

template <typename, typename = std::void_t<>>
struct HasIID : std::false_type
{
};

template <typename T>
struct HasIID<T, std::void_t<typename T::ThisIIID>> : std::true_type
{
};

template <typename T, typename Enable = void>
struct CoreTypeHelper
{
    typedef IBaseObject Interface;

    static ErrCode FromConvertible(T& /*val*/, IConvertible* /*convObj*/)
    {
        return OPENDAQ_ERR_CONVERSIONFAILED;
    }

    static void Print(std::ostringstream& os, const T& val)
    {
        os << val;
    }

    static CoreType GetCoreType()
    {
        return ctUndefined;
    }
};

template <>
struct CoreTypeHelper<Bool>
{
    typedef IBoolean Interface;
    typedef Bool TrueType;

    static ErrCode FromConvertible(Bool& val, IConvertible* convObj)
    {
        ErrCode err = convObj->toBool(&val);
        return err;
    }

    static void Print(std::ostringstream& os, Bool val)
    {
        if (val)
            os << "True";
        else
            os << "False";
    }

    static CoreType GetCoreType()
    {
        return ctBool;
    }

    static IBoolean* Create(Bool value)
    {
        return Boolean_Create(value);
    }
};

template <>
struct CoreTypeHelper<bool>
{
    typedef IBoolean Interface;
    typedef Bool TrueType;

    static ErrCode FromConvertible(bool& val, IConvertible* convObj)
    {
        Bool valBool;
        auto err = convObj->toBool(&valBool);
        val = IsTrue(valBool);
        return err;
    }

    static void Print(std::ostringstream& os, bool val)
    {
        if (val)
            os << "True";
        else
            os << "False";
    }

    static CoreType GetCoreType()
    {
        return ctBool;
    }

    static IBoolean* Create(bool value)
    {
        return Boolean_Create(value ? True : False);
    }
};

template <typename T>
struct CoreTypeHelper<T, typename std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
{
    typedef IInteger Interface;
    typedef Int TrueType;

    static ErrCode FromConvertible(T& val, IConvertible* convObj)
    {
        Int intVal;
        ErrCode err = convObj->toInt(&intVal);
        val = (T) intVal;
        return err;
    }

    static void Print(std::ostringstream& os, T val)
    {
        os << val;
    }

    static CoreType GetCoreType()
    {
        return ctInt;
    }

    static IInteger* Create(T value)
    {
        return Integer_Create((Int) value);
    }
};

template <typename T>
struct CoreTypeHelper<T, typename std::enable_if_t<std::is_floating_point_v<T>>>
{
    typedef IFloat Interface;
    typedef Float TrueType;

    static ErrCode FromConvertible(T& val, IConvertible* convObj)
    {
        Float floatVal;
        ErrCode err = convObj->toFloat(&floatVal);
        val = static_cast<T>(floatVal);
        return err;
    }

    static void Print(std::ostringstream& os, T& val)
    {
        os << val;
    }

    static CoreType GetCoreType()
    {
        return ctFloat;
    }

    static IFloat* Create(T value)
    {
        return Float_Create(value);
    }
};

template <typename T>
struct CoreTypeHelper<Complex_Number<T>>
{
    typedef IComplexNumber Interface;
    typedef Complex_Number<T> TrueType;

    static ErrCode FromConvertible(Complex_Number<T>& val, IConvertible* convObj)
    {
        Float floatVal;
        ErrCode err = convObj->toFloat(&floatVal);
        val = static_cast<T>(floatVal);
        return err;
    }

    static void Print(std::ostream& os, const Complex_Number<T>& val)
    {
        os << "(" << val.real << ", " << val.imaginary << ")";
    }

    static CoreType GetCoreType()
    {
        return ctComplexNumber;
    }

    static IComplexNumber* Create(const ComplexFloat32& value)
    {
        return ComplexNumber_Create(value.real, value.imaginary);
    }

    static IComplexNumber* Create(const ComplexFloat64& value)
    {
        return ComplexNumber_Create(value.real, value.imaginary);
    }
};

template struct CoreTypeHelper<ComplexFloat32>;
template struct CoreTypeHelper<ComplexFloat64>;

template <>
struct CoreTypeHelper<std::wstring>
{
    typedef IString Interface;
    typedef std::wstring TrueType;

    /*  static ErrCode FromConvertible(std::wstring& val, IConvertible* convObj)
        {
            SizeT maxLen;

            ErrCode err = convObj->toString(nullptr, &maxLen);
            if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_SIZETOOSMALL)
                return err;

            CharPtr s = new (std::nothrow) char[maxLen + 1];
            if (s == nullptr)
                return OPENDAQ_ERR_NOMEMORY;

            err = convObj->toString(s, &maxLen);
            if (OPENDAQ_FAILED(err))
            {
                delete[] s;
                return err;
            }

            val = stringToWstring(s);
            delete[] s;

            return OPENDAQ_SUCCESS;
        }*/

    static void Print(std::ostringstream& os, const std::wstring& val)
    {
        os << wstringToString(val);
    }

    static CoreType GetCoreType()
    {
        return ctString;
    }

    static IString* Create(const std::wstring& value)
    {
        return String_Create(wstringToString(value).c_str());
    }

    static std::wstring ToWString(IString* string)
    {
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

        ConstCharPtr data;
        ErrCode err = string->getCharPtr(&data);
        checkErrorInfo(err);

        return converter.from_bytes(data);
    }

    static std::wstring stringToWString(const std::string& t_str)
    {
#ifdef __MINGW32__
        // workaround for mingw bug
        size_t l = t_str.length() * 2 + 1;

    #ifdef __GNUC__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wvla"
    #endif

        wchar_t dest[l];

    #ifdef __GNUC__
        #pragma GCC diagnostic pop
    #endif

        mbstowcs(dest, t_str.c_str(), l);
        return dest;
#else
        typedef std::codecvt_utf8<wchar_t> ConvertType;
        std::wstring_convert<ConvertType, wchar_t> converter;

        return converter.from_bytes(t_str);
#endif
    }

    static std::string wstringToString(const std::wstring& t_str)
    {
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
        //setup converter
        typedef std::codecvt_utf8<wchar_t> ConvertType;
        std::wstring_convert<ConvertType, wchar_t> converter;

#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

        //use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
        return converter.to_bytes(t_str);
    }
};

// OPENDAQ_TODO: std::string conversion
template <>
struct CoreTypeHelper<std::string>
{
    typedef IString Interface;
    typedef std::string TrueType;

    /*  static ErrCode FromConvertible(std::string& val, IConvertible* convObj)
        {
            SizeT maxLen;

            ErrCode err = convObj->toString(nullptr, &maxLen);
            if (OPENDAQ_FAILED(err) && err != OPENDAQ_ERR_SIZETOOSMALL)
                return err;

            CharPtr s = new (std::nothrow) char[maxLen + 1];
            if (s == nullptr)
                return OPENDAQ_ERR_NOMEMORY;

            err = convObj->toString(s, &maxLen);
            if (OPENDAQ_FAILED(err))
            {
                delete[] s;
                return err;
            }

            val = s;
            delete[] s;

            return OPENDAQ_SUCCESS;
        }*/

    static void Print(std::ostringstream& os, const std::string& val)
    {
        os << val;
    }

    static CoreType GetCoreType()
    {
        return ctString;
    }

    static IString* Create(const std::string& value)
    {
        return String_Create(value.c_str());
    }
};

template <typename Ty, typename Enable = void>
struct Is_ct_conv
    : std::false_type
{
};

template <>
struct Is_ct_conv<bool>
    : std::true_type
{
};

template <>
struct Is_ct_conv<Bool>
    : std::true_type
{
};

template <typename T>
struct Is_ct_conv<T, typename std::enable_if_t<std::is_floating_point_v<T>>>
    : std::true_type
{
};

template <typename T>
struct Is_ct_conv<T, typename std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>>>
    : std::true_type
{
};

template <>
struct Is_ct_conv<std::wstring>
    : std::true_type
{
};

template <>
struct Is_ct_conv<std::string>
    : std::true_type
{
};

template <class Ty>
struct is_ct_conv
    : Is_ct_conv<std::remove_cv_t<Ty>>
{
};

template <>
struct Is_ct_conv<ComplexFloat32> : std::true_type
{
};

template <>
struct Is_ct_conv<ComplexFloat64> : std::true_type
{
};

// ------ IntfToCoreType -------

template <typename T>
struct IntfToCoreType
{
    typedef void CoreType;
};

template <>
struct IntfToCoreType<IInteger>
{
    typedef Int CoreType;
};

template <>
struct IntfToCoreType<IFloat>
{
    typedef Float CoreType;
};

template <>
struct IntfToCoreType<IBoolean>
{
    typedef Bool CoreType;
};

template <>
struct IntfToCoreType<IString>
{
    typedef std::string CoreType;
};

template <>
struct IntfToCoreType<IComplexNumber>
{
    typedef ComplexFloat64 CoreType;
};

// ------ supports_conv_from_coretype ------

template <typename Intf>
struct supports_conv_from_coretype
    : std::false_type
{
};

template <>
struct supports_conv_from_coretype<INumber>
    : std::true_type
{
};

// ------ BinOperation -------

enum class BinOperationType
{
    add = 0,
    sub,
    mul,
    div,
    logOr,
    logAnd,
    equals,
    notEquals,
    greater,
    greaterOrEqual,
    less,
    lessOrEqual
};

template <typename T, BinOperationType O>
struct BinOperation
{
};

template <typename T>
struct BinOperation<T, BinOperationType::add>
{
    static T Op(const T& val1, const T& val2)
    {
        return val1 + val2;
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::sub>
{
    static T Op(const T& val1, const T& val2)
    {
        return val1 - val2;
    }
};

template <>
struct BinOperation<std::wstring, BinOperationType::sub>
{
    static std::wstring Op(const std::wstring& /*val1*/, const std::wstring& /*val2*/)
    {
        throw std::logic_error("Undefined");
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::mul>
{
    static T Op(const T& val1, const T& val2)
    {
        return val1 * val2;
    }
};

template <>
struct BinOperation<std::wstring, BinOperationType::mul>
{
    static std::wstring Op(const std::wstring& /*val1*/, const std::wstring& /*val2*/)
    {
        throw std::logic_error("Undefined");
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::div>
{
    static T Op(const T& val1, const T& val2)
    {
        return val1 / val2;
    }
};

template <>
struct BinOperation<std::wstring, BinOperationType::div>
{
    static std::wstring Op(const std::wstring& /*val1*/, const std::wstring& /*val2*/)
    {
        throw std::logic_error("Undefined");
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::logOr>
{
    static bool Op(const T& val1, const T& val2)
    {
        return val1 || val2;
    }
};

template <>
struct BinOperation<std::wstring, BinOperationType::logOr>
{
    static bool Op(const std::wstring& /*val1*/, const std::wstring& /*val2*/)
    {
        throw std::logic_error("Undefined");
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::logAnd>
{
    static bool Op(const T& val1, const T& val2)
    {
        bool r = val1 && val2;
        return r;
    }
};

template <>
struct BinOperation<std::wstring, BinOperationType::logAnd>
{
    static bool Op(const std::wstring& /*val1*/, const std::wstring& /*val2*/)
    {
        throw std::logic_error("Undefined");
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::equals>
{
    static bool Op(const T& val1, const T& val2)
    {
        return val1 == val2;
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::notEquals>
{
    static bool Op(const T& val1, const T& val2)
    {
        return val1 == val2;
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::greater>
{
    static bool Op(const T& val1, const T& val2)
    {
        return val1 > val2;
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::greaterOrEqual>
{
    static bool Op(const T& val1, const T& val2)
    {
        return val1 >= val2;
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::less>
{
    static bool Op(const T& val1, const T& val2)
    {
        return val1 < val2;
    }
};

template <typename T>
struct BinOperation<T, BinOperationType::lessOrEqual>
{
    static bool Op(const T& val1, const T& val2)
    {
        return val1 <= val2;
    }
};

// ------ BinOperationToStdOp -------

template <BinOperationType O>
struct BinOperationToStdOp
{
};

template <>
struct BinOperationToStdOp<BinOperationType::add>
{
    typedef std::plus<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::sub>
{
    typedef std::minus<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::mul>
{
    typedef std::multiplies<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::div>
{
    typedef std::divides<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::logOr>
{
    typedef std::logical_or<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::logAnd>
{
    typedef std::logical_and<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::equals>
{
    typedef std::equal_to<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::notEquals>
{
    typedef std::not_equal_to<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::greater>
{
    typedef std::greater<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::greaterOrEqual>
{
    typedef std::greater_equal<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::less>
{
    typedef std::less<> op;
};

template <>
struct BinOperationToStdOp<BinOperationType::lessOrEqual>
{
    typedef std::less_equal<> op;
};


// ------ UnaryOperation -------

enum class UnaryOperationType
{
    Negate = 0,
    LogNegate = 1,
};

template <typename T, UnaryOperationType O>
struct UnaryOperation
{
};

template <typename T>
struct UnaryOperation<T, UnaryOperationType::Negate>
{
    static T Op(const T& val)
    {
        return -val;
    }
};

template <typename T>
struct UnaryOperation<T, UnaryOperationType::LogNegate>
{
    static T Op(const T& val)
    {
        return !val;
    }
};

// ------ UnaryOperationToStdOp -------

template <UnaryOperationType O>
struct UnaryOperationToStdOp
{
};

template <>
struct UnaryOperationToStdOp<UnaryOperationType::Negate>
{
    typedef std::negate<> op;
};

template <>
struct UnaryOperationToStdOp<UnaryOperationType::LogNegate>
{
    typedef std::logical_not<> op;
};

// ------

template <class C, class I>
struct IsCoreTypeAndIntfSame : std::false_type
{
};

/*
template <class C, class I>
struct IsCoreTypeAndIntfSame : std::true_type
{
};
*/

END_NAMESPACE_OPENDAQ
