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

#ifdef __cplusplus
extern "C"
{
#endif

#include "ccommon.h"

#include "ccoretypes/base_object.h"
#include "ccoretypes/binarydata.h"
#include "ccoretypes/boolean.h"
#include "ccoretypes/complex_number.h"
#include "ccoretypes/convertible.h"
#include "ccoretypes/dictobject.h"
#include "ccoretypes/enumeration.h"
#include "ccoretypes/enumeration_type.h"
#include "ccoretypes/event.h"
#include "ccoretypes/event_args.h"
#include "ccoretypes/event_handler.h"
#include "ccoretypes/float.h"
#include "ccoretypes/freezable.h"
#include "ccoretypes/function.h"
#include "ccoretypes/integer.h"
#include "ccoretypes/iterable.h"
#include "ccoretypes/iterator.h"
#include "ccoretypes/listobject.h"
#include "ccoretypes/number.h"
#include "ccoretypes/procedure.h"
#include "ccoretypes/ratio.h"
#include "ccoretypes/serializable.h"
#include "ccoretypes/serialized_list.h"
#include "ccoretypes/serialized_object.h"
#include "ccoretypes/serializer.h"
#include "ccoretypes/simple_type.h"
#include "ccoretypes/stringobject.h"
#include "ccoretypes/struct_builder.h"
#include "ccoretypes/struct.h"
#include "ccoretypes/struct_type.h"
#include "ccoretypes/type.h"
#include "ccoretypes/type_manager.h"
#include "ccoretypes/updatable.h"
#include "ccoretypes/version_info.h"

#include "ccoretypes/factories.h"

#include "ccoreobjects/argument_info.h"
#include "ccoreobjects/authentication_provider.h"
#include "ccoreobjects/callable_info.h"
#include "ccoreobjects/coercer.h"
#include "ccoreobjects/common.h"
#include "ccoreobjects/core_event_args.h"
#include "ccoreobjects/end_update_event_args.h"
#include "ccoreobjects/eval_value.h"
#include "ccoreobjects/ownable.h"
#include "ccoreobjects/permission_manager.h"
#include "ccoreobjects/permission_mask_builder.h"
#include "ccoreobjects/permissions.h"
#include "ccoreobjects/permissions_builder.h"
#include "ccoreobjects/property.h"
#include "ccoreobjects/property_builder.h"
#include "ccoreobjects/property_object.h"
#include "ccoreobjects/property_object_class.h"
#include "ccoreobjects/property_object_class_builder.h"
#include "ccoreobjects/property_object_protected.h"
#include "ccoreobjects/property_value_event_args.h"
#include "ccoreobjects/unit.h"
#include "ccoreobjects/unit_builder.h"
#include "ccoreobjects/user.h"
#include "ccoreobjects/validator.h"


#ifdef __cplusplus
}
#endif
