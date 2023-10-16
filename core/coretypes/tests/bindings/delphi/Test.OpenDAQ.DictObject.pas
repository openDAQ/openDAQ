unit Test.OpenDAQ.DictObject;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  TDictTestObject = class(TInterfacedObject, IBaseObject)
  public
    function BorrowInterface(const IID: TGUID; out Obj): HResult; stdcall;
    procedure Dispose; stdcall;

    function GetHashCodeEx(out HashCode: SizeT): ErrCode; stdcall;
    function EqualsObject(Other: IBaseObject; out Equals: Boolean): ErrCode; stdcall;
    function ToCharPtr(Str: PPAnsiChar): ErrCode; stdcall;
  end;

  [TestFixture]
  TTest_DictObject = class(TDSUnitTest)
  private
    procedure IteratorMoveNext(It, EndIt: IIterator);
    procedure IteratorCheckIsEnd(It, EndIt: IIterator);
  public
    [Test]
    procedure HashingTestObj;
    [Test]
    procedure EqualityTestObj;
    [Test]
    procedure Setting;
    [Test]
    procedure Getting;
    [Test]
    procedure Updating;
    [Test]
    procedure Clearing;
    [Test]
    procedure Removing;
    [Test]
    procedure Delete;
    [Test]
    procedure EmptyValues;
    [Test]
    procedure NotFound;
    [Test]
    procedure Freeze;
    [Test]
    procedure SetWhenFrozen;
    [Test]
    procedure RemoveWhenFrozen;
    [Test]
    procedure DeleteWhenFrozen;
    [Test]
    procedure ClearWhenFrozen;
    [Test]
    procedure IsFrozenFalse;
    [Test]
    procedure DoubleFreeze;
    [Test]
    procedure CoreType;
    [Test]
    procedure GetKeyList;
    [Test]
    procedure GetKeys;
    [Test]
    procedure GetValueList;
    [Test]
    procedure GetValues;
    [Test]
    procedure HasKeyTrue;
    [Test]
    procedure HasKeyFalse;
    [Test]
    procedure CheckIterateGuid;
    [Test]
    procedure CheckIteratorGuid;
  end;

implementation
uses
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors, SysUtils,
  OpenDAQ.Exceptions, OpenDAQ.Dict;

{ TTest_DictObject }

procedure TTest_DictObject.Clearing;
var
  DictObj: IDictObject;
  KeyTestObj1, KeyTestObj2: IBaseObject;
  BaseObj1, BaseObj2: IBaseObject;
  Count: SizeT;
begin
  CreateDict(DictObj);

  KeyTestObj1 := TDictTestObject.Create;
  KeyTestObj2 := TDictTestObject.Create;
  CreateBaseObject(BaseObj1);
  CreateBaseObject(BaseObj2);

  DictObj.SetItem(KeyTestObj1, BaseObj1);
  DictObj.SetItem(KeyTestObj2, BaseObj2);
  DictObj.Clear;

  DictObj.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(0));
end;

procedure TTest_DictObject.Delete;
var
  DictObj: IDictObject;
  KeyTestObj: IBaseObject;
  BaseObj1: IBaseObject;
  Count: SizeT;
begin
  CreateDict(DictObj);
  KeyTestObj := TDictTestObject.Create;
  CreateBaseObject(BaseObj1);

  DictObj.SetItem(KeyTestObj, BaseObj1);
  DictObj.DeleteItem(KeyTestObj);
  DictObj.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(0));
end;

procedure TTest_DictObject.EmptyValues;
var
  DictObj: IDictObject;
  KeyTestObj1, KeyTestObj2, KeyTestObj3: IBaseObject;
  TmpBaseObject: IBaseObject;
  BaseObject1, BaseObject2: IBaseObject;
begin
  CreateDict(DictObj);

  KeyTestObj1 := TDictTestObject.Create;
  KeyTestObj2 := TDictTestObject.Create;
  KeyTestObj3 := TDictTestObject.Create;

  TmpBaseObject := nil;
  DictObj.SetItem(KeyTestObj1, TmpBaseObject);
  DictObj.SetItem(KeyTestObj2, TmpBaseObject);
  DictObj.SetItem(KeyTestObj3, TmpBaseObject);

  DictObj.GetItem(KeyTestObj1, BaseObject1);
  Assert.IsTrue(BaseObject1 = nil);

  DictObj.DeleteItem(KeyTestObj1);
  DictObj.RemoveItem(KeyTestObj1, BaseObject2);

  DictObj.Clear;
