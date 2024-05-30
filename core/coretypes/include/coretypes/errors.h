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

/**
 * @ingroup types_errors_group
 * @defgroup types_errors_macros Error Code Macros
 * @{
 */

#define OPENDAQ_ERROR_CODE(TYPE_ID, ERROR_CODE) (0x80000000u | ((TYPE_ID) << 16u) | (ERROR_CODE))
#define OPENDAQ_GET_ERROR_CODE(ERROR) (0x0000FFFFu & ERROR)

#define OPENDAQ_FAILED(x)    ((x) & 0x80000000u)
#define OPENDAQ_SUCCEEDED(x) (!OPENDAQ_FAILED(x))

#define OPENDAQ_ERRTYPE_GENERIC        0x00u

#define OPENDAQ_SUCCESS                0x00000000u

#define OPENDAQ_LOWER                  0x00000002u
#define OPENDAQ_EQUAL                  0x00000003u
#define OPENDAQ_GREATER                0x00000004u
#define OPENDAQ_NO_MORE_ITEMS          0x00000005u
#define OPENDAQ_IGNORED                0x00000006u
#define OPENDAQ_NOTFOUND               0x00000007u

#define OPENDAQ_MAKE_ERROR(err) OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, err)

#define OPENDAQ_ERR_NOMEMORY                       OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0000u)
#define OPENDAQ_ERR_INVALIDPARAMETER               OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0001u)
#define OPENDAQ_ERR_NOINTERFACE                    OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x4002u) // to make it COM-compatible
#define OPENDAQ_ERR_SIZETOOSMALL                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0003u)
#define OPENDAQ_ERR_CONVERSIONFAILED               OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0004u)
/*!
 * @types_error{
 * 0x80000005,
 * The requested item index exceeds the target structure size.
 * }
 */
#define OPENDAQ_ERR_OUTOFRANGE                     OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0005u)
/*!
 * @types_error{
 * 0x80000006,
 * Requested object is not a part of the target structure.
 * }
 */
#define OPENDAQ_ERR_NOTFOUND                       OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0006u)

/*!
 * @types_error{
 * 0x8000000A,
 * The object being added is already present in the target structure.
 * }
 */
#define OPENDAQ_ERR_ALREADYEXISTS                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x000Au)
#define OPENDAQ_ERR_NOTASSIGNED                    OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x000Bu)
#define OPENDAQ_ERR_CALLFAILED                     OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x000Cu)
#define OPENDAQ_ERR_PARSEFAILED                    OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x000Du)
#define OPENDAQ_ERR_INVALIDVALUE                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x000Eu)
#define OPENDAQ_ERR_RESOLVEFAILED                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0010u)
#define OPENDAQ_ERR_INVALIDTYPE                    OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0011u)
#define OPENDAQ_ERR_ACCESSDENIED                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0012u)
#define OPENDAQ_ERR_NOTENABLED                     OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0013u)
#define OPENDAQ_ERR_GENERALERROR                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0014u)
#define OPENDAQ_ERR_CALCFAILED                     OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0015u)
#define OPENDAQ_ERR_NOTIMPLEMENTED                 OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0016u)
#define OPENDAQ_ERR_FROZEN                         OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0017u)
#define OPENDAQ_ERR_NOT_SERIALIZABLE               OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0018u)
#define OPENDAQ_ERR_FACTORY_NOT_REGISTERED         OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0020u)
#define OPENDAQ_ERR_DESERIALIZE_PARSE_ERROR        OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0021u)
#define OPENDAQ_ERR_DESERIALIZE_UNKNOWN_TYPE       OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0022u)
#define OPENDAQ_ERR_DESERIALIZE_NO_TYPE            OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0023u)
#define OPENDAQ_ERR_INVALIDPROPERTY                OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0024u)
#define OPENDAQ_ERR_DUPLICATEITEM                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0025u)
#define OPENDAQ_ERR_ARGUMENT_NULL                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0026u)
#define OPENDAQ_ERR_INVALID_OPERATION              OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0027u)
#define OPENDAQ_ERR_UNINITIALIZED                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0028u)
#define OPENDAQ_ERR_INVALIDSTATE                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0029u)
#define OPENDAQ_ERR_COMPONENT_REMOVED              OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x002Au)

/*!
 * @types_error{
 * 0x80000030,
 * Validation of the passed value failed. The value does not adhere to the imposed type
 * or content predicates.
 * }
 */
#define OPENDAQ_ERR_VALIDATE_FAILED                OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0030u)
#define OPENDAQ_ERR_NOT_UPDATABLE                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0031u)
#define OPENDAQ_ERR_NO_COMPATIBLE_VERSION          OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0032u)
#define OPENDAQ_ERR_LOCKED                         OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0033u)
#define OPENDAQ_ERR_SIZETOOLARGE                   OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0034u)
#define OPENDAQ_ERR_BUFFERFULL                     OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0035u)
#define OPENDAQ_ERR_CREATE_FAILED                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0036u)
#define OPENDAQ_ERR_EMPTY_SCALING_TABLE            OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0037u)
#define OPENDAQ_ERR_EMPTY_RANGE                    OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0038u)
#define OPENDAQ_ERR_DISCOVERY_FAILED               OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0039u)
/*!
 * @types_error{
 * 0x80000040,
 * Coercion of the passed value failed. The value does not adhere to the imposed type
 * or content predicates. It cannot be coerced to fit to the provided value restrictions. 
 * }
 */
#define OPENDAQ_ERR_COERCE_FAILED                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0040u)
#define OPENDAQ_ERR_NOT_SUPPORTED                  OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0041u)

#define OPENDAQ_ERR_LIST_NOT_HOMOGENEOUS           OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0042u)
#define OPENDAQ_ERR_NOT_FROZEN                     OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0043u)
#define OPENDAQ_ERR_NO_TYPE_MANAGER                OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0044u)

#define OPENDAQ_ERR_NO_DATA                        OPENDAQ_ERROR_CODE(OPENDAQ_ERRTYPE_GENERIC, 0x0050u)

/*!
 * @}
 */
