unit Test.OpenDAQ.PropertyObject;

interface

uses
  OpenDAQ.CoreTypes, DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.ObjectPtr,
  OpenDAQ.PropertyObjectClassConfig, OpenDAQ.PropertyObjectClassManager;

type
  [TestFixture]
  TTest_PropertyObject = class(TDSUnitTest)
  private
    FPropObjManager: IPropertyObjectClassManager;
    FTestPropClass: IPropertyObjectClassConfigPtr;
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure Create;
    [Test]
    procedure EmptyClassName;
    [Test]
    procedure ClassName;
    [Test]
    procedure SimpleProperty;
    [Test]
    procedure Ownership;
    [Test]
    procedure SerializeJsonSimple;
    [Test]
    procedure DeserializeJsonSimple;
    [Test]
    procedure GetNullProperty;
    [Test]
    procedure ClearNullProperty;
    [Test]
    procedure ClearPropertyValue;
    [Test]
    procedure ClearNonExistentProperty;
    [Test]
    procedure GetNonExistentPropertyObject;
    [Test]
    procedure GetPropertyObjectParamNull;
    [Test]
    procedure GetPropertyObjectWhenClassNull;
    [Test]
    procedure EnumVisibleWithVisibleThroughRefs;
    [Test]
    procedure SelectionPropertiesCustomOrder;
    [Test]
    procedure SelectionPropertiesInsertionOrder;
    [Test]
    procedure SetProtectedReadOnlyProperty;
    [Test]
    procedure SparseSelectionPropertyGet;
    [Test]
    procedure SparseSelectionPropertySet;
    [Test]
    procedure SparseSelectionPropertySetInvalid;
    [Test]
    procedure TrySetReadOnlyProperty;
    [Test]
    procedure EnumVisiblePropertyWhenClassNull;
    [Test]
    procedure ReadOnlyPropertyAssigned;
    [Test]
    procedure ValueNotFound;
    [Test]
    procedure ConvertToPropertyCoreType;
    [Test]
    procedure ConvertToPropertyCoreTypeFails;
    [Test]
    procedure ConvertToPropertyCoreTypeFails2;
    [Test]
    procedure SetNullPropertyValue;
    [Test]
    procedure SetNullProperty;
    [Test]
    procedure SelectionProp;
    [Test]
    procedure SelectionPropNoEnum;
    [Test]
    procedure SelectionPropNotRegistered;
    [Test]
    procedure SelectionPropNoList;
    [Test]
    procedure ChildPropSet;
    [Test]
    procedure ChildPropGet;
    [Test]
    procedure ChildPropSetViaRefProp;
    [Test]
    procedure ChildPropGetViaRefProp;
    [Test]
    procedure OnValueChange;
    [Test]
    procedure OnValueChangePropertyEvent;
    [Test]
    procedure LocalProperties;
    [Test]
    procedure EventSubscriptionCount;
    [Test]
    procedure EventSubscriptionMuteFreeFunction;
    [Test]
    procedure EventSubscriptionMute;
    [Test]
    procedure PropertyListIndexerOutOfRange;
    [Test]
    procedure PropertyIndexer;
    [Test]
    procedure ChildPropGetArray;
    [Test]
    procedure AutoCoerceMax;
    [Test]
    procedure AutoCoerceMaxEvalValue;
    [Test]
    procedure AutoCoerceMin;
    [Test]
    procedure AutoCoerceMinEvalValue;
    [Test]
    procedure ChildPropClear;
    [Test]
    procedure ChildPropClearViaRefProp;
    [Test]
    procedure ClearPropertyWhenFrozen;
    [Test]
    procedure CoercerDeserialize;
    [Test]
    procedure CoercerSerialize;
    [Test]
    procedure CoercerTestEvalValue;
    [Test]
    procedure DictProp;
    [Test]
    procedure HasPropertyFalse;
    [Test]
    procedure HasPropertyLocal;
    [Test]
    procedure HasPropertyOnClass;
    [Test]
    procedure HasPropertyOnClassParent;
    [Test]
    procedure HasPropertyPrivate;
    [Test]
    procedure ImplementationName;
    [Test]
    procedure InheritedParent;
    [Test]
    procedure Inspectable;
    [Test]
    procedure LocalSelectionProperty;
    [Test]
    procedure NestedChildPropClear;
    [Test]
    procedure NestedChildPropClearViaRefProp;
    [Test]
    procedure NestedChildPropGet;
    [Test]
    procedure NestedChildPropGetViaRefProp;
    [Test]
    procedure NestedChildPropSet;
    [Test]
    procedure NestedChildPropSetViaRefProp;
    [Test]
    procedure NotLocalProperty;
    [Test]
    procedure OnValueChangeClear;
    [Test]
    procedure PropertyCoerceEvalValue;
    [Test]
    procedure PropertyCoerceFailedEvalValue;
    [Test]
    procedure PropertyNotListIndexer;
    [Test]
    procedure PropertyValidateEvalValue;
    [Test]
    procedure PropertyValidateFailedEvalValue;
    [Test]
    procedure PropertyWriteValidateEvalValueMultipleTimes;
    [Test]
    procedure RegisterPropertyWhenFrozen;
    [Test]
    procedure SetOwnerWhenFrozen;
    [Test]
    procedure SetPropertyWhenFrozen;
    [Test]
    procedure SumFunctionProp;
    [Test]
    procedure TestToString;
    [Test]
    procedure ToStringWithoutPropertyClass;
    [Test]
    procedure ValidatorDeserialize;
    [Test]
    procedure ValidatorSerialize;
    [Test]
    procedure ValidatorTestEvalVale;
    [Test]
    procedure ClearProtectedReadOnlyProperty;
    [Test]
    procedure TryClearReadOnlyProperty;
  end;

implementation

uses
  SysUtils,
  WinApi.Windows,
  Generics.Collections,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.TProperty,
  OpenDAQ.EvalValue,
  OpenDAQ.PropertyConfig,
  OpenDAQ.PropertyObjectClass,
  OpenDAQ.List,
  OpenDAQ.Dict,
  OpenDAQ.TString,
  OpenDAQ.ProxyValue,
  OpenDAQ.Serializer,
  OpenDAQ.Exceptions,
  OpenDAQ.Deserializer,
  OpenDAQ.PropertyObject,
  OpenDAQ.PropertyObjectProtected,
  OpenDAQ.CallableInfo,
  OpenDAQ.ProcedureImpl,
  OpenDAQ.FunctionImpl
  ;

const
  NumVisibleProperties = SizeT(12);
  NumAllProperties = SizeT(14);

{ TTest_BaseObject }

procedure TTest_PropertyObject.Setup;
var
  Prop: IPropertyConfigPtr;
  AtomicObj: IPropertyObjectPtr;
//  DefaultValueLst: IListPtr<IInteger>;
  IntValuesLst: IListPtr<IInteger>;
  StringValuesLst: IListPtr<IString>;
  SparseValues: IDictionaryPtr<IInteger, IString>;
  PropClass: IPropertyObjectClassConfigPtr;
  PropObjManager: IPropertyObjectClassManagerPtr;
