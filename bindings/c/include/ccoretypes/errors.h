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

/**
 * @ingroup types_errors_group
 * @defgroup types_errors_macros Error Code Macros
 * @{
 */

#define DAQ_ERROR_CODE(TYPE_ID, ERROR_CODE) (0x80000000u | ((TYPE_ID) << 16u) | (ERROR_CODE))
#define DAQ_GET_ERROR_CODE(ERROR) (0x0000FFFFu & ERROR)

#define DAQ_FAILED(x)    ((x) & 0x80000000u)
#define DAQ_SUCCEEDED(x) (!DAQ_FAILED(x))

#define DAQ_ERRTYPE_GENERIC        0x00u

#define DAQ_SUCCESS                0x00000000u

#define DAQ_LOWER                  0x00000002u
#define DAQ_EQUAL                  0x00000003u
#define DAQ_GREATER                0x00000004u
#define DAQ_NO_MORE_ITEMS          0x00000005u
#define DAQ_IGNORED                0x00000006u
#define DAQ_NOTFOUND               0x00000007u

#define DAQ_MAKE_ERROR(err) OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, err)

#define DAQ_ERR_NOMEMORY                       DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0000u)
#define DAQ_ERR_INVALIDPARAMETER               DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0001u)
#define DAQ_ERR_NOINTERFACE                    DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x4002u) // to make it COM-compatible
#define DAQ_ERR_SIZETOOSMALL                   DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0003u)
#define DAQ_ERR_CONVERSIONFAILED               DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0004u)
/*!
 * @types_error{
 * 0x80000005,
 * The requested item index exceeds the target structure size.
 * }
 */
#define DAQ_ERR_OUTOFRANGE                     DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0005u)
/*!
 * @types_error{
 * 0x80000006,
 * Requested object is not a part of the target structure.
 * }
 */
#define DAQ_ERR_NOTFOUND                       DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0006u)

/*!
 * @types_error{
 * 0x8000000A,
 * The object being added is already present in the target structure.
 * }
 */
#define DAQ_ERR_ALREADYEXISTS                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x000Au)
#define DAQ_ERR_NOTASSIGNED                    DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x000Bu)
#define DAQ_ERR_CALLFAILED                     DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x000Cu)
#define DAQ_ERR_PARSEFAILED                    DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x000Du)
#define DAQ_ERR_INVALIDVALUE                   DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x000Eu)
#define DAQ_ERR_RESOLVEFAILED                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0010u)
#define DAQ_ERR_INVALIDTYPE                    DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0011u)
#define DAQ_ERR_ACCESSDENIED                   DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0012u)
#define DAQ_ERR_NOTENABLED                     DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0013u)
#define DAQ_ERR_GENERALERROR                   DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0014u)
#define DAQ_ERR_CALCFAILED                     DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0015u)
#define DAQ_ERR_NOTIMPLEMENTED                 DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0016u)
#define DAQ_ERR_FROZEN                         DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0017u)
#define DAQ_ERR_NOT_SERIALIZABLE               DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0018u)
#define DAQ_ERR_FACTORY_NOT_REGISTERED         DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0020u)
#define DAQ_ERR_DESERIALIZE_PARSE_ERROR        DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0021u)
#define DAQ_ERR_DESERIALIZE_UNKNOWN_TYPE       DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0022u)
#define DAQ_ERR_DESERIALIZE_NO_TYPE            DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0023u)
#define DAQ_ERR_INVALIDPROPERTY                DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0024u)
#define DAQ_ERR_DUPLICATEITEM                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0025u)
#define DAQ_ERR_ARGUMENT_NULL                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0026u)
#define DAQ_ERR_INVALID_OPERATION              DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0027u)
#define DAQ_ERR_UNINITIALIZED                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0028u)
#define DAQ_ERR_INVALIDSTATE                   DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0029u)

/*!
 * @types_error{
 * 0x80000030,
 * Validation of the passed value failed. The value does not adhere to the imposed type
 * or content predicates.
 * }
 */
#define DAQ_ERR_VALIDATE_FAILED                DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0030u)
#define DAQ_ERR_NOT_UPDATABLE                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0031u)
#define DAQ_ERR_NO_COMPATIBLE_VERSION          DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0032u)
#define DAQ_ERR_LOCKED                         DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0033u)
#define DAQ_ERR_SIZETOOLARGE                   DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0034u)
#define DAQ_ERR_BUFFERFULL                     DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0035u)
#define DAQ_ERR_CREATE_FAILED                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0036u)
#define DAQ_ERR_EMPTY_SCALING_TABLE            DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0037u)
#define DAQ_ERR_EMPTY_RANGE                    DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0038u)
#define DAQ_ERR_DISCOVERY_FAILED               DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0039u)
/*!
 * @types_error{
 * 0x80000040,
 * Coercion of the passed value failed. The value does not adhere to the imposed type
 * or content predicates. It cannot be coerced to fit to the provided value restrictions. 
 * }
 */
#define DAQ_ERR_COERCE_FAILED                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0040u)
#define DAQ_ERR_NOT_SUPPORTED                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0041u)
#define DAQ_ERR_LIST_NOT_HOMOGENEOUS           DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0042u)
#define DAQ_ERR_NOT_FROZEN                     DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0043u)
#define DAQ_ERR_NO_TYPE_MANAGER                DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0044u)

#define DAQ_ERR_NO_DATA                        DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0050u)
#define DAQ_ERR_INVALID_ARGUMENT               DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0051u)
#define DAQ_ERR_DEVICE_LOCKED                  DAQ_ERROR_CODE(DAQ_ERRTYPE_GENERIC, 0x0052u)

/*!
 * @}
 */