end;

procedure TTest_DictObject.EqualityTestObj;
var
  TestObject1, TestObject2: IBaseObject;
  Eq: Boolean;
begin
  TestObject1 := TDictTestObject.Create;
  TestObject2 := TDictTestObject.Create;

  Eq := False;
  TestObject1.EqualsObject(TestObject1, Eq);
  Assert.IsTrue(Eq);

  TestObject1.EqualsObject(nil, Eq);
  Assert.IsFalse(Eq);

  TestObject1.EqualsObject(TestObject2, Eq);
  Assert.IsFalse(Eq);

  TestObject2.EqualsObject(TestObject1, Eq);
  Assert.IsFalse(Eq);

  TestObject2.EqualsObject(TestObject2, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_DictObject.Getting;
var
  DictObj: IDictObject;
  BaseObj, BaseObj1: IBaseObject;
  KeyTestObj: IBaseObject;
begin
  CreateDict(DictObj);
  KeyTestObj := TDictTestObject.Create;
  CreateBaseObject(Baseobj);

  DictObj.SetItem(KeyTestObj, Baseobj);
  DictObj.GetItem(KeyTestObj, BaseObj1);

  Assert.AreEqual(Baseobj, BaseObj1);
end;

procedure TTest_DictObject.HashingTestObj;
var
  TestObject1, TestObject2: IBaseObject;
  HashCode1, HashCode2: NativeInt;
begin
  TestObject1 := TDictTestObject.Create;
  TestObject2 := TDictTestObject.Create;

  TestObject1.GetHashCodeEx(HashCode1);
  Assert.AreNotEqual(HashCode1, Nativeint(0));
  TestObject2.GetHashCodeEx(HashCode2);
  Assert.AreNotEqual(HashCode2, Nativeint(0));
  Assert.AreNotEqual(HashCode1, HashCode2);
end;

procedure TTest_DictObject.NotFound;
var
  DictObj: IDictObject;
  BaseObject: IBaseObject;
  KeyTestObj: IBaseObject;
  Res: Cardinal;
begin
  CreateDict(DictObj);
  KeyTestObj := TDictTestObject.Create;

  Res := DictObj.GetItem(KeyTestObj, BaseObject);
  Assert.AreEqual(Res, OPENDAQ_ERR_NOTFOUND);

  Res := DictObj.DeleteItem(KeyTestObj);
  Assert.AreEqual(Res, OPENDAQ_ERR_NOTFOUND);

  Res := DictObj.RemoveItem(KeyTestObj, BaseObject);
  Assert.AreEqual(Res, OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_DictObject.Removing;
var
  DictObj: IDictObject;
  BaseObj1: IBaseObject;
  BaseObj2: IBaseObject;
  KeyTestObj: IBaseObject;
  Count: SizeT;
begin
  CreateDict(DictObj);
  KeyTestObj := TDictTestObject.Create;
  CreateBaseObject(BaseObj1);

  DictObj.SetItem(KeyTestObj, BaseObj1);
  DictObj.RemoveItem(KeyTestObj, BaseObj2);
  DictObj.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(0));
end;

procedure TTest_DictObject.Setting;
var
  DictObj: IDictObject;
  BaseObj: IBaseObject;
  KeyTestObj: IBaseObject;
  Count: SizeT;
begin
  CreateDict(DictObj);
  KeyTestObj := TDictTestObject.Create;
  CreateBaseObject(BaseObj);
  DictObj.SetItem(KeyTestObj, Baseobj);
  DictObj.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(1));
end;

procedure TTest_DictObject.Updating;
var
  DictObj: IDictObject;
  BaseObj1, BaseObj2, BaseObj3, BaseObj4: IBaseObject;
  KeyTestObj1, KeyTestObj2: IBaseObject;
  Count: SizeT;
  Eq: Boolean;
