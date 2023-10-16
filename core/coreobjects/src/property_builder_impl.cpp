#include <coreobjects/property_builder_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createPropertyBuilder,
    IString*, name
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createBoolPropertyBuilder,
    IString*, name,
    IBoolean*, defaultValue
)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createIntPropertyBuilder,
    IString*, name,
    IInteger*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createFloatPropertyBuilder,
    IString*, name,
    IFloat*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createStringPropertyBuilder,
    IString*, name,
    IString*, defaultValue
)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createListPropertyBuilder,
    IString*, name,
    IList*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createDictPropertyBuilder,
    IString*, name,
    IDict*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createRatioPropertyBuilder,
    IString*, name,
    IRatio*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createObjectPropertyBuilder,
    IString*, name,
    IPropertyObject*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createFunctionPropertyBuilder,
    IString*, name,
    ICallableInfo*, callableInfo
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createReferencePropertyBuilder,
    IString*, name,
    IEvalValue*, referencedPropertyEval
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createSelectionPropertyBuilder,
    IString*, name,
    IList*, selectionValues,
    IInteger*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createSparseSelectionPropertyBuilder,
    IString*, name,
    IDict*, selectionValues,
    IInteger*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, PropertyBuilder,
    IPropertyBuilder, createStructPropertyBuilder,
    IString*, name,
    IStruct*, defaultValue
)

END_NAMESPACE_OPENDAQ
