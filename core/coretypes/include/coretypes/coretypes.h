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
#include <coretypes/coretypes_config.h>

#include <coretypes/errors.h>
#include <coretypes/version.h>
#include <coretypes/intfs.h>
#include <coretypes/delegate.hpp>

#include <coretypes/formatter.h>

#include <coretypes/weakref_impl.h>
#include <coretypes/weakrefobj.h>
#include <coretypes/weakrefptr.h>

#include <coretypes/inspectable_ptr.h>

#include <coretypes/baseobject_factory.h>
#include <coretypes/boolean_factory.h>
#include <coretypes/integer_factory.h>
#include <coretypes/float_factory.h>
#include <coretypes/stringobject_factory.h>
#include <coretypes/function_factory.h>
#include <coretypes/procedure_factory.h>
#include <coretypes/binarydata_factory.h>
#include <coretypes/ratio_factory.h>
#include <coretypes/complex_number_factory.h>

#include <coretypes/number_ptr.h>

#include <coretypes/iterable_ptr.h>

#include <coretypes/event_args_ptr.h>
#include <coretypes/event_args_factory.h>

#include <coretypes/event_emitter.h>
#include <coretypes/event_factory.h>
#include <coretypes/event_handler.h>
#include <coretypes/event_handler_ptr.h>

#include <coretypes/errorinfo_factory.h>

#include <coretypes/serialized_object.h>
#include <coretypes/serialized_list.h>
#include <coretypes/deserializer.h>
#include <coretypes/serialized_list_ptr.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/json_serializer_factory.h>
#include <coretypes/json_deserializer_factory.h>

#include <coretypes/objectptr.h>
#include <coretypes/listobject_factory.h>
#include <coretypes/dictobject_factory.h>
#include <coretypes/list_element_type.h>
#include <coretypes/dict_element_type.h>

#include <coretypes/impl.h>
#include <coretypes/cycle_detector.h>

#include <coretypes/struct_factory.h>
#include <coretypes/struct_type_factory.h>
#include <coretypes/simple_type_factory.h>
#include <coretypes/type_manager_factory.h>

#include <coretypes/enumeration_factory.h>
#include <coretypes/enumeration_type_factory.h>

/*!
 * @defgroup coretypes Core Types
 * @brief Core types
 */

/*!
 * @defgroup coreobjects Core Objects
 * @brief Core objects
 */

/*!
 * @defgroup opendaq openDAQ
 */

/*!
 * @ingroup coretypes
 * @defgroup types_base_concepts Base Concepts
 * @brief Core types base object classes
 */

/*!
 * @ingroup coretypes
 * @defgroup types_utility Utility
 * @brief Core types utilities
 */

/*!
 * @ingroup coretypes
 * @defgroup types_numerics Numerics
 * @brief Core numeric types
 */

/*!
 * @ingroup coretypes
 * @defgroup types_containers Containers
 * @brief Core container types
 */

/*!
 * @ingroup coretypes
 * @defgroup types_serialization Serialization
 * @brief Core serialization support
 */

/*!
 * @ingroup coretypes
 * @defgroup types_events Events
 * @brief Core types event support
 */

/*!
 * @ingroup coretypes
 * @defgroup types_functions Function Types
 * @brief Core types function and procedure support
 */

/*!
 * @ingroup coretypes
 * @defgroup types_types Type management
 * @brief Core types Type management utilities
 */

/*!
 * @ingroup coretypes
 * @defgroup types_structs Structs
 * @brief Core types Struct-type objects and utilities
 */

/*!
 * @ingroup coretypes
 * @defgroup types_enumerations Enumerations
 * @brief Core types Enumeration-type objects and utilities
 */

/*!
 * @ingroup coretypes
 * @defgroup types_errors_group Errors
 */

/*!
 * @ingroup types_errors_group
 * @defgroup  types_error_codes Error Codes
 * @{
 * @page types_errors Error Codes
 * @brief Core types error codes
 * @}
 */
