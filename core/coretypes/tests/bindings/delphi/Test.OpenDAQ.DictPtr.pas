unit Test.OpenDAQ.DictPtr;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes, OpenDAQ.Iterator;

type
  ITestObject = interface(IBaseObject)
  ['{CCE11876-7994-4DD8-BE6E-493B17F02BED}']
  end;

  TDictPtrTestObject = class(TInterfacedObject, ITestObject)
  public
    constructor Create(Hash : SizeT);
    function BorrowInterface(const IID: TGUID; out Obj): HResult; stdcall;
    procedure Dispose; stdcall;

    function GetHashCodeEx(out HashCode: SizeT): ErrCode; stdcall;
    function EqualsObject(Other: IBaseObject; out Equals: Boolean): ErrCode; stdcall;
    function ToCharPtr(Str: PPAnsiChar): ErrCode; stdcall;
  private
    FHash : SizeT;
  end;

  [TestFixture]
  TTest_DictPtr = class(TDSUnitTest)
  private
    procedure IteratorMoveNext(It, EndIt: IIteratorPtr);
    procedure IteratorCheckIsEnd(It, EndIt: IIteratorPtr);
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
    procedure GetValueList;
    [Test]
    procedure GetKeys;
    [Test]
    procedure GetValues;
    [Test]
    procedure ForEachKeys;
    [Test]
    procedure ForEachValues;
    [Test]
    procedure HasKeyTrue;
    [Test]
    procedure HasKeyFalse;
    [Test]
    procedure Indexer;
    [Test]
    procedure SetWithInterface;
    [Test]
    procedure SetWithPtr;
  end;

implementation
uses
  WinApi.Windows,
  System.SysUtils,
  OpenDAQ.List,
  OpenDAQ.Dict,
  OpenDAQ.ObjectPtr,
  OpenDAQ.Integer,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.Iterable;

{ TTest_DictObject }

procedure TTest_DictPtr.Clearing();
var
  DictObj: IDictionaryPtr<ITestObject, IBaseObject>;
  KeyTestObj1, KeyTestObj2: ITestObject;
  BaseObj1, BaseObj2: IBaseObject;
begin
  DictObj := TDictionaryPtr<ITestObject, IBaseObject>.Create();

  KeyTestObj1 := TDictPtrTestObject.Create(1);
  KeyTestObj2 := TDictPtrTestObject.Create(2);

  BaseObj1 := TObjectPtr<IBaseObject>.Create();
  BaseObj2 := TObjectPtr<IBaseObject>.Create();

  DictObj.SetItem(KeyTestObj1, BaseObj1);
  DictObj.SetItem(KeyTestObj2, BaseObj2);

  DictObj.Clear();
  Assert.AreEqual<SizeT>(DictObj.GetCount(), 0);
end;

procedure TTest_DictPtr.Delete();
var
  DictObj: IDictionaryPtr<ITestObject, IBaseObject>;
  KeyTestObj: ITestObject;
  BaseObj1: IBaseObject;
begin
  DictObj := TDictionaryPtr<ITestObject, IBaseObject>.Create();
  KeyTestObj := TDictPtrTestObject.Create(1);
  BaseObj1 := TObjectPtr<IBaseObject>.Create();

  DictObj.SetItem(KeyTestObj, BaseObj1);
  DictObj.DeleteItem(KeyTestObj);

  Assert.AreEqual<SizeT>(DictObj.GetCount(), 0);
end;

procedure TTest_DictPtr.EmptyValues();
var
  DictObj: IDictionaryPtr<ITestObject, IBaseObject>;
  KeyTestObj1, KeyTestObj2, KeyTestObj3: ITestObject;
  TmpBaseObject: IBaseObject;
  BaseObject1, BaseObject2: IBaseObject;
begin
  DictObj := TDictionaryPtr<ITestObject, IBaseObject>.Create();

  KeyTestObj1 := TDictPtrTestObject.Create(1);
  KeyTestObj2 := TDictPtrTestObject.Create(2);
  KeyTestObj3 := TDictPtrTestObject.Create(3);

  TmpBaseObject := nil;
  DictObj.SetItem(KeyTestObj1, TmpBaseObject);
  DictObj.SetItem(KeyTestObj2, TmpBaseObject);
  DictObj.SetItem(KeyTestObj3, TmpBaseObject);

  BaseObject1 := DictObj.GetItem(KeyTestObj1);
  Assert.IsTrue(BaseObject1 = nil);

  DictObj.DeleteItem(KeyTestObj1);
  BaseObject2 := DictObj.RemoveItem(KeyTestObj2);

  DictObj.Clear();
