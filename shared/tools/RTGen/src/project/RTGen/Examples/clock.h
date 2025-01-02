/*
 * Copyright 2022-2025 openDAQ d.o.o.
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
#include <coretypes/ratio.h>
#include <corestructure/unit.h>
#include <corestructure/scaling.h>

BEGIN_NAMESPACE_DEWESOFT_RT_CORE

using IClockScaling = IScaling<Int, double>;

/*#
 * [include(ISampleTypes)]
 * [libraryInterfaces(CoreStructure, IUnit, IScaling)]
 */
DECLARE_RT_INTERFACE(IClock, IBaseObject)
{
    virtual ErrCode INTERFACE_FUNC getUnit(IUnit** unit) = 0;

    // clock resolution (the smallest possible difference between 2 ticks in a fraction of the Unit)
    virtual ErrCode INTERFACE_FUNC getMultiplier(IRatio** multiplier) = 0;
    virtual ErrCode INTERFACE_FUNC setMultiplier(IRatio* multiplier) = 0;

    // k = number of clock ticks between samples
    // n = start offset
    virtual ErrCode INTERFACE_FUNC getScaling(IClockScaling** scaling) = 0;
};

OPENDAQ_DECLARE_CLASS_FACTORY(LIBRARY_FACTORY, Clock,
    IRatio*, multiplier,
    IClockScaling*, scaling,
    IUnit*, unit
)

END_NAMESPACE_DEWESOFT_RT_CORE