begin
  FTestPropClass := TPropertyObjectClassConfigPtr.CreatePropertyObjectClassWithManager(nil, 'Test');

  /////////////////// - 1 - ////////////////////////////

  Prop := TPropertyConfigPtr.CreateFunctionProperty('Function', TCallableInfoPtr.Create(nil, ctObject), DaqBoxValue(True));
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 2 - ////////////////////////////

  Prop := TPropertyConfigPtr.CreateFunctionProperty('Procedure', TCallableInfoPtr.Create(nil, ctUndefined), DaqBoxValue(True));
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 3 - ////////////////////////////

  Prop := TPropertyConfigPtr.Create('FloatReadOnlyPropAssigned');
  Prop.SetDefaultValue(1);
  Prop.SetValueType(ctFloat);
  Prop.SetReadOnly(True);
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 4 - ////////////////////////////

  Prop := TPropertyConfigPtr.Create('FloatProperty');
  Prop.SetValueType(ctFloat);
  Prop.SetDefaultValue(1.0);
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 5 - ////////////////////////////

//  DefaultValueLst := TListPtr<IInteger>.Create();
  IntValuesLst := TListPtr<IInteger>.Create();
  IntValuesLst.PushBack(1);
  IntValuesLst.PushBack(2);
  IntValuesLst.PushBack(3);
  IntValuesLst.PushBack(4);

  Prop := TPropertyConfigPtr.Create('ListProperty');
  Prop.SetValueType(ctList);
//  Prop.SetDefaultValue(DefaultValueLst);
  Prop.SetDefaultValue(IntValuesLst);
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 6 - ////////////////////////////

  AtomicObj := TPropertyObjectPtr.Create();

  Prop := TPropertyConfigPtr.Create('AtomicObject');
  Prop.SetValueType(ctObject);
  Prop.SetDefaultValue(AtomicObj);
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 7 - ////////////////////////////

  Prop := TPropertyConfigPtr.CreateReferenceProperty('IntProperty', TEvalValuePtr.Create('%TwoHopReference') as IEvalValue);
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 8 - ////////////////////////////

  Prop := TPropertyConfigPtr.CreateReferenceProperty('TwoHopReference', TEvalValuePtr.Create('%Referenced') as IEvalValue);
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 9 - ////////////////////////////

  StringValuesLst := TListPtr<IString>.Create();
  StringValuesLst.PushBack('a');
  StringValuesLst.PushBack('b');
  StringValuesLst.PushBack('c');

  Prop := TPropertyConfigPtr.Create('SelectionProp');
  Prop.SetValueType(ctInt);
  Prop.SetDefaultValue(0);
  Prop.SetSelectionValues(StringValuesLst);
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 10 - ////////////////////////////

  Prop := TPropertyConfigPtr.Create('SelectionPropNoList');
  Prop.SetValueType(ctInt);
  Prop.SetDefaultValue(0);
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 11, 12 - ////////////////////////////

  SparseValues := TDictionaryPtr<IInteger, IString>.Create;
  SparseValues[0] := 'a';
  SparseValues[1] := 'b';
  SparseValues[2] := 'c';
  SparseValues[5] := 'd';
  SparseValues[8] := 'e';

  Prop := TPropertyConfigPtr.Create('SparseSelectionProp');
  Prop.SetValueType(ctInt);
  Prop.SetDefaultValue(5);
  Prop.SetSelectionValues(SparseValues);
  FTestPropClass.AddProperty(Prop as IProperty);

  Prop := TPropertyConfigPtr.CreateDictProperty('DictProp', SparseValues as IDictObject, DaqBoxValue(True));
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 13 - ////////////////////////////

  Prop := TPropertyConfigPtr.CreateReferenceProperty('Kind', TEvalValuePtr.Create('%Child') as IEvalValue);
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - 14 - ////////////////////////////

  Prop := TPropertyConfigPtr.CreateIntProperty('Referenced', DaqBoxValue(10), DaqBoxValue(True));
//  Prop.GetOnPropertyValueWrite
  FTestPropClass.AddProperty(Prop as IProperty);

  /////////////////// - Manager - ////////////////////////////

  PropObjManager := TPropertyObjectClassManagerPtr.Create();
  FPropObjManager := PropObjManager as IPropertyObjectClassManager;

    /////////////////// - Classes - ////////////////////////////

  PropObjManager.AddClass(FTestPropClass as IPropertyObjectClass);

  PropClass := TPropertyObjectClassConfigPtr.CreatePropertyObjectClassWithManager(FPropObjManager, 'DerivedClass');
  PropClass.SetParentName('Test');
  PropClass.AddProperty(TPropertyConfigPtr.CreateIntProperty('AdditionalProp', DaqBoxValue(1), DaqBoxValue(True)));
  PropObjManager.AddClass(PropClass as IPropertyObjectClass);

  PropClass := TPropertyObjectClassConfigPtr.Create('BaseClass');
  PropObjManager.AddClass(PropClass as IPropertyObjectClass);

  PropClass := TPropertyObjectClassConfigPtr.Create('SpecificClass');
  PropClass.SetParentName('BaseClass');
  PropObjManager.AddClass(PropClass as IPropertyObjectClass);
end;

procedure TTest_PropertyObject.TearDown;
begin
end;

procedure TTest_PropertyObject.Create;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
end;

procedure TTest_PropertyObject.EmptyClassName;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.Create;
  Assert.AreEqual(PropObj.GetClassName.ToString, '');

  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(nil, nil);
  Assert.AreEqual(PropObj.GetClassName.ToString, '');

  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(nil, '');
  Assert.AreEqual(PropObj.GetClassName.ToString, '');

  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, nil);
  Assert.AreEqual(PropObj.GetClassName.ToString, '');

  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, '');
  Assert.AreEqual(PropObj.GetClassName.ToString, '');
end;

procedure TTest_PropertyObject.ClassName;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.AreEqual(PropObj.GetClassName.ToString, 'Test');
end;

procedure TTest_PropertyObject.SimpleProperty;
var
  PropObj: IPropertyObjectPtr;
  Prop: IPropertyConfigPtr;
  Value: string;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Prop := TPropertyConfigPtr.Create('Name');
  Prop.SetValueType(ctString);
  Prop.SetDefaultValue('');
  PropObj.AddProperty(Prop as IPropertyPtr);

  PropObj.SetPropertyValue('Name', 'Unknown');

  Value := PropObj.GetPropertyValue('Name');
  Assert.AreEqual(Value, 'Unknown');
end;

procedure TTest_PropertyObject.Ownership;
var
  Parent: IPropertyObjectPtr;
  Child: IPropertyObjectPtr;
  ChildProp: IPropertyConfigPtr;
  ChildPtr: IPropertyObjectPtr;
  Eq: Boolean;
begin
  Parent := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  Child := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', Child as IPropertyObject);
  Parent.AddProperty(ChildProp as IProperty);

  Parent.SetPropertyValue('Child', Child);
  ChildPtr := Parent.GetPropertyValue('Child').AsPtr<IPropertyObjectPtr>;

  Eq := Child.EqualsObject(ChildPtr);
  Parent.GetInterface.Dispose();

  Assert.IsTrue(Eq);
end;

{ TCallback }

type
  TCallback = record
  public
    function Func(Params: IBaseObject; out Res: IBaseObject): ErrCode;
    function Proc(Params: IBaseObject): ErrCode;
  end;

function TCallback.Func(Params: IBaseObject; out Res: IBaseObject): ErrCode;
begin
  Result := OPENDAQ_SUCCESS;
end;

function TCallback.Proc(Params: IBaseObject): ErrCode;
begin
  Result := OPENDAQ_SUCCESS;
end;