begin
  CreateDict(DictObj);
  KeyTestObj1 := TDictTestObject.Create;
  KeyTestObj2 := TDictTestObject.Create;
  CreateBaseObject(BaseObj1);
  CreateBaseObject(BaseObj2);

  DictObj.SetItem(KeyTestObj1, BaseObj1);
  DictObj.SetItem(KeyTestObj2, BaseObj2);

  CreateBaseObject(BaseObj3);
  DictObj.SetItem(KeyTestObj2, BaseObj3);
  DictObj.GetItem(KeyTestObj2, BaseObj4);

  Eq := False;
  BaseObj3.EqualsObject(BaseObj4, Eq);
  Assert.IsTrue(Eq);

  DictObj.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(2));
end;

procedure TTest_DictObject.Freeze;
var
  DictObj: IDictObject;
  Freezable: IFreezable;
  Frozen: Boolean;
begin
  CreateDict(DictObj);
  Freezable := GetFreezableInterface(DictObj);
  Assert.AreEqual(Freezable.Freeze(), OPENDAQ_SUCCESS);
  Assert.AreEqual(Freezable.IsFrozen(Frozen), OPENDAQ_SUCCESS);
  Assert.IsTrue(Frozen);
end;

procedure TTest_DictObject.SetWhenFrozen;
var
  DictObj: IDictObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateDict(DictObj);
  Freezable := GetFreezableInterface(DictObj);
  Freezable.Freeze();

  Res := DictObj.SetItem(nil, nil);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_DictObject.RemoveWhenFrozen;
var
  DictObj: IDictObject;
  Freezable: IFreezable;
  Res: ErrCode;
  BaseObj1: IBaseObject;
begin
  CreateDict(DictObj);
  Freezable := GetFreezableInterface(DictObj);
  Freezable.Freeze();

  CreateBaseObject(BaseObj1);
  Res := DictObj.RemoveItem(nil, BaseObj1);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_DictObject.DeleteWhenFrozen;
var
  DictObj: IDictObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateDict(DictObj);
  Freezable := GetFreezableInterface(DictObj);
  Freezable.Freeze();

  Res := DictObj.DeleteItem(nil);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_DictObject.ClearWhenFrozen;
var
  DictObj: IDictObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateDict(DictObj);
  Freezable := GetFreezableInterface(DictObj);
  Freezable.Freeze();

  Res := DictObj.Clear;
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_DictObject.IsFrozenFalse;
var
  DictObj: IDictObject;
  Freezable: IFreezable;
  Frozen: Boolean;
begin
  CreateDict(DictObj);
  Freezable := GetFreezableInterface(DictObj);
  Assert.AreEqual(Freezable.IsFrozen(Frozen), OPENDAQ_SUCCESS);
  Assert.IsFalse(Frozen);
end;

procedure TTest_DictObject.DoubleFreeze;
var
  DictObj: IDictObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateDict(DictObj);
  Freezable := GetFreezableInterface(DictObj);
  Freezable.Freeze();
  Res := Freezable.Freeze();
  Assert.AreEqual(Res, OPENDAQ_IGNORED);
end;

procedure TTest_DictObject.CoreType;
var
  DictObj: IDictObject;
  CoreType: TCoreType;
begin
  CreateDict(DictObj);
  CoreType := GetCoreType(DictObj);
  Assert.AreEqual(CoreType, ctDict);
end;

procedure TTest_DictObject.GetKeyList;
var
  DictObj: IDictObject;
  IntObj1, IntObj2: IInteger;
  StringObj1, StringObj2: IString;
  Keys: IListObject;
  Count: SizeT;
  IntObj3, IntObj4: IInteger;
  IntVal1, IntVal2: RtInt;
