/*
 * Copyright 2022-2024 Blueberry d.o.o.
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

#include <py_core_objects/py_core_objects.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using variant_full_t = std::variant<std::monostate,
                                    bool,
                                    int64_t,
                                    double,
                                    std::pair<int64_t, int64_t>,
                                    std::complex<double>,
                                    py::str,
                                    py::list,
                                    py::dict,
                                    py::object>;

// converts a daq type to a python type
template <typename DaqInterfaceType>
struct DaqInterfaceTypeToNativeType
{
    using type = py::object;
};

template <>
struct DaqInterfaceTypeToNativeType<daq::IBoolean>
{
    using type = bool;
};

template <>
struct DaqInterfaceTypeToNativeType<daq::IFloat>
{
    using type = double;
};

template <>
struct DaqInterfaceTypeToNativeType<daq::IInteger>
{
    using type = int64_t;
};

template <>
struct DaqInterfaceTypeToNativeType<daq::INumber>
{
    using type = double;
};

template <>
struct DaqInterfaceTypeToNativeType<daq::IRatio>
{
    using type = std::pair<int64_t, int64_t>;
};

template <>
struct DaqInterfaceTypeToNativeType<daq::IComplexNumber>
{
    using type = std::complex<double>;
};

template <>
struct DaqInterfaceTypeToNativeType<daq::IList>
{
    using type = py::list;
};

template <>
struct DaqInterfaceTypeToNativeType<daq::IDict>
{
    using type = py::dict;
};

template <>
struct DaqInterfaceTypeToNativeType<daq::IString>
{
    using type = py::str;
};

// check Variant has type
template <typename... Ts>
struct contains : std::false_type
{
};
template <typename T, typename Head, typename... Tail>
struct contains<T, Head, Tail...> : std::disjunction<std::is_same<T, Head>, contains<T, Tail...>>
{
};

template <typename T, typename... Ts>
constexpr bool variant_contains = contains<T, Ts...>::value;

template <typename VariantType>
daq::BaseObjectPtr getVariantValueInternal(VariantType& v)
{
    if (auto ptr = std::get_if<std::pair<int64_t, int64_t>>(&v))
    {
        return daq::Ratio(ptr->first, ptr->second);
    }
    else if (auto complex = std::get_if<std::complex<double>>(&v))
    {
        return daq::ComplexNumber(complex->real(), complex->imag());
    }
    else if (auto boolp = std::get_if<bool>(&v))
    {
        return *boolp;
    }
    else if (auto floatp = std::get_if<double>(&v))
    {
        return *floatp;
    }
    else if (auto intp = std::get_if<int64_t>(&v))
    {
        return *intp;
    }
    else if (auto dict = std::get_if<py::dict>(&v))
    {
        auto d = daq::Dict<daq::IBaseObject, daq::IBaseObject>();
        for (auto& [k, v] : *dict)
        {
            auto key = k.template cast<variant_full_t>();
            auto value = v.template cast<variant_full_t>();
            d.set(getVariantValueInternal(key), getVariantValueInternal(value));
        }
        return d;
    }
    else if (auto list = std::get_if<py::list>(&v))
    {
        auto l = daq::List<daq::IBaseObject>();
        for (auto& item : *list)
        {
            auto value = item.template cast<variant_full_t>();
            l.pushBack(getVariantValueInternal(value));
        }
        return l;
    }
    else if (auto str = std::get_if<py::str>(&v))
    {
        return std::string(*str);
    }
    else if (auto obj = std::get_if<py::object>(&v))
    {
        return pyObjectToBaseObject(*obj);
    }
    return nullptr;
}

template <typename DaqType,
          typename NativeType = typename DaqInterfaceTypeToNativeType<std::remove_pointer_t<DaqType>>::type,
          typename Variant>
daq::ObjectPtr<std::remove_pointer_t<DaqType>> getVariantValue(Variant& v)
{
    if constexpr (variant_contains<daq::IEvalValue*, Variant>)
    {
        if (auto ptr = std::get_if<daq::IEvalValue*>(&v))
        {
            return daq::ObjectPtr<std::remove_pointer_t<DaqType>>(*ptr);
        }
    }
    if (auto daq = std::get_if<DaqType>(&v))
    {
        return daq::ObjectPtr<std::remove_pointer_t<DaqType>>::Borrow(*daq);
    }
    else if (auto native = std::get_if<NativeType>(&v))
    {
        variant_full_t& variant = *native;
        return getVariantValueInternal(variant);
    }
    return nullptr;
}