{ TTest_PropertyObject }

procedure TTest_PropertyObject.SerializeJsonSimple;
const
  Expected: string = '{"__type":"PropertyObject","className":"Test","propValues":{"Referenced":10}}';
var
  PropObj: IPropertyObjectPtr;
  Serializer: ISerializerPtr;
  Func: IFunction;
  Proc: IProcedure;
  Json: string;
  Callback: TCallback;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Func := TFunctionImpl.Create(Callback.Func);
  Proc := TProcedureImpl.Create(Callback.Proc);

  PropObj.SetPropertyValue('IntProperty', '10');
  PropObj.SetPropertyValue('Function', Func);
  PropObj.SetPropertyValue('Procedure', Proc);

  Serializer := TSerializerPtr.Create();
  (PropObj as ISerializablePtr).Serialize(Serializer as ISerializer);

  Json := Serializer.GetOutput().ToString();

  Assert.AreEqual(Expected, Json);
end;

procedure TTest_PropertyObject.DeserializeJsonSimple;
const
  Json: string = '{"__type":"PropertyObject","className":"Test","propValues":{"Referenced":10}}';
var
  Deserializer: IDeserializerPtr<IPropertyObject>;
  PropObjPtr: IPropertyObjectPtr;
begin
  Deserializer := TDeserializerPtr<IPropertyObject>.Create();
  PropObjPtr := TPropertyObjectPtr.Create(Deserializer.DeserializeRaw(Json, FPropObjManager as IBaseObject));

  Assert.AreEqual('Test', PropObjPtr.GetClassName().ToString());
  Assert.AreEqual<RtInt>(10, PropObjPtr.GetPropertyValue('Referenced'));
end;

procedure TTest_PropertyObject.SetNullProperty;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.SetPropertyValue(nil, nil);
  end, ERTArgumentNullException);
end;

procedure TTest_PropertyObject.GetNullProperty;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.GetPropertyValue(IString(nil));
  end, ERTArgumentNullException);
end;

procedure TTest_PropertyObject.ClearNullProperty;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.ClearPropertyValue(IString(nil));
  end, ERTArgumentNullException);
end;

procedure TTest_PropertyObject.ClearPropertyValue;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillNotRaiseAny(procedure()
  begin
    PropObj.ClearPropertyValue('Function');
  end);
end;

procedure TTest_PropertyObject.ClearNonExistentProperty;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.ClearPropertyValue('DoesNotExist');
  end, ERTNotFoundException);
end;

procedure TTest_PropertyObject.GetNonExistentPropertyObject;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.GetPropertyValue('DoesNotExist');
  end, ERTNotFoundException);
end;

procedure TTest_PropertyObject.GetPropertyObjectParamNull;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.GetPropertyValue(IString(nil));
  end, ERTArgumentNullException);
end;

procedure TTest_PropertyObject.EnumVisiblePropertyWhenClassNull;
var
  PropObj: IPropertyObjectPtr;
  Props: IListPtr<IProperty>;
begin
  PropObj := TPropertyObjectPtr.Create;
  Props := PropObj.GetVisibleProperties();

  Assert.AreEqual<SizeT>(0, Props.GetCount());
end;

procedure TTest_PropertyObject.GetPropertyObjectWhenClassNull;
var
  PropObj: IPropertyObjectPtr;
  Prop: IPropertyPtr;
begin
  PropObj := TPropertyObjectPtr.Create;

  Assert.WillRaise(procedure()
  begin
    Prop := PropObj.GetProperty('Test');
  end, ERTNotFoundException);
end;

procedure TTest_PropertyObject.SelectionPropertiesInsertionOrder;
var
  PropObj: IPropertyObjectPtr;
  ChildProp: IPropertyConfigPtr;

  Props: IListPtr<IProperty>;
  Order: Integer;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'DerivedClass');

  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', TPropertyObjectPtr.Create);
  PropObj.AddProperty(ChildProp as IProperty);

  Props := PropObj.GetAllProperties;
  Assert.AreEqual<SizeT>(NumAllProperties + 2, Props.GetCount);

  Order := 0;
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'Function');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'Procedure');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'FloatReadOnlyPropAssigned');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'FloatProperty');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'ListProperty');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'AtomicObject');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'IntProperty');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'TwoHopReference');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'SelectionProp');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'SelectionPropNoList');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'SparseSelectionProp');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'DictProp');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'Kind');
  // Derived class properties after base
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'Referenced');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'AdditionalProp');
end;

procedure TTest_PropertyObject.SelectionPropertiesCustomOrder;
var
  PropObj: IPropertyObjectPtr;
  ChildProp: IPropertyConfigPtr;

  PropOrder: IListPtr<IString>;
  Props: IListPtr<IProperty>;
  Order: Integer;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'DerivedClass');

  PropOrder := TListPtr<IString>.Create;
  PropOrder.PushBack('Kind');
  PropOrder.PushBack('AdditionalProp');

  propObj.setPropertyOrder(PropOrder);

  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', TPropertyObjectPtr.Create);
  PropObj.AddProperty(ChildProp as IProperty);

  Props := PropObj.GetAllProperties;
  Assert.AreEqual<SizeT>(NumAllProperties + 2, Props.GetCount);

  Order := 0;
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'Kind');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'AdditionalProp');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'Function');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'Procedure');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'FloatReadOnlyPropAssigned');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'FloatProperty');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'ListProperty');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'AtomicObject');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'IntProperty');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'TwoHopReference');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'SelectionProp');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'SelectionPropNoList');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'SparseSelectionProp');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'DictProp');
  Inc(order);
  Assert.AreEqual(props[order].AsPtr<IPropertyPtr>.GetName.ToString, 'Referenced');
end;

procedure TTest_PropertyObject.SparseSelectionPropertyGet;
var
  Dict: IDictionaryPtr<IInteger, IString>;
  PropObj: IPropertyObjectPtr;
  SelectionValue: string;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  Dict := PropObj.GetProperty('SparseSelectionProp').GetSelectionValues.AsPtr<IDictionaryPtr<IInteger, IString>>;

  SelectionValue := PropObj.GetPropertySelectionValue('SparseSelectionProp');
  Assert.AreEqual<string>(SelectionValue, Dict[5]);
end;

procedure TTest_PropertyObject.SparseSelectionPropertySet;
var
  Dict: IDictionaryPtr<IInteger, IString>;
  PropObj: IPropertyObjectPtr;
  SelectionValue: string;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  Dict := PropObj.GetProperty('SparseSelectionProp').GetSelectionValues.AsPtr<IDictionaryPtr<IInteger, IString>>;

  PropObj.SetPropertyValue('SparseSelectionProp', 8);

  SelectionValue := PropObj.GetPropertySelectionValue('SparseSelectionProp');
  Assert.AreEqual<string>(SelectionValue, Dict[8]);
end;

procedure TTest_PropertyObject.SparseSelectionPropertySetInvalid;
var
  Dict: IDictionaryPtr<IInteger, IString>;
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  Dict := PropObj.GetProperty('SparseSelectionProp').GetSelectionValues.AsPtr<IDictionaryPtr<IInteger, IString>>;

  Assert.WillRaiseAny(procedure()
  begin
    PropObj.SetPropertyValue('SparseSelectionProp', 10);
  end);
end;

