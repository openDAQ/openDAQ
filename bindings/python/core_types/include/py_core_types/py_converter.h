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

// <string> is needed by many other files that include this header
#include <pybind11/numpy.h>
#include "py_opendaq_daq.h"

// helper function to avoid making a copy when returning a py::array_t
// https://pybind11-numpy-example.readthedocs.io/en/latest/index.html
// source: https://github.com/pybind/pybind11/issues/1042#issuecomment-642215028
template <typename Sequence>
inline py::array toPyArray(Sequence&& seq,
                                                            const py::array::ShapeContainer& shape = {},
                                                            const py::array::StridesContainer& strides = {},
                                                            const py::dtype& dtype = {})
{
    const auto size = seq.size();
    const auto data = seq.data();
    std::unique_ptr<Sequence> seq_ptr = std::make_unique<Sequence>(std::move(seq));
    auto capsule = py::capsule(seq_ptr.get(), [](void* p) { std::unique_ptr<Sequence>(reinterpret_cast<Sequence*>(p)); });
    seq_ptr.release();

    py::dtype dt = py::dtype::of<typename Sequence::value_type>();

    if (dtype)
    {
        // it looks like datetime64 is ignored by pybind11 now, so we need to use WA
        // this is for future use
        dt = dtype;
    }

    py::array::ShapeContainer arrayShape = {size};
    if (!shape->empty())
        arrayShape = std::move(shape);

    if (strides->empty())
        return py::array(dt, arrayShape, data, capsule);

    return py::array(dt, arrayShape, strides, data, capsule);
}
