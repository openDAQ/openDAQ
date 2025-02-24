#include <coretypes/objectptr.h>
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

#include <coretypes/enumeration_type_ptr.h>

BEGIN_NAMESPACE_OPENDAQ

template class ObjectPtr<IBaseObject>;
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

template class GenericTypePtr<IEnumerationType>;

END_NAMESPACE_OPENDAQ