procedure TTest_PropertyObject.EnumVisibleWithVisibleThroughRefs;
var
  PropObj: IPropertyObjectPtr;
  ChildProp: IPropertyConfigPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', TPropertyObjectPtr.Create);
  PropObj.AddProperty(ChildProp as IProperty);

  Assert.AreEqual<SizeT>(PropObj.GetAllProperties.GetCount, NumAllProperties + 1);
  Assert.AreEqual<SizeT>(PropObj.GetVisibleProperties.GetCount, NumVisibleProperties);
end;

procedure TTest_PropertyObject.TrySetReadOnlyProperty;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.SetPropertyValue('FloatReadOnlyPropAssigned', 3.33)
  end, EDaqAccessDeniedException);
end;

procedure TTest_PropertyObject.SetProtectedReadOnlyProperty;
var
  PropObj: IPropertyObjectPtr;
  PropProtected: IPropertyObjectProtectedPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  PropProtected := TPropertyObjectProtectedPtr.Create(PropObj as IPropertyObjectProtected);

  Assert.WillNotRaiseAny(procedure()
  begin
    PropProtected.SetProtectedPropertyValue('FloatReadOnlyPropAssigned', 3.33);
  end);

  Assert.AreEqual<RtFloat>(3.33, PropObj.getPropertyValue('FloatReadOnlyPropAssigned'));
end;

procedure TTest_PropertyObject.ReadOnlyPropertyAssigned;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  Assert.AreEqual<RtInt>(1, PropObj.GetPropertyValue('FloatReadOnlyPropAssigned'));
end;

procedure TTest_PropertyObject.TryClearReadOnlyProperty;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.clearPropertyValue('FloatReadOnlyPropAssigned')
  end, EDaqAccessDeniedException);
end;

procedure TTest_PropertyObject.ClearProtectedReadOnlyProperty;
var
  PropObj: IPropertyObjectPtr;
  PropProtected: IPropertyObjectProtectedPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  PropProtected := TPropertyObjectProtectedPtr.Create(PropObj as IPropertyObjectProtected);

  PropProtected.SetProtectedPropertyValue('FloatReadOnlyPropAssigned', 3.33);

  Assert.WillNotRaiseAny(procedure()
  begin
    PropProtected.clearProtectedPropertyValue('FloatReadOnlyPropAssigned')
  end);

  Assert.AreEqual<RtFloat>(PropObj.GetPropertyValue('FloatReadOnlyPropAssigned'), 1.0);
end;

procedure TTest_PropertyObject.ValueNotFound;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.GetPropertyValue('SomeNoneExistingProperty');
  end, ERTNotFoundException);
end;

procedure TTest_PropertyObject.ConvertToPropertyCoreType;
var
  PropObj: IPropertyObjectPtr;
  ValuePtr: IObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  PropObj.SetPropertyValue('FloatProperty', '1');
  ValuePtr := PropObj.GetPropertyValue('FloatProperty');

  Assert.AreEqual(ValuePtr.GetCoreType(), ctFloat);
end;

procedure TTest_PropertyObject.ConvertToPropertyCoreTypeFails;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.SetPropertyValue('FloatProperty', 'a');
  end, ERTCoversionFailedException);
end;

procedure TTest_PropertyObject.ConvertToPropertyCoreTypeFails2;
var
  PropObj: IPropertyObjectPtr;
  List: IListPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  List := TListPtr<IBaseObject>.Create();

  Assert.WillRaise(procedure()
  begin
    PropObj.SetPropertyValue('FloatProperty', List);
  end, ERTNoInterfaceException);
end;

procedure TTest_PropertyObject.SetNullPropertyValue;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.SetPropertyValue('FloatProperty', nil);
  end, ERTArgumentNullException);
end;

procedure TTest_PropertyObject.SelectionProp;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  PropObj.SetPropertyValue('SelectionProp', 1);

  Assert.AreEqual<RtInt>(propObj.GetPropertyValue('SelectionProp'), 1);
  Assert.AreEqual<string>(propObj.GetPropertySelectionValue('SelectionProp'), 'b');
end;

procedure TTest_PropertyObject.SelectionPropNoEnum;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.GetPropertySelectionValue('IntProperty');
  end, ERTInvalidPropertyException);
end;

procedure TTest_PropertyObject.SelectionPropNotRegistered;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.GetPropertySelectionValue('TestProp');
  end, ERTNotFoundException);
end;

procedure TTest_PropertyObject.SelectionPropNoList;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillRaise(procedure()
  begin
    PropObj.GetPropertySelectionValue('SelectionPropNoList');
  end, ERTInvalidPropertyException);
end;

procedure TTest_PropertyObject.DictProp;
var
  Dict: IDictionaryPtr<IInteger, IString>;
  PropObj: IPropertyObjectPtr;
  NewDict: IDictionaryPtr<IInteger, IString>;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.WillNotRaiseAny(procedure()
  begin
    Dict := PropObj.GetPropertyValue('DictProp').AsPtr<IDictionaryPtr<IInteger, IString>>;
  end);
  Assert.AreEqual<string>('a', Dict[0]);

  Dict := TDictionaryPtr<IInteger, IString>.Create;
  Dict.SetItem(0, 'g');

  PropObj.SetPropertyValue('DictProp', Dict);

  NewDict := PropObj.GetPropertyValue('DictProp').AsPtr<IDictionaryPtr<IInteger, IString>>;
  Assert.AreEqual<string>(NewDict[0], 'g');
end;

procedure TTest_PropertyObject.ChildPropSet;
var
  PropObj: IPropertyObjectPtr;
  ChildObj: IPropertyObjectPtr;
  ChildProp: IPropertyConfigPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  ChildObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', ChildObj as IPropertyObject);

  PropObj.AddProperty(ChildProp as IProperty);
  PropObj.SetPropertyValue('Child', ChildObj);
  PropObj.SetPropertyValue('Child.IntProperty', 1);

  Assert.AreEqual<RtInt>(1, ChildObj.GetPropertyValue('IntProperty'));
end;

procedure TTest_PropertyObject.NestedChildPropSet;
var
  PropObj: IPropertyObjectPtr;
  ChildObj1: IPropertyObjectPtr;
  ChildProp1: IProperty;
  ChildObj2: IPropertyObjectPtr;
  ChildProp2: IProperty;
  ChildObj3: IPropertyObjectPtr;
  ChildProp3: IProperty;
  ChildObj4: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  ChildObj1 := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  ChildObj2 := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  ChildObj3 := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  ChildObj4 := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  ChildProp1 := TPropertyConfigPtr.CreateObjectProperty('Child', ChildObj1 as IPropertyObject);
  PropObj.AddProperty(ChildProp1);

  ChildProp2 := TPropertyConfigPtr.CreateObjectProperty('Child', ChildObj2 as IPropertyObject);
  ChildObj1.AddProperty(ChildProp2);

  ChildProp3 := TPropertyConfigPtr.CreateObjectProperty('Child', ChildObj3 as IPropertyObject);
  ChildObj2.AddProperty(ChildProp3);

  PropObj.SetPropertyValue('Child.IntProperty', 1);
  PropObj.SetPropertyValue('Child.Child.IntProperty', 2);
  PropObj.SetPropertyValue('Child.Child.Child.IntProperty', 3);

  Assert.AreEqual<RtInt>(childObj1.GetPropertyValue('IntProperty'), 1);
  Assert.AreEqual<RtInt>(childObj2.GetPropertyValue('IntProperty'), 2);
  Assert.AreEqual<RtInt>(childObj3.GetPropertyValue('IntProperty'), 3);