end;

procedure TTest_DictPtr.EqualityTestObj();
var
  TestObject1, TestObject2: ITestObject;
  Eq: Boolean;
begin
  TestObject1 := TDictPtrTestObject.Create(1);
  TestObject2 := TDictPtrTestObject.Create(2);

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

procedure TTest_DictPtr.Getting();
var
  DictObj: IDictionaryPtr<ITestObject, IBaseObject>;
  BaseObj, BaseObj1: IBaseObject;
  KeyTestObj: ITestObject;
begin
  DictObj := TDictionaryPtr<ITestObject, IBaseObject>.Create();
  KeyTestObj := TDictPtrTestObject.Create(1);
  BaseObj := TObjectPtr<IBaseObject>.Create();

  DictObj.SetItem(KeyTestObj, Baseobj);
  BaseObj1 := DictObj.GetItem(KeyTestObj);

  Assert.AreEqual(Baseobj, BaseObj1);
end;

procedure TTest_DictPtr.HashingTestObj();
var
  TestObject1, TestObject2: ITestObject;
  HashCode1, HashCode2: SizeT;
begin
  TestObject1 := TDictPtrTestObject.Create(1);
  TestObject2 := TDictPtrTestObject.Create(2);

  TestObject1.GetHashCodeEx(HashCode1);
  Assert.AreNotEqual<SizeT>(HashCode1, 0);

  TestObject2.GetHashCodeEx(HashCode2);
  Assert.AreNotEqual<SizeT>(HashCode2, 0);
  Assert.AreNotEqual(HashCode1, HashCode2);
end;

procedure TTest_DictPtr.NotFound();
var
  DictObj: IDictionaryPtr<ITestObject, IBaseObject>;
  BaseObject: IBaseObject;
  KeyTestObj: ITestObject;
begin
  DictObj := TDictionaryPtr<ITestObject, IBaseObject>.Create();
  KeyTestObj := TDictPtrTestObject.Create(1);

  Assert.WillRaise(procedure()
    begin
      DictObj.GetItem(KeyTestObj);
    end,
    ERTNotFoundException
  );

  Assert.WillRaise(procedure()
    begin
      DictObj.DeleteItem(KeyTestObj);
    end,
    ERTNotFoundException
  );

  Assert.WillRaise(procedure()
    begin
      BaseObject := DictObj.RemoveItem(KeyTestObj);
    end,
    ERTNotFoundException
  );
end;

procedure TTest_DictPtr.Removing();
var
  DictObj: IDictionaryPtr<ITestObject, IBaseObject>;
  BaseObj1: IBaseObject;
  BaseObj2: IBaseObject;
  KeyTestObj: ITestObject;
begin
  DictObj := TDictionaryPtr<ITestObject, IBaseObject>.Create();
  KeyTestObj := TDictPtrTestObject.Create(1);
  CreateBaseObject(BaseObj1);

  DictObj.SetItem(KeyTestObj, BaseObj1);
  BaseObj2 := DictObj.RemoveItem(KeyTestObj);
  Assert.AreEqual(DictObj.GetCount(), Nativeint(0));
end;

procedure TTest_DictPtr.Setting();
var
  DictObj: IDictionaryPtr<ITestObject, IBaseObject>;
  BaseObj: IObjectPtr;
  KeyTestObj: ITestObject;
begin
  DictObj := TDictionaryPtr<ITestObject, IBaseObject>.Create();
  KeyTestObj := TDictPtrTestObject.Create(1);
  BaseObj := TObjectPtr<IBaseObject>.Create();

  DictObj.SetItem(KeyTestObj, Baseobj);

  Assert.AreEqual<SizeT>(DictObj.GetCount(), 1);
end;

procedure TTest_DictPtr.Updating();
var
  DictObj: IDictionaryPtr<ITestObject, IBaseObject>;
  BaseObj1, BaseObj2, BaseObj3, BaseObj4: IBaseObject;
//  BaseObj5 : IBaseObjectPtr;
  KeyTestObj1, KeyTestObj2: ITestObject;
  Err : ErrCode;
  Eq: Boolean;
