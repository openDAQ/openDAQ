#include <coreobjects/property_impl.h>

BEGIN_NAMESPACE_OPENDAQ

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createProperty,
    IString*, name
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createBoolProperty,
    IString*, name,
    IBoolean*, defaultValue,
    IBoolean*, visible
)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createIntProperty,
    IString*, name,
    IInteger*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createFloatProperty,
    IString*, name,
    IFloat*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createStringProperty,
    IString*, name,
    IString*, defaultValue,
    IBoolean*, visible
)
OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createListProperty,
    IString*, name,
    IList*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createDictProperty,
    IString*, name,
    IDict*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createRatioProperty,
    IString*, name,
    IRatio*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createObjectProperty,
    IString*, name,
    IPropertyObject*, defaultValue
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createFunctionProperty,
    IString*, name,
    ICallableInfo*, callableInfo,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createReferenceProperty,
    IString*, name,
    IEvalValue*, referencedPropertyEval
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createSelectionProperty,
    IString*, name,
    IList*, selectionValues,
    IInteger*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createSparseSelectionProperty,
    IString*, name,
    IDict*, selectionValues,
    IInteger*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createStructProperty,
    IString*, name,
    IStruct*, defaultValue,
    IBoolean*, visible
)

OPENDAQ_DEFINE_CLASS_FACTORY_WITH_INTERFACE_AND_CREATEFUNC(
    LIBRARY_FACTORY, Property,
    IProperty, createPropertyFromBuildParams,
    IDict*, buildParams
)

END_NAMESPACE_OPENDAQ