end;

procedure TTest_PropertyObject.ChildPropGet;
var
  PropObj: IPropertyObjectPtr;
  ChildObj: IPropertyObjectPtr;
  ChildProp: IPropertyConfigPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  ChildObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', ChildObj as IPropertyObject);

  PropObj.AddProperty(ChildProp as IProperty);
  PropObj.SetPropertyValue('Child', ChildObj);
  ChildObj.SetPropertyValue('IntProperty', 2);

  Assert.AreEqual<RtInt>(2, PropObj.GetPropertyValue('Child.IntProperty'));
end;

//////////////////////////////

procedure TTest_PropertyObject.ChildPropClear;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     auto childObj = PropertyObject(objManager, "Test");
//
//     const auto childProp = ObjectProperty("Child", childObj);
//     propObj.addProperty(childProp);
//
//     propObj.setPropertyValue("Child", childObj);
//     childObj.setPropertyValue("IntProperty", 2);
//
//     ASSERT_NO_THROW(propObj.clearPropertyValue("Child.IntProperty"));
//     ASSERT_EQ(childObj.getPropertyValue("IntProperty"), 10);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.NestedChildPropGet;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     auto childObj1 = PropertyObject(objManager, "Test");
//     auto childObj2 = PropertyObject(objManager, "Test");
//     auto childObj3 = PropertyObject(objManager, "Test");
//
//     const auto childProp1 = ObjectProperty("Child", childObj1);
//     propObj.addProperty(childProp1);
//
//     const auto childProp2 = ObjectProperty("Child", childObj2);
//     childObj1.addProperty(childProp2);
//
//     const auto childProp3 = ObjectProperty("Child", childObj3);
//     childObj2.addProperty(childProp3);
//
//     childObj1.setPropertyValue("IntProperty", 1);
//     childObj2.setPropertyValue("IntProperty", 2);
//     childObj3.setPropertyValue("IntProperty", 3);
//
//     ASSERT_EQ(propObj.getPropertyValue("Child.IntProperty"), 1);
//     ASSERT_EQ(propObj.getPropertyValue("Child.Child.IntProperty"), 2);
//     ASSERT_EQ(propObj.getPropertyValue("Child.Child.Child.IntProperty"), 3);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.NestedChildPropClear;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     auto childObj1 = PropertyObject(objManager, "Test");
//     auto childObj2 = PropertyObject(objManager, "Test");
//     auto childObj3 = PropertyObject(objManager, "Test");
//
//     const auto childProp1 = ObjectProperty("Child", childObj1);
//     propObj.addProperty(childProp1);
//
//     const auto childProp2 = ObjectProperty("Child", childObj2);
//     childObj1.addProperty(childProp2);
//
//     const auto childProp3 = ObjectProperty("Child", childObj3);
//     childObj2.addProperty(childProp3);
//
//     childObj1.setPropertyValue("IntProperty", 1);
//     childObj2.setPropertyValue("IntProperty", 2);
//     childObj3.setPropertyValue("IntProperty", 3);
//
//     ASSERT_NO_THROW(propObj.clearPropertyValue("Child.IntProperty"));
//     ASSERT_NO_THROW(propObj.clearPropertyValue("Child.Child.IntProperty"));
//     ASSERT_NO_THROW(propObj.clearPropertyValue("Child.Child.Child.IntProperty"));
//
//     ASSERT_EQ(childObj1.getPropertyValue("IntProperty"), 10);
//     ASSERT_EQ(childObj2.getPropertyValue("IntProperty"), 10);
//     ASSERT_EQ(childObj3.getPropertyValue("IntProperty"), 10);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.ChildPropSetViaRefProp;
var
  PropObj: IPropertyObjectPtr;
  ChildObj: IPropertyObjectPtr;
  ChildProp: IProperty;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  ChildObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', ChildObj as IPropertyObject);

  PropObj.AddProperty(ChildProp);
  PropObj.SetPropertyValue('Child', ChildObj);
  PropObj.SetPropertyValue('Kind.IntProperty', 1);

  Assert.AreEqual<RtInt>(ChildObj.GetPropertyValue('IntProperty'), 1);
end;

procedure TTest_PropertyObject.NestedChildPropSetViaRefProp;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     auto childObj1 = PropertyObject(objManager, "Test");
//     auto childObj2 = PropertyObject(objManager, "Test");
//     auto childObj3 = PropertyObject(objManager, "Test");
//
//     const auto childProp1 = ObjectProperty("Child", childObj1);
//     propObj.addProperty(childProp1);
//
//     const auto childProp2 = ObjectProperty("Child", childObj2);
//     childObj1.addProperty(childProp2);
//
//     const auto childProp3 = ObjectProperty("Child", childObj3);
//     childObj2.addProperty(childProp3);
//
//     propObj.setPropertyValue("Kind.IntProperty", 1);
//     propObj.setPropertyValue("Kind.Kind.IntProperty", 2);
//     propObj.setPropertyValue("Kind.Kind.Kind.IntProperty", 3);
//
//     ASSERT_EQ(childObj1.getPropertyValue("IntProperty"), 1);
//     ASSERT_EQ(childObj2.getPropertyValue("IntProperty"), 2);
//     ASSERT_EQ(childObj3.getPropertyValue("IntProperty"), 3);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.ChildPropGetViaRefProp;
var
  PropObj: IPropertyObjectPtr;
  ChildObj: IPropertyObjectPtr;
  ChildProp: IProperty;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  ChildObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', ChildObj as IPropertyObject);

  PropObj.AddProperty(ChildProp);
  PropObj.SetPropertyValue('Child', ChildObj);
  ChildObj.SetPropertyValue('IntProperty', 2);

  Assert.AreEqual<RtInt>(PropObj.GetPropertyValue('Kind.IntProperty'), 2);
end;