begin
  DictObj := TDictionaryPtr<ITestObject, IBaseObject>.Create();
  KeyTestObj1 := TDictPtrTestObject.Create(1);
  KeyTestObj2 := TDictPtrTestObject.Create(2);
  BaseObj1 := TObjectPtr<IBaseObject>.Create();
  BaseObj2 := TObjectPtr<IBaseObject>.Create();

  DictObj.SetItem(KeyTestObj1, BaseObj1);

  DictObj.SetItem(KeyTestObj2, BaseObj2);

  BaseObj3 := TObjectPtr<IBaseObject>.Create();
  DictObj.SetItem(KeyTestObj2, BaseObj3);

  BaseObj4 := DictObj.GetItem(KeyTestObj2);
//  BaseObj5 := DictObj.GetItem(KeyTestObj2).AsPtr<>;

  Err := BaseObj3.EqualsObject(BaseObj4, Eq);
  Assert.AreEqual(Err, OPENDAQ_SUCCESS);
  Assert.IsTrue(Eq);

  Assert.AreEqual<SizeT>(DictObj.GetCount(), 2);
end;

procedure TTest_DictPtr.Freeze();
var
  DictObj: IDictionaryPtr<IBaseObject, IBaseObject>;
  Freezable: IFreezablePtr;
begin
  DictObj := TDictionaryPtr<IBaseObject, IBaseObject>.Create();
  Freezable := GetFreezableInterface(DictObj);

  Assert.WillNotRaiseAny(procedure()
    begin
      Freezable.Freeze();
    end
  );

  Assert.IsTrue(Freezable.IsFrozen());
end;

procedure TTest_DictPtr.SetWhenFrozen();
var
  DictObj: IDictionaryPtr<IBaseObject, IBaseObject>;
  Freezable: IFreezablePtr;
begin
  DictObj := TDictionaryPtr<IBaseObject, IBaseObject>.Create();
  Freezable := GetFreezableInterface(DictObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      DictObj.SetItem(nil, nil);
    end,
    ERTFrozenException
  );
end;

procedure TTest_DictPtr.RemoveWhenFrozen();
var
  DictObj: IDictionaryPtr<IBaseObject, IBaseObject>;
  Freezable: IFreezablePtr;
  BaseObj1: IBaseObject;
begin
  DictObj := TDictionaryPtr<IBaseObject, IBaseObject>.Create();
  Freezable := GetFreezableInterface(DictObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      BaseObj1 := DictObj.RemoveItem(nil);
    end,
    ERTFrozenException
  );
end;

procedure TTest_DictPtr.DeleteWhenFrozen();
var
  DictObj: IDictionaryPtr<IBaseObject, IBaseObject>;
  Freezable: IFreezablePtr;
begin
  DictObj := TDictionaryPtr<IBaseObject, IBaseObject>.Create();
  Freezable := GetFreezableInterface(DictObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      DictObj.DeleteItem(nil);
    end,
    ERTFrozenException
  );
end;

procedure TTest_DictPtr.ClearWhenFrozen();
var
  DictObj: IDictionaryPtr<IBaseObject, IBaseObject>;
  Freezable: IFreezablePtr;
begin
  DictObj := TDictionaryPtr<IBaseObject, IBaseObject>.Create();
  Freezable := GetFreezableInterface(DictObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      DictObj.Clear();
    end,
    ERTFrozenException
  );
end;

procedure TTest_DictPtr.IsFrozenFalse();
var
  DictObj: IDictionaryPtr<IBaseObject, IBaseObject>;
  Freezable: IFreezablePtr;
begin
  DictObj := TDictionaryPtr<IBaseObject, IBaseObject>.Create();
  Freezable := GetFreezableInterface(DictObj);

  Assert.IsFalse(Freezable.IsFrozen());
end;

procedure TTest_DictPtr.DoubleFreeze();
var
  DictObj: IDictionaryPtr<IBaseObject, IBaseObject>;
  Freezable: IFreezablePtr;
  Intf: IFreezable;
  Err: ErrCode;
begin
  DictObj := TDictionaryPtr<IBaseObject, IBaseObject>.Create();
  Freezable := GetFreezableInterface(DictObj);

  Freezable.Freeze();
  Intf := Freezable as IFreezable;

  Err := Intf.Freeze();
  Assert.AreEqual(Err, OPENDAQ_IGNORED)