begin
  CreateDict(DictObj);
  CreateInteger(IntObj1, 1);
  CreateInteger(IntObj2, 2);
  CreateString(StringObj1, 'a');
  CreateString(StringObj2, 'b');
  DictObj.SetItem(IntObj1, StringObj1);
  DictObj.SetItem(IntObj2, StringObj2);

  CreateList(Keys);
  DictObj.GetKeyList(Keys);

  Keys.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(2));
  Keys.GetItemAt(0, IBaseObject(IntObj3));
  Keys.GetItemAt(1, IBaseObject(IntObj4));

  IntObj3.GetValue(IntVal1);
  IntObj4.GetValue(IntVal2);
  Assert.IsTrue((IntVal1 = 1) or (IntVal1 = 2));
  Assert.IsTrue((IntVal2 = 1) or (IntVal2 = 2));
  Assert.IsTrue(IntVal1 <> IntVal2);
end;

procedure TTest_DictObject.GetKeys;
var
  DictObj: IDictObject;
  IntObj1, IntObj2: IInteger;
  StringObj1, StringObj2: IString;
  Keys: IIterable;
  It, EndIt: IIterator;
  IntObj3, IntObj4: IInteger;
  IntVal1, IntVal2: RtInt;
begin
  CreateDict(DictObj);
  CreateInteger(IntObj1, 1);
  CreateInteger(IntObj2, 2);
  CreateString(StringObj1, 'a');
  CreateString(StringObj2, 'b');
  DictObj.SetItem(IntObj1, StringObj1);
  DictObj.SetItem(IntObj2, StringObj2);

  //CreateList(Keys);
  DictObj.GetKeys(Keys);

  Keys.CreateStartIterator(It);
  Keys.CreateEndIterator(EndIt);

  IteratorMoveNext(It, EndIt);
  It.GetCurrent(IBaseObject(IntObj3));

  IteratorMoveNext(It, EndIt);
  It.GetCurrent(IBaseObject(IntObj4));

  IteratorCheckIsEnd(It, EndIt);

  IntObj3.GetValue(IntVal1);
  IntObj4.GetValue(IntVal2);
  Assert.IsTrue((IntVal1 = 1) or (IntVal1 = 2));
  Assert.IsTrue((IntVal2 = 1) or (IntVal2 = 2));
  Assert.IsTrue(IntVal1 <> IntVal2);
end;

procedure TTest_DictObject.IteratorMoveNext(It, EndIt: IIterator);
var
  Eq: Boolean;
begin
  Assert.AreEqual(It.MoveNext, OPENDAQ_SUCCESS);

  It.EqualsObject(EndIt, Eq);
  Assert.IsFalse(Eq);
end;

procedure TTest_DictObject.IteratorCheckIsEnd(It, EndIt: IIterator);
var
 Eq: Boolean;
begin
  Assert.AreEqual(It.MoveNext, OPENDAQ_NO_MORE_ITEMS);
  Assert.IsTrue(It.EqualsObject(EndIt, Eq) = OPENDAQ_SUCCESS);
end;

procedure TTest_DictObject.GetValueList;
var
  DictObj: IDictObject;
  IntObj1, IntObj2: IInteger;
  StringObj1, StringObj2: IString;
  Values: IListObject;
  Count: SizeT;
  StringObj3, StringObj4: IInteger;
  Str1, Str2: PAnsiChar;
begin
  CreateDict(DictObj);
  CreateInteger(IntObj1, 1);
  CreateInteger(IntObj2, 2);
  CreateString(StringObj1, 'a');
  CreateString(StringObj2, 'b');
  DictObj.SetItem(IntObj1, StringObj1);
  DictObj.SetItem(IntObj2, StringObj2);

  CreateList(Values);
  DictObj.GetValueList(Values);

  Values.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(2));
  Values.GetItemAt(0, IBaseObject(StringObj3));
  Values.GetItemAt(1, IBaseObject(StringObj4));

  StringObj3.ToCharPtr(@Str1);
  StringObj4.ToCharPtr(@Str2);

  Assert.IsTrue((string(Str1) = 'a') or (string(Str1) = 'b'));
  Assert.IsTrue((string(Str2) = 'b') or (string(Str2) = 'a'));
  Assert.IsTrue(Str1 <> Str2);

  DaqFreeMemory(Str1);
  DaqFreeMemory(Str2);
end;