procedure TTest_PropertyObject.ChildPropClearViaRefProp;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     auto childObj = PropertyObject(objManager, "Test");
//
//     const auto childProp = ObjectProperty("Child", childObj);
//     propObj.addProperty(childProp);
//
//     propObj.setPropertyValue("Child", childObj);
//     childObj.setPropertyValue("IntProperty", 1);
//
//     ASSERT_NO_THROW(propObj.clearPropertyValue("Kind.IntProperty"));
//     ASSERT_EQ(childObj.getPropertyValue("IntProperty"), 10);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.NestedChildPropGetViaRefProp;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     auto childObj1 = PropertyObject(objManager, "Test");
//     auto childObj2 = PropertyObject(objManager, "Test");
//     auto childObj3 = PropertyObject(objManager, "Test");
//
//     const auto childProp1 = ObjectProperty("Child", childObj1);
//     propObj.addProperty(childProp1);
//
//     const auto childProp2 = ObjectProperty("Child", childObj2);
//     childObj1.addProperty(childProp2);
//
//     const auto childProp3 = ObjectProperty("Child", childObj3);
//     childObj2.addProperty(childProp3);
//
//     childObj1.setPropertyValue("IntProperty", 1);
//     childObj2.setPropertyValue("IntProperty", 2);
//     childObj3.setPropertyValue("IntProperty", 3);
//
//     ASSERT_EQ(propObj.getPropertyValue("Kind.IntProperty"), 1);
//     ASSERT_EQ(propObj.getPropertyValue("Kind.Kind.IntProperty"), 2);
//     ASSERT_EQ(propObj.getPropertyValue("Kind.Kind.Kind.IntProperty"), 3);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.NestedChildPropClearViaRefProp;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     auto childObj1 = PropertyObject(objManager, "Test");
//     auto childObj2 = PropertyObject(objManager, "Test");
//     auto childObj3 = PropertyObject(objManager, "Test");
//
//     const auto childProp1 = ObjectProperty("Child", childObj1);
//     propObj.addProperty(childProp1);
//
//     const auto childProp2 = ObjectProperty("Child", childObj2);
//     childObj1.addProperty(childProp2);
//
//     const auto childProp3 = ObjectProperty("Child", childObj3);
//     childObj2.addProperty(childProp3);
//
//     childObj1.setPropertyValue("IntProperty", 1);
//     childObj2.setPropertyValue("IntProperty", 2);
//     childObj3.setPropertyValue("IntProperty", 3);
//
//     ASSERT_NO_THROW(propObj.clearPropertyValue("Kind.IntProperty"));
//     ASSERT_NO_THROW(propObj.clearPropertyValue("Kind.Kind.IntProperty"));
//     ASSERT_NO_THROW(propObj.clearPropertyValue("Kind.Kind.Kind.IntProperty"));
//
//     ASSERT_EQ(childObj1.getPropertyValue("IntProperty"), 10);
//     ASSERT_EQ(childObj2.getPropertyValue("IntProperty"), 10);
//     ASSERT_EQ(childObj3.getPropertyValue("IntProperty"), 10);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.OnValueChange;
begin

// auto propObj = PropertyObject(objManager, "Test");
// BaseObjectPtr propValue;
// StringPtr propName;
//
// propObj.getOnPropertyValueWrite("Referenced") += [&propValue, &propName](PropertyObjectPtr& sender, PropertyValueEventArgsPtr& args)
// {
//     auto prop = args.getProperty();
//
//     propValue = sender.getPropertyValue("IntProperty");
//     propName = prop.getName();
// };
//
// propObj.setPropertyValue("IntProperty", 2);
// ASSERT_EQ(propValue, 2);
// ASSERT_EQ(propName, "Referenced");

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.OnValueChangeClear;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     Int numCallbacks = 0;
//
//     propObj.getOnPropertyValueWrite("Referenced") +=
//         [&numCallbacks](PropertyObjectPtr& /*sender*/, PropertyValueEventArgsPtr& /*args*/)
//     {
//         numCallbacks++;
//     };
//
//     propObj.setPropertyValue("IntProperty", 2);
//     propObj.clearPropertyValue("IntProperty");
//     ASSERT_EQ(numCallbacks, 2);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.OnValueChangePropertyEvent;
begin

// auto propObj = PropertyObject(objManager, "Test");
//
// propObj.setPropertyValue("Referenced", 2);
// ASSERT_EQ(propValue, 2);
// ASSERT_EQ(propName, "Referenced");

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.InheritedParent;
begin
//     auto newTestPropClass = PropertyObjectClass(objManager, "NewTest");
//     newTestPropClass.setParentName("Test");
//     newTestPropClass.addProperty(FloatProperty("NewFloatProperty", 1.2));
//     objManager.addClass(newTestPropClass);
//
//     auto propObj = PropertyObject(objManager, "Test");
//
//     const auto childProp = ObjectProperty("Child", PropertyObject());
//     propObj.addProperty(childProp);
//
//     auto props = propObj.getVisibleProperties();
//     ASSERT_EQ(NumVisibleProperties, props.getCount());
//
//     propObj.release();
//     objManager.removeClass("NewTest");

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.LocalProperties;
var
  PropObj: IPropertyObjectPtr;
  LocalProp: IPropertyConfigPtr;
  ChildProp: IProperty;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', TPropertyObjectPtr.Create as IPropertyObject);
  PropObj.AddProperty(ChildProp);

  Assert.AreEqual<SizeT>(NumVisibleProperties, PropObj.GetVisibleProperties().GetCount());

  LocalProp := TPropertyConfigPtr.Create('LocalProp');
  LocalProp.SetValueType(ctInt);
  LocalProp.SetDefaultValue(1);
  PropObj.AddProperty(LocalProp as IProperty);

  Assert.AreEqual<SizeT>(NumVisibleProperties + 1, PropObj.GetVisibleProperties().GetCount());
  Assert.AreEqual<RtInt>(1, PropObj.GetPropertyValue('LocalProp'));
end;

procedure TTest_PropertyObject.LocalSelectionProperty;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//
//     auto localSelectionProp = Property("LocalSelectionProp");
//     localSelectionProp.setValueType(ctInt);
//     auto selectionValues = List<IString>("one", "two", "three");
//     localSelectionProp.setSelectionValues(selectionValues);
//     localSelectionProp.setDefaultValue(0);
//     propObj.addProperty(localSelectionProp);
//
//     auto value = propObj.getPropertyValue("LocalSelectionProp");
//     ASSERT_EQ(value, 0);
//     auto valueAsText = propObj.getPropertySelectionValue("LocalSelectionProp");
//     ASSERT_EQ(valueAsText, "one");
//
//     propObj.setPropertyValue("LocalSelectionProp", 1);
//     value = propObj.getPropertyValue("LocalSelectionProp");
//     ASSERT_EQ(value, 1);
//     valueAsText = propObj.getPropertySelectionValue("LocalSelectionProp");
//     ASSERT_EQ(valueAsText, "two");

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.NotLocalProperty;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     auto classProp = propObj.getProperty("FloatProperty");

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.EventSubscriptionCount;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.AreEqual<SizeT>(0, PropObj.GetOnPropertyValueWrite('FloatProperty').GetSubscriberCount());
end;

procedure TTest_PropertyObject.EventSubscriptionMuteFreeFunction;
begin

end;

procedure TTest_PropertyObject.EventSubscriptionMute;
begin

end;

procedure TTest_PropertyObject.PropertyListIndexerOutOfRange;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  PropObj.SetPropertyValue('ListProperty', TListPtr<IBaseObject>.Create());

  Assert.WillRaise(procedure()
  begin
    PropObj.GetPropertyValue('ListProperty[3]');
  end, ERTOutOfRangeException);
end;

procedure TTest_PropertyObject.PropertyIndexer;
var
  PropObj: IPropertyObjectPtr;
  List: IListPtr<IInteger>;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  List := TListPtr<IInteger>.Create();
  List.PushBack(4);
  List.PushBack(3);
  List.PushBack(2);
  List.PushBack(1);

  PropObj.SetPropertyValue('ListProperty', List);

  Assert.AreEqual<RtInt>(1, PropObj.GetPropertyValue('ListProperty[3]'));
end;

procedure TTest_PropertyObject.PropertyNotListIndexer;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  PropObj.SetPropertyValue('IntProperty', 5);

  Assert.WillRaise(procedure()
  begin
    PropObj.GetPropertyValue('IntProperty[3]');
  end, ERTInvalidParameterException);
end;

procedure TTest_PropertyObject.ChildPropGetArray;
var
  ParentObj: IPropertyObjectPtr;
  ChildObj: IPropertyObjectPtr;
  ChildProp: IProperty;
  List: IListPtr<IInteger>;