end;

procedure TTest_DictPtr.CoreType();
var
  DictObj: IDictionaryPtr<IBaseObject, IBaseObject>;
  CoreType: TCoreType;
begin
  DictObj := TDictionaryPtr<IBaseObject, IBaseObject>.Create();
  CoreType := GetCoreType(DictObj);
  Assert.AreEqual(CoreType, ctDict);
end;

procedure TTest_DictPtr.GetKeyList();
var
  DictObj: IDictionaryPtr<IInteger, IString>;
  Keys: IListPtr<IInteger>;
  IntVal1, IntVal2: RtInt;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();

  DictObj.SetItem(1, 'a');
  DictObj.SetItem(2, 'b');

  Keys := DictObj.GetKeyList();

  Assert.AreEqual<SizeT>(Keys.GetCount(), 2);
  IntVal1 := Keys.GetItemAt(0);
  IntVal2 := Keys.GetItemAt(1);

  Assert.IsTrue((IntVal1 = 1) or (IntVal1 = 2));
  Assert.IsTrue((IntVal2 = 1) or (IntVal2 = 2));
  Assert.IsTrue(IntVal1 <> IntVal2);
end;

procedure TTest_DictPtr.GetValueList();
var
  DictObj: IDictionaryPtr<IInteger, IString>;
  Values: IListPtr<IString>;
  Str1, Str2 : string;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();
  DictObj.SetItem(1, 'a');
  DictObj.SetItem(2, 'b');

  Values := DictObj.GetValueList();
  Assert.AreEqual<SizeT>(Values.GetCount(), 2);

  Str1 := Values.GetItemAt(0);
  Str2 := Values.GetItemAt(1);

  Assert.IsTrue((Str1 = 'a') or (Str1 = 'b'));
  Assert.IsTrue((Str2 = 'b') or (Str2 = 'a'));
  Assert.IsTrue(Str1 <> Str2);
end;

procedure TTest_DictPtr.GetKeys();
var
  DictObj: IDictionaryPtr<IInteger, IString>;
  Keys: IIterablePtr<IInteger>;
  It, EndIt: IIteratorPtr<IInteger>;
  IntVal1, IntVal2: RtInt;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();

  DictObj.SetItem(1, 'a');
  DictObj.SetItem(2, 'b');

  Keys := DictObj.GetKeys();

  It := Keys.CreateStartIterator;
  EndIt := Keys.CreateEndIterator;

  IteratorMoveNext(It, EndIt);
  It.GetCurrent.GetValue(IntVal1);
  IteratorMoveNext(It, EndIt);
  It.GetCurrent.GetValue(IntVal2);
  IteratorCheckIsEnd(It, EndIt);

  Assert.IsTrue((IntVal1 = 1) or (IntVal1 = 2));
  Assert.IsTrue((IntVal2 = 1) or (IntVal2 = 2));
  Assert.IsTrue(IntVal1 <> IntVal2);
end;

procedure TTest_DictPtr.IteratorMoveNext(It, EndIt: IIteratorPtr);
begin
  Assert.IsTrue(It.MoveNext);
  Assert.IsFalse(It.EqualsObject(EndIt));
end;

procedure TTest_DictPtr.IteratorCheckIsEnd(It, EndIt: IIteratorPtr);
begin
  Assert.IsFalse(It.MoveNext);
  Assert.IsTrue(It.EqualsObject(EndIt));
end;

procedure TTest_DictPtr.GetValues();
var
  DictObj: IDictionaryPtr<IInteger, IString>;
  Values: IIterablePtr<IString>;
  It, EndIt: IIteratorPtr<IString>;
  Str1, Str2 : string;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();
  DictObj.SetItem(1, 'a');
  DictObj.SetItem(2, 'b');

  Values := DictObj.GetValues();

  It := Values.CreateStartIterator;
  EndIt := Values.CreateEndIterator;

  IteratorMoveNext(It, EndIt);
  Str1 := RtToString(It.GetCurrent);
  IteratorMoveNext(It, EndIt);
  Str2 := RtToString(It.GetCurrent);
  IteratorCheckIsEnd(It, EndIt);

  Assert.IsTrue((Str1 = 'a') or (Str1 = 'b'));
  Assert.IsTrue((Str2 = 'b') or (Str2 = 'a'));
  Assert.IsTrue(Str1 <> Str2);
