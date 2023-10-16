/*
 * Copyright 2022-2023 Blueberry d.o.o.
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

/*
 * This header contains macros for parameter validation for functions which return ErrorCode. The
 * `OPENDAQ_ENABLE_PARAMETER_VALIDATION` CMake option can be set to enable or disable validation.
 * Validation is normally enabled, but can be disabled for performance (in which case the caller
 * must guarantee valid parameter values to avoid undefined behavior).
 */

#ifdef OPENDAQ_ENABLE_PARAMETER_VALIDATION

#define OPENDAQ_PARAM_REQUIRE(cond) \
    do { if (!(cond)) return OPENDAQ_ERR_INVALIDPARAMETER; } while (0)

#define OPENDAQ_PARAM_NOT_NULL(param) \
    do { if (nullptr == (param)) return OPENDAQ_ERR_ARGUMENT_NULL; } while (0)

#define OPENDAQ_PARAM_TRUTHY(param) \
    do { if (!(param)) return OPENDAQ_ERR_INVALIDPARAMETER; } while (0)

#define OPENDAQ_PARAM_FALSY(param) \
    do { if (param) return OPENDAQ_ERR_INVALIDPARAMETER; } while (0)

#define OPENDAQ_PARAM_GE(param, val) \
    do { if (!((param) >= (val))) return OPENDAQ_ERR_INVALIDPARAMETER; } while (0)

#define OPENDAQ_PARAM_GT(param, val) \
    do { if (!((param) > (val))) return OPENDAQ_ERR_INVALIDPARAMETER; } while (0)

#define OPENDAQ_PARAM_LE(param, val) \
    do { if (!((param) <= (val))) return OPENDAQ_ERR_INVALIDPARAMETER; } while (0)

#define OPENDAQ_PARAM_LT(param, val) \
    do { if (!((param) < (val))) return OPENDAQ_ERR_INVALIDPARAMETER; } while (0)

#define OPENDAQ_PARAM_BETWEEN(param, a, b) \
    do { if (!((param) >= (a)) || !((param) <= (b))) return OPENDAQ_ERR_INVALIDPARAMETER; } while (0)

#define OPENDAQ_PARAM_STRICTLY_BETWEEN(param, a, b) \
    do { if (!((param) > (a)) || !((param) < (b))) return OPENDAQ_ERR_INVALIDPARAMETER; } while (0)

#else

#define OPENDAQ_PARAM_REQUIRE(cond)
#define OPENDAQ_PARAM_NOT_NULL(param)
#define OPENDAQ_PARAM_TRUTHY(param)
#define OPENDAQ_PARAM_FALSY(param)
#define OPENDAQ_PARAM_GE(param, val)
#define OPENDAQ_PARAM_GT(param, val)
#define OPENDAQ_PARAM_LE(param, val)
#define OPENDAQ_PARAM_LT(param, val)
#define OPENDAQ_PARAM_BETWEEN(param, a, b)
#define OPENDAQ_PARAM_STRICTLY_BETWEEN(param, a, b)

#endif