begin
  ParentObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  ChildObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  ChildProp := TPropertyConfigPtr.CreateObjectProperty('Child', TPropertyObjectPtr.Create as IPropertyObject);
  ParentObj.AddProperty(ChildProp);

  List := TListPtr<IInteger>.Create();
  List.PushBack(2);
  List.PushBack(1);

  ParentObj.SetPropertyValue('Child', ChildObj);
  ChildObj.SetPropertyValue('ListProperty', List);

  Assert.AreEqual<RtInt>(1, ParentObj.GetPropertyValue('Child.ListProperty[1]'));
end;

procedure TTest_PropertyObject.HasPropertyFalse;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  Assert.IsFalse(PropObj.HasProperty('DoesNotExist'));
end;

procedure TTest_PropertyObject.HasPropertyPrivate;
var
  PropObj: IPropertyObjectPtr;
  Prop: IPropertyConfigPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  Prop := TPropertyConfigPtr.Create('SomeProperty');
  Prop.SetDefaultValue('foo');
  PropObj.AddProperty(Prop as IProperty);

  Assert.IsTrue(PropObj.HasProperty('SomeProperty'));
end;

procedure TTest_PropertyObject.HasPropertyOnClass;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  Assert.IsTrue(PropObj.HasProperty('SelectionProp'));
end;

procedure TTest_PropertyObject.HasPropertyOnClassParent;
begin
//     auto parentClass = PropertyObjectClass(objManager, "Parent");
//     parentClass.addProperty(BoolProperty("IsTest", true));
//
//     auto baseClass = PropertyObjectClass(objManager, "Base");
//     baseClass.setParentName("Parent");
//
//     objManager.addClass(parentClass);
//     objManager.addClass(baseClass);
//
//     auto propObj = PropertyObject(objManager, "Base");
//     ASSERT_TRUE(propObj.hasProperty("IsTest"));

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.HasPropertyLocal;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
  PropObj.AddProperty(TPropertyConfigPtr.CreateBoolProperty('BoolProp', DaqBoxValue(False), DaqBoxValue(True)));

  Assert.IsTrue(PropObj.HasProperty('BoolProp'));
end;

procedure TTest_PropertyObject.TestToString;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');

  Assert.AreEqual(PropObj.ToString, 'PropertyObject {Test}')
end;

procedure TTest_PropertyObject.ToStringWithoutPropertyClass;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.Create();
  Assert.AreEqual(PropObj.ToString, 'PropertyObject');
end;