procedure TTest_DictObject.GetValues;
var
  DictObj: IDictObject;
  IntObj1, IntObj2: IInteger;
  StringObj1, StringObj2: IString;
  Values: IIterable;
  It, EndIt: IIterator;
  StringObj3, StringObj4: IInteger;
  Str1, Str2: PAnsiChar;
begin
  CreateDict(DictObj);
  CreateInteger(IntObj1, 1);
  CreateInteger(IntObj2, 2);
  CreateString(StringObj1, 'a');
  CreateString(StringObj2, 'b');
  DictObj.SetItem(IntObj1, StringObj1);
  DictObj.SetItem(IntObj2, StringObj2);

  DictObj.GetValues(Values);

  Values.CreateStartIterator(It);
  Values.CreateEndIterator(EndIt);

  IteratorMoveNext(It, EndIt);
  It.GetCurrent(IBaseObject(StringObj3));

  IteratorMoveNext(It, EndIt);
  It.GetCurrent(IBaseObject(StringObj4));

  IteratorCheckIsEnd(It, EndIt);

  StringObj3.ToCharPtr(@Str1);
  StringObj4.ToCharPtr(@Str2);

  Assert.IsTrue((string(Str1) = 'a') or (string(Str1) = 'b'));
  Assert.IsTrue((string(Str2) = 'b') or (string(Str2) = 'a'));
  Assert.IsTrue(Str1 <> Str2);

  DaqFreeMemory(Str1);
  DaqFreeMemory(Str2);
end;

procedure TTest_DictObject.HasKeyTrue;
var
  DictObj: IDictObject;
  IntObj1, IntObj2: IInteger;
  StringObj1: IString;
  Bool: Boolean;
begin
  CreateDict(DictObj);
  CreateInteger(IntObj1, 1);
  CreateString(StringObj1, 'a');
  DictObj.SetItem(IntObj1, StringObj1);

  CreateInteger(IntObj2, 1);
  DictObj.HasKey(IntObj2, Bool);
  Assert.IsTrue(Bool);
end;

procedure TTest_DictObject.HasKeyFalse;
var
  DictObj: IDictObject;
  IntObj1, IntObj2: IInteger;
  StringObj1: IString;
  Bool: Boolean;
begin
  CreateDict(DictObj);
  CreateInteger(IntObj1, 1);
  CreateString(StringObj1, 'a');
  DictObj.SetItem(IntObj1, StringObj1);

  CreateInteger(IntObj2, 2);
  DictObj.HasKey(IntObj2, Bool);
  Assert.IsFalse(Bool);
end;

procedure TTest_DictObject.CheckIterateGuid;
var
  DictObj: IDictObject;
  Iterable: IIterable;
  BaseObjectIterable: IBaseObject;
begin
  CreateDict(DictObj);

  DictObj.GetValues(IIterable(BaseObjectIterable));

  Iterable := BaseObjectIterable as IIterable;
  Assert.IsNotNull(Iterable);
end;

procedure TTest_DictObject.CheckIteratorGuid;
var
  DictObj: IDictObject;
  Iterable: IIterable;
  Iterator: IIterator;
  BaseObjectIterator: IBaseObject;
begin
  CreateDict(DictObj);

  DictObj.GetValues(Iterable);

  Iterable.CreateStartIterator(IIterator(BaseObjectIterator));

  Iterator := BaseObjectIterator as IIterator;
  Assert.IsNotNull(Iterator);
end;

{ TestObject }

function TDictTestObject.BorrowInterface(const IID: TGUID; out Obj): HResult;
begin
  Result := 0;
end;

procedure TDictTestObject.Dispose;
begin
end;

function TDictTestObject.EqualsObject(Other: IBaseObject; out Equals: Boolean): ErrCode;
begin
  if Other = Self as IBaseObject then
    Equals := True
  else
    Equals := False;

  Result := OPENDAQ_SUCCESS;
end;

function TDictTestObject.ToCharPtr(Str: PPAnsiChar): ErrCode;
begin
  Result := OPENDAQ_SUCCESS;
end;

function TDictTestObject.GetHashCodeEx(out HashCode: SizeT): ErrCode;
begin
  HashCode := SizeT(Self);
  Result := OPENDAQ_SUCCESS;
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_DictObject);

end.
