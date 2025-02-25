#include <coretypes/objectptr.h>
#include <coretypes/weakrefptr.h>

#include <coretypes/string_ptr.h>
#include <coretypes/enumeration_ptr.h>
#include <coretypes/complex_number_ptr.h>
#include <coretypes/deserializer_ptr.h>
#include <coretypes/event_handler_ptr.h>
#include <coretypes/event_ptr.h>
#include <coretypes/function_ptr.h>
#include <coretypes/inspectable_ptr.h>
#include <coretypes/iterable_ptr.h>
#include <coretypes/number_ptr.h>
#include <coretypes/procedure_ptr.h>
#include <coretypes/ratio_ptr.h>
#include <coretypes/serializable_ptr.h>
#include <coretypes/serialized_list_ptr.h>
#include <coretypes/serialized_object_ptr.h>
#include <coretypes/serializer_ptr.h>
#include <coretypes/struct_builder_ptr.h>
#include <coretypes/struct_ptr.h>
#include <coretypes/type_manager_ptr.h>
#include <coretypes/updatable_ptr.h>
#include <coretypes/version_info_ptr.>

#include <coretypes/struct_type_ptr.h>
#include <coretypes/enumeration_type_ptr.h>

#include <coretypes/>

BEGIN_NAMESPACE_OPENDAQ

template class ObjectPtr<IBaseObject>;
template class ObjectPtr<IInteger>;
template class ObjectPtr<IBoolean>;
template class ObjectPtr<IFloat>;
template class ObjectPtr<IString>;
template class ObjectPtr<IEnumeration>;
template class ObjectPtr<IComplexNumber>;
template class ObjectPtr<IDeserializer>;
template class ObjectPtr<IEventHandler>;
template class ObjectPtr<IEvent>;
template class ObjectPtr<IFunction>;
template class ObjectPtr<IInspectable>;
template class ObjectPtr<IIterable>;
template class ObjectPtr<INumber>;
template class ObjectPtr<IProcedure>;
template class ObjectPtr<IRatio>;
template class ObjectPtr<ISerializable>;
template class ObjectPtr<ISerializedList>;
template class ObjectPtr<ISerializedObject>;
template class ObjectPtr<ISerializer>;
template class ObjectPtr<IStructBuilder>;
template class ObjectPtr<IStruct>;
template class ObjectPtr<IType>;
template class ObjectPtr<ITypeManager>;
template class ObjectPtr<IUpdatable>;
template class ObjectPtr<IVersionInfo>;

template class GenericTypePtr<IType>;
template class GenericTypePtr<IEnumerationType>;
template class GenericTypePtr<IStructType>;

template class GenericStructPtr<IStruct>;

END_NAMESPACE_OPENDAQ
