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
#include <coreobjects/coreobjects_config.h>

#include <coreobjects/version.h>

#include <coreobjects/exceptions.h>
#include <coreobjects/property.h>
#include <coreobjects/property_factory.h>
#include <coreobjects/property_object_class.h>
#include <coreobjects/property_object_protected_ptr.h>
#include <coreobjects/property_object.h>
#include <coreobjects/ownable.h>
#include <coreobjects/eval_value.h>

#include <coreobjects/property_object_impl.h>

#include <coreobjects/property_ptr.h>
#include <coreobjects/property_object_ptr.h>
#include <coreobjects/property_object_factory.h>
#include <coreobjects/property_object_class_factory.h>
#include <coreobjects/property_object_class_ptr.h>
#include <coreobjects/ownable_ptr.h>
#include <coreobjects/eval_value_ptr.h>

#include <coreobjects/eval_value_factory.h>

#include <coreobjects/property_value_event_args_factory.h>
#include <coreobjects/end_update_event_args_factory.h>

#include <coreobjects/owning_list_factory.h>
#include <coreobjects/owning_dict_factory.h>

#include <coreobjects/coercer_factory.h>
#include <coreobjects/validator_factory.h>

#include <coreobjects/unit_factory.h>

#include <coreobjects/component_type_ptr.h>
#include <coreobjects/user_factory.h>
#include <coreobjects/authentication_provider_factory.h>

/*!
 * @ingroup coreobjects
 * @defgroup objects_property Property
 * @brief Property related concepts. Used to add metadata information to property objects.
 */

/*!
 * @ingroup coreobjects
 * @defgroup objects_property_object Property Object
 * @brief Property object concepts. Used to hold key-value pairs of strings and IBaseObject descendants.
 */

/*!
 * @ingroup coreobjects
 * @defgroup objects_utility Utility
 * @brief Utility openDAQ objects concepts.
 */

/*!
 * @ingroup coreobjects
 * @defgroup objects_errors_group Errors
 */

/*!
 * @ingroup objects_errors_group
 * @defgroup  objects_error_codes Error Codes
 * @{
 * @page objects_errors Error Codes
 * @brief Core objects error codes
 * @}
 */
