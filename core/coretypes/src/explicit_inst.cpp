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
#include <coretypes/version_info_ptr.h>

#include <coretypes/struct_type_ptr.h>
#include <coretypes/enumeration_type_ptr.h>

#include <coretypes/common.h>

BEGIN_NAMESPACE_OPENDAQ

template class PUBLIC_EXPORT ObjectPtr<IBaseObject>;
template class PUBLIC_EXPORT ObjectPtr<IInteger>;
template class PUBLIC_EXPORT ObjectPtr<IBoolean>;
template class PUBLIC_EXPORT ObjectPtr<IFloat>;
template class PUBLIC_EXPORT ObjectPtr<IString>;
template class PUBLIC_EXPORT ObjectPtr<IEnumeration>;
template class PUBLIC_EXPORT ObjectPtr<IComplexNumber>;
template class PUBLIC_EXPORT ObjectPtr<IDeserializer>;
template class PUBLIC_EXPORT ObjectPtr<IEventHandler>;
template class PUBLIC_EXPORT ObjectPtr<IEvent>;
template class PUBLIC_EXPORT ObjectPtr<IFunction>;
template class PUBLIC_EXPORT ObjectPtr<IInspectable>;
template class PUBLIC_EXPORT ObjectPtr<IIterable>;
template class PUBLIC_EXPORT ObjectPtr<INumber>;
template class PUBLIC_EXPORT ObjectPtr<IProcedure>;
template class PUBLIC_EXPORT ObjectPtr<IRatio>;
template class PUBLIC_EXPORT ObjectPtr<ISerializable>;
template class PUBLIC_EXPORT ObjectPtr<ISerializedList>;
template class PUBLIC_EXPORT ObjectPtr<ISerializedObject>;
template class PUBLIC_EXPORT ObjectPtr<ISerializer>;
template class PUBLIC_EXPORT ObjectPtr<IStructBuilder>;
template class PUBLIC_EXPORT ObjectPtr<IStruct>;
template class PUBLIC_EXPORT ObjectPtr<IType>;
template class PUBLIC_EXPORT ObjectPtr<ITypeManager>;
template class PUBLIC_EXPORT ObjectPtr<IUpdatable>;
template class PUBLIC_EXPORT ObjectPtr<IVersionInfo>;
template class PUBLIC_EXPORT ObjectPtr<IWeakRef>;

template class PUBLIC_EXPORT GenericTypePtr<IType>;
template class PUBLIC_EXPORT GenericTypePtr<IEnumerationType>;
template class PUBLIC_EXPORT GenericTypePtr<IStructType>;

template class PUBLIC_EXPORT GenericStructPtr<IStruct>;

END_NAMESPACE_OPENDAQ