end;

procedure TTest_DictPtr.ForEachKeys;
var
  DictObj: IDictionaryPtr<IInteger, IString>;
  Key: IInteger;
  Sum: RtInt;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();

  for Key in DictObj.GetKeys do
    Assert.Fail();

  DictObj.SetItem(1, 'a');
  DictObj.SetItem(2, 'b');
  DictObj.SetItem(3, 'c');

  Sum := 0;
  for Key in DictObj.GetKeys do
    Sum := Sum + BaseObjectToInt(Key);
  Assert.AreEqual<RtInt>(6, Sum);
end;

procedure TTest_DictPtr.ForEachValues;
var
  DictObj: IDictionaryPtr<IInteger, IString>;
  Key: IString;
  KeyStr: string;
  Sum: RtInt;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();

  for Key in DictObj.GetValues do
    Assert.Fail();

  DictObj.SetItem(1, '1');
  DictObj.SetItem(2, '2');
  DictObj.SetItem(3, '3');

  Sum := 0;
  for Key in DictObj.GetValues do
  begin
    KeyStr := BaseObjectToString(Key);
    Sum := Sum + strtoint(KeyStr);
  end;

  Assert.AreEqual<RtInt>(6, Sum);
end;

procedure TTest_DictPtr.HasKeyTrue();
var
  DictObj: IDictionaryPtr<IInteger, IString>;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();

  DictObj.SetItem(1, 'a');
  Assert.IsTrue(DictObj.HasKey(1));
end;

procedure TTest_DictPtr.HasKeyFalse();
var
  DictObj: IDictionaryPtr<IInteger, IString>;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();
  DictObj.SetItem(1, 'a');

  Assert.IsFalse(DictObj.HasKey(2));
end;

procedure TTest_DictPtr.SetWithInterface();
var
  DictObj: IDictionaryPtr<IInteger, IString>;
  IntObj : IInteger;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();

  CreateInteger(IntObj, 5);
  DictObj[IntObj] := 'd';
  Assert.AreEqual<string>(DictObj[5], 'd');
end;

procedure TTest_DictPtr.SetWithPtr();
var
  DictObj: IDictionaryPtr<IInteger, IString>;
  IntPtr : IIntegerPtr;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();

  IntPtr := TIntegerPtr.Create(5);
  DictObj[IntPtr] := 'd';
  Assert.AreEqual<string>(DictObj[5], 'd');
end;

procedure TTest_DictPtr.Indexer();
var
  DictObj: IDictionaryPtr<IInteger, IString>;
begin
  DictObj := TDictionaryPtr<IInteger, IString>.Create();
  DictObj.SetItem(1, 'a');
  DictObj.SetItem(2, 'b');

  Assert.AreEqual<string>(DictObj[1], 'a');

  DictObj[2] := 'c';
  Assert.AreEqual<string>(DictObj[2], 'c');
  Assert.AreNotEqual<string>(DictObj[2], 'b');
end;

{ TestObject }

function TDictPtrTestObject.BorrowInterface(const IID: TGUID; out Obj): HResult;
begin
  Result := 0;
end;

constructor TDictPtrTestObject.Create(Hash: SizeT);
begin
  inherited Create();
  FHash := Hash;
end;

procedure TDictPtrTestObject.Dispose();
begin
end;

function TDictPtrTestObject.EqualsObject(Other: IBaseObject; out Equals: Boolean): ErrCode;
var
  OtherHash : SizeT;
  OtherTest : ITestObject;
begin
  if not Assigned(Other) then
  begin
    Equals := False;
    Exit(OPENDAQ_SUCCESS);
  end;

  if (Other.QueryInterface(ITestObject, OtherTest) = S_OK) then
  begin
    OtherTest.GetHashCodeEx(OtherHash);
    if (OtherHash = FHash) then
      Equals := True
    else
      Equals := False;
  end
  else
    Equals := False;

  Result := OPENDAQ_SUCCESS;
end;

function TDictPtrTestObject.ToCharPtr(Str: PPAnsiChar): ErrCode;
begin
  Result := OPENDAQ_SUCCESS;
end;

function TDictPtrTestObject.GetHashCodeEx(out HashCode: SizeT): ErrCode;
begin
  HashCode := FHash;
  Result := OPENDAQ_SUCCESS;
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_DictPtr);

end.