procedure TTest_PropertyObject.ValidatorTestEvalVale;
begin
//     ValidatorPtr validator = Validator("value > 5");
//     ASSERT_NO_THROW(validator.validate(nullptr, 10));
//     ASSERT_THROW(validator.validate(nullptr, 0), ValidateFailedException);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.CoercerTestEvalValue;
begin
//     CoercerPtr coercer = Coercer("value + 2");
//     ASSERT_EQ(coercer.coerce(nullptr, 10), 12);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.PropertyCoerceEvalValue;
begin
//     std::string propertyName = "CoerceProp";
//     Float value = 10.2;
//
//     auto ptr = FloatProperty(propertyName, value);
//     CoercerPtr coercer = Coercer("value + 2");
//     ptr.setCoercer(coercer);
//
//     auto obj = PropertyObject(objManager, "Test");
//
//     obj.addProperty(ptr);
//     obj.setPropertyValue(propertyName, value);
//
//     Float validatedValue = obj.getPropertyValue(propertyName);
//
//     ASSERT_DOUBLE_EQ(value + 2.0, validatedValue);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.PropertyCoerceFailedEvalValue;
begin
//     std::string propertyName = "CoerceProp";
//     Float value = 10.2;
//
//     auto obj = PropertyObject(objManager, "Test");
//
//     auto ptr = FloatProperty(propertyName, value);
//     CoercerPtr coercer = Coercer("if(value == foo, 5, value)");
//     ptr.setCoercer(coercer);
//
//     obj.addProperty(ptr);
//     ASSERT_THROW(obj.setPropertyValue(propertyName, value), CoerceFailedException);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.PropertyValidateEvalValue;
begin
//     std::string propertyName = "ValidateProp";
//
//     auto ptr = FloatProperty(propertyName, 1.1);
//     ValidatorPtr validator = Validator("($SelectionProp == 2) && (value == 10.2)");
//     ptr.setValidator(validator);
//
//     auto obj = PropertyObject(objManager, "Test");
//     obj.addProperty(ptr);
//     obj.setPropertyValue("SelectionProp", 2);
//
//     ErrCode err = obj->setPropertyValue(String(propertyName), Floating(10.2));
//     ASSERT_EQ(err, OPENDAQ_SUCCESS);
//     ASSERT_NO_THROW(obj.setPropertyValue(propertyName, 10.2));

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.PropertyValidateFailedEvalValue;
begin
//     std::string propertyName = "ValidateProp";
//
//     auto ptr = FloatProperty(propertyName, 1.1);
//     ValidatorPtr validator = Validator("value != 10.2");
//     ptr.setValidator(validator);
//
//     auto obj = PropertyObject(objManager, "Test");
//     obj.addProperty(ptr);
//
//     ErrCode err = obj->setPropertyValue(String(propertyName), Floating(10.2));
//     ASSERT_EQ(err, OPENDAQ_ERR_VALIDATE_FAILED);
//
//     ASSERT_THROW(obj.setPropertyValue(propertyName, 10.2), ValidateFailedException);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.PropertyWriteValidateEvalValueMultipleTimes;
begin
//     std::string propertyName = "ValidateProp";
//
//     auto ptr = FloatProperty(propertyName, 1.1);
//     ValidatorPtr validator = Validator("($SelectionProp == 2) && (value > 10)");
//     ptr.setValidator(validator);
//
//     auto obj = PropertyObject(objManager, "Test");
//     obj.addProperty(ptr);
//     obj.setPropertyValue("SelectionProp", 2);
//
//     ErrCode err = obj->setPropertyValue(String(propertyName), Floating(10.2));
//     ASSERT_EQ(err, OPENDAQ_SUCCESS);
//     ASSERT_NO_THROW(obj.setPropertyValue(propertyName, 10.2));
//
//     err = obj->setPropertyValue(String(propertyName), Floating(11.2));
//     ASSERT_EQ(err, OPENDAQ_SUCCESS);
//     ASSERT_NO_THROW(obj.setPropertyValue(propertyName, 11.2));
//
//     err = obj->setPropertyValue(String(propertyName), Floating(12.2));
//     ASSERT_EQ(err, OPENDAQ_SUCCESS);
//     ASSERT_NO_THROW(obj.setPropertyValue(propertyName, 12.2));
//
//     err = obj->setPropertyValue(String(propertyName), Floating(5.0));
//     ASSERT_EQ(err, OPENDAQ_ERR_VALIDATE_FAILED);
//     ASSERT_THROW(obj.setPropertyValue(propertyName, 5), ValidateFailedException);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.ValidatorSerialize;
begin
//     std::string expectedJson = R"({"__type":"Validator","EvalStr":"value == 10"})";
//     auto serializer = JsonSerializer();
//
//     ValidatorPtr validator = Validator("value == 10");
//     validator.serialize(serializer);
//     std::string json = serializer.getOutput();
//     ASSERT_EQ(json, expectedJson);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.ValidatorDeserialize;
begin
//     std::string serializedJson = R"({"__type":"Validator","EvalStr":"value == 10"})";
//
//     auto deserializer = JsonDeserializer();
//     ValidatorPtr validator = deserializer.deserialize(serializedJson);
//
//     auto serializer = JsonSerializer();
//     validator.serialize(serializer);
//
//     std::string deserialized = serializer.getOutput();
//
//     ASSERT_EQ(serializedJson, deserialized);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.CoercerSerialize;
begin
//     std::string expectedJson = R"({"__type":"Coercer","EvalStr":"value == 10"})";
//     auto serializer = JsonSerializer();
//
//     CoercerPtr coercer = Coercer("value == 10");
//     coercer.serialize(serializer);
//     std::string json = serializer.getOutput();
//     ASSERT_EQ(json, expectedJson);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.CoercerDeserialize;
begin
//     std::string serializedJson = R"({"__type":"Coercer","EvalStr":"value == 10"})";
//
//     auto deserializer = JsonDeserializer();
//     CoercerPtr coercer = deserializer.deserialize(serializedJson);
//
//     auto serializer = JsonSerializer();
//     coercer.serialize(serializer);
//
//     std::string deserialized = serializer.getOutput();
//
//     ASSERT_EQ(serializedJson, deserialized);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.SetPropertyWhenFrozen;
begin
//     auto propObj = PropertyObject(objManager, "Test");
//     propObj.freeze();
//
//     ASSERT_THROW(propObj.setPropertyValue("test", "testing"), FrozenException);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.ClearPropertyWhenFrozen;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
//     auto propObj = PropertyObject(objManager, "Test");
//     propObj.freeze();
//
//     ASSERT_THROW(propObj.clearPropertyValue("test"), FrozenException);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.RegisterPropertyWhenFrozen;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
//     auto propObj = PropertyObject(objManager, "Test");
//     propObj.freeze();
//
//     auto localProp = IntProperty("LocalProperty", 0);
//     ASSERT_THROW(testPropClass.addProperty(localProp), FrozenException);
//     ASSERT_THROW(propObj.addProperty(localProp), FrozenException);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.SetOwnerWhenFrozen;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
//     auto propObj = PropertyObject(objManager, "Test");
//     propObj.freeze();
//
//     auto ownable = propObj.asPtr<IOwnable>(true);
//     ASSERT_NO_THROW(ownable.setOwner(nullptr));
//
//     auto newOwner = PropertyObject();
//     ASSERT_NO_THROW(ownable.setOwner(newOwner));

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.AutoCoerceMin;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
//     auto propObj = PropertyObject(objManager, "Test");
//     auto intPropInfo = IntProperty("IntProperty2", 12);
//     intPropInfo.setMinValue(10);
//     propObj.addProperty(intPropInfo);
//
//     auto floatPropInfo = FloatProperty("FloatProperty2", 12.1);
//     floatPropInfo.setMinValue(Floating(10.0));
//     propObj.addProperty(floatPropInfo);
//
//     propObj.setPropertyValue("IntProperty2", 5);
//     ASSERT_EQ(propObj.getPropertyValue("IntProperty2"), 10);
//
//     propObj.setPropertyValue("FloatProperty2", 5.0);
//     ASSERT_EQ(propObj.getPropertyValue("FloatProperty2"), 10.0);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.AutoCoerceMax;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
//     auto propObj = PropertyObject(objManager, "Test");
//     auto intPropInfo = IntProperty("IntProperty2", 1);
//     intPropInfo.setMaxValue(10);
//     propObj.addProperty(intPropInfo);
//
//     auto floatPropInfo = FloatProperty("FloatProperty2", 1.1);
//     floatPropInfo.setMaxValue(Floating(10.0));
//     propObj.addProperty(floatPropInfo);
//
//     propObj.setPropertyValue("IntProperty2", 50);
//     ASSERT_EQ(propObj.getPropertyValue("IntProperty2"), 10);
//
//     propObj.setPropertyValue("FloatProperty2", 50.0);
//     ASSERT_EQ(propObj.getPropertyValue("FloatProperty2"), 10.0);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.AutoCoerceMinEvalValue;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
//     auto propObj = PropertyObject(objManager, "Test");
//
//     auto minPropInfo = IntProperty("MinProperty", 10);
//     propObj.addProperty(minPropInfo);
//
//     auto intPropInfo = IntProperty("IntProperty2" , 12);
//     intPropInfo.setMinValue(EvalValue("$MinProperty"));
//     propObj.addProperty(intPropInfo);
//
//     auto floatPropInfo = FloatProperty("FloatProperty2", 12.1);
//     floatPropInfo.setMinValue(EvalValue("$MinProperty - 3"));
//     propObj.addProperty(floatPropInfo);
//
//     propObj.setPropertyValue("IntProperty2", 5);
//     ASSERT_EQ(propObj.getPropertyValue("IntProperty2"), 10);
//
//     propObj.setPropertyValue("FloatProperty2", 5.0);
//     ASSERT_EQ(propObj.getPropertyValue("FloatProperty2"), 7.0);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.AutoCoerceMaxEvalValue;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.CreateWithClassAndManager(FPropObjManager, 'Test');
//     auto propObj = PropertyObject(objManager, "Test");
//
//     auto maxPropInfo = IntProperty("MaxProperty", 10);
//     propObj.addProperty(maxPropInfo);
//
//     auto intPropInfo = IntProperty("IntProperty2", 1);
//     intPropInfo.setMaxValue(EvalValue("$MaxProperty"));
//     propObj.addProperty(intPropInfo);
//
//     auto floatPropInfo = FloatProperty("FloatProperty2", 1.1);
//     floatPropInfo.setMaxValue(EvalValue("$MaxProperty + 10"));
//     propObj.addProperty(floatPropInfo);
//
//     propObj.setPropertyValue("IntProperty2", 50);
//     ASSERT_EQ(propObj.getPropertyValue("IntProperty2"), 10);
//
//     propObj.setPropertyValue("FloatProperty2", 50.0);
//     ASSERT_EQ(propObj.getPropertyValue("FloatProperty2"), 20.0);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.Inspectable;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.Create;
//     auto obj = PropertyObject();
//
//     auto ids = obj.asPtr<IInspectable>(true).getInterfaceIds();
//     ASSERT_EQ(ids[0], IPropertyObject::Id);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.ImplementationName;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.Create();
//     auto obj = PropertyObject();
//
//     std::string className = obj.asPtr<IInspectable>(true).getRuntimeClassName();
//     auto prefix = className.find("daq::GenericPropertyObjectImpl<");
//     ASSERT_EQ(prefix, 0u);

  Assert.Fail('TODO: Convert from C++');
end;

procedure TTest_PropertyObject.SumFunctionProp;
var
  PropObj: IPropertyObjectPtr;
begin
  PropObj := TPropertyObjectPtr.Create();
//     auto propObj = PropertyObject();
//
//     auto arguments = List<IArgumentInfo>(ArgumentInfo("Val1", ctInt), ArgumentInfo("Val2", ctInt));
//     propObj.addProperty(FunctionProperty("SumFunction", FunctionInfo(ctInt, arguments)));
//
//     auto func = Function([](IntegerPtr val1, IntegerPtr val2)
//     {
//         return val1 + val2;
//     });
//     propObj.setPropertyValue("SumFunction", func);
//
//     FunctionPtr getFunc = propObj.getPropertyValue("SumFunction");
//
//     ASSERT_EQ(getFunc(10, 20), 30);

  Assert.Fail('TODO: Convert from C++');
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_PropertyObject);

end.

