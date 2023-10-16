unit Test.OpenDAQ.ListObject;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  TListTestObject = class(TInterfacedObject, IBaseObject)
  public
    function BorrowInterface(const IID: TGUID; out Obj): HResult; stdcall;
    procedure Dispose; stdcall;

    function GetHashCodeEx(out HashCode: SizeT): ErrCode; stdcall;
    function EqualsObject(Other: IBaseObject; out Equals: Boolean): ErrCode; stdcall;
    function ToCharPtr(Str: PPAnsiChar): ErrCode; stdcall;
  end;

  [TestFixture]
  TTest_ListObject = class(TDSUnitTest)
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure Pushing;
    [Test]
    procedure PushingFront;
    [Test]
    procedure Popping;
    [Test]
    procedure PoppingFront;
    [Test]
    procedure SetItem;
    [Test]
    procedure DeleteRemove;
    [Test]
    procedure Clear;
    [Test]
    procedure Insert;
    [Test]
    procedure OutOfRange;
    [Test]
    procedure CoreType;
    [Test]
    procedure ToString; reintroduce;
    [Test]
    procedure Freeze;
    [Test]
    procedure SetItemAtWhenFrozen;
    [Test]
    procedure PushBackWhenFrozen;
    [Test]
    procedure PushFrontWhenFrozen;
    [Test]
    procedure PopBackWhenFrozen;
    [Test]
    procedure PopFrontWhenFrozen;
    [Test]
    procedure InsertAtWhenFrozen;
    [Test]
    procedure RemoveAtWhenFrozen;
    [Test]
    procedure DeleteAtWhenFrozen;
    [Test]
    procedure ClearWhenFrozen;
    [Test]
    procedure IsFrozenFalse;
    [Test]
    procedure DoubleFreeze;
    [Test]
    procedure SerializeId;
  end;

implementation
uses
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions, OpenDAQ.List;

{ TTest_BaseObject }

procedure TTest_ListObject.Clear;
var
  ListObject: IListObject;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  Count: SizeT;
begin
  CreateList(ListObject);
  TestObj1 := TListTestObject.Create;
  TestObj2 := TListTestObject.Create;
  TestObj3 := TListTestObject.Create;

  ListObject.PushBack(TestObj1);
  ListObject.PushBack(TestObj2);
  ListObject.PushBack(TestObj3);

  ListObject.Clear;

  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(0));
end;

procedure TTest_ListObject.DeleteRemove;
var
  ListObject: IListObject;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObject: IBaseObject;
  Count: SizeT;
  Eq: Boolean;
begin
  CreateList(ListObject);
  TestObj1 := TListTestObject.Create;
  TestObj2 := TListTestObject.Create;
  TestObj3 := TListTestObject.Create;

  ListObject.PushBack(TestObj1);
  ListObject.PushBack(TestObj2);
  ListObject.PushBack(TestObj3);
  ListObject.PushBack(nil);
  ListObject.PushBack(nil);

  ListObject.RemoveAt(1, TestObject);
  ListObject.DeleteAt(1);

  Eq := False;
  TestObj2.EqualsObject(TestObject, Eq);
  Assert.IsTrue(Eq);

  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(3));
  ListObject.DeleteAt(2);
  ListObject.RemoveAt(1, TestObject);
  Assert.IsTrue(TestObject = nil);
end;

procedure TTest_ListObject.Insert;
var
  ListObject: IListObject;
  TestObj1, TestObj2, TestObj3, TestObj4: IBaseObject;
  TestObject: IBaseObject;
  Count: SizeT;
  Eq: Boolean;
begin
  CreateList(ListObject);

  TestObj1 := TListTestObject.Create;
  ListObject.PushBack(TestObj1);
  TestObj2 := TListTestObject.Create;
  ListObject.PushBack(TestObj2);
  TestObj3 := TListTestObject.Create;
  ListObject.PushBack(TestObj3);

  TestObj4 := TListTestObject.Create;
  ListObject.InsertAt(1, TestObj4);
  ListObject.GetItemAt(1, TestObject);

  Eq := False;
  TestObj4.EqualsObject(TestObject, Eq);
  Assert.IsTrue(Eq);

  ListObject.InsertAt(3, nil);
  ListObject.GetItemAt(3, TestObject);
  Assert.IsTrue(TestObject = nil);

  Assert.AreEqual(ListObject.GetCount(Count), OPENDAQ_SUCCESS);
  Assert.AreEqual(Count, Nativeint(5));
end;

procedure TTest_ListObject.OutOfRange;
var
  ListObject: IListObject;
  TestObject: IBaseObject;
  Res: Cardinal;
begin
  CreateList(ListObject);

  Res := ListObject.DeleteAt(0);
  Assert.IsTrue(Res = OPENDAQ_ERR_OUTOFRANGE);
  Res := ListObject.RemoveAt(0, TestObject);
  Assert.IsTrue(Res = OPENDAQ_ERR_OUTOFRANGE);
  Res := ListObject.InsertAt(0, TestObject);
  Assert.IsTrue(Res = OPENDAQ_ERR_OUTOFRANGE);
  Res := ListObject.GetItemAt(0, TestObject);
  Assert.IsTrue(Res = OPENDAQ_ERR_OUTOFRANGE);
  Res := ListObject.SetItemAt(0, TestObject);
  Assert.IsTrue(Res = OPENDAQ_ERR_OUTOFRANGE);
  Res := ListObject.PopFront(TestObject);
  Assert.IsTrue(Res = OPENDAQ_ERR_NOTFOUND);
  Res := ListObject.PopBack(TestObject);
  Assert.IsTrue(Res = OPENDAQ_ERR_NOTFOUND);
end;

procedure TTest_ListObject.Popping;
var
  ListObject: IListObject;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObject: IBaseObject;
  Count: SizeT;
  Eq: Boolean;
begin
  CreateList(ListObject);
  TestObj1 := TListTestObject.Create;
  TestObj2 := TListTestObject.Create;
  TestObj3 := TListTestObject.Create;

  ListObject.PushBack(TestObj1);
  ListObject.PushBack(TestObj2);
  ListObject.PushBack(TestObj3);

  ListObject.PopBack(TestObject);
  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(2));

  Eq := False;
  TestObj3.EqualsObject(TestObject, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_ListObject.PoppingFront;
var
  ListObject: IListObject;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObject: IBaseObject;
  Count: SizeT;
  Eq: Boolean;
begin
  CreateList(ListObject);
  TestObj1 := TListTestObject.Create;
  TestObj2 := TListTestObject.Create;
  TestObj3 := TListTestObject.Create;

  ListObject.PushFront(TestObj1);
  ListObject.PushFront(TestObj2);
  ListObject.PushFront(TestObj3);

  ListObject.PopFront(TestObject);
  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(2));

  Eq := False;
  TestObj3.EqualsObject(TestObject, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_ListObject.Pushing;
var
  ListObject: IListObject;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObj: IBaseObject;
  Count: SizeT;
  Eq: Boolean;
begin
  CreateList(ListObject);
  TestObj1 := TListTestObject.Create;
  TestObj2 := TListTestObject.Create;
  TestObj3 := TListTestObject.Create;

  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(0));
  ListObject.PushBack(TestObj1);
  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(1));
  ListObject.PushBack(TestObj2);
  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(2));
  ListObject.PushBack(TestObj3);
  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(3));

  ListObject.GetItemAt(2, TestObj);

  Eq := False;
  TestObj3.EqualsObject(TestObj, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_ListObject.PushingFront;
var
  ListObject: IListObject;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObj: IBaseObject;
  Count: SizeT;
  Eq: Boolean;
begin
  CreateList(ListObject);
  TestObj1 := TListTestObject.Create;
  TestObj2 := TListTestObject.Create;
  TestObj3 := TListTestObject.Create;

  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(0));
  ListObject.PushFront(TestObj1);
  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(1));
  ListObject.PushFront(TestObj2);
  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(2));
  ListObject.PushFront(TestObj3);
  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(3));

  ListObject.GetItemAt(0, TestObj);

  Eq := False;
  TestObj3.EqualsObject(TestObj, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_ListObject.SetItem;
var
  ListObject: IListObject;
  TestObj1, TestObj2, TestObj3, TestObj4: IBaseObject;
  BaseObject: IBaseObject;
  Count: SizeT;
begin
  CreateList(ListObject);
  TestObj1 := TListTestObject.Create;
  TestObj2 := TListTestObject.Create;
  TestObj3 := TListTestObject.Create;

  ListObject.PushBack(TestObj1);
  ListObject.PushBack(TestObj2);
  ListObject.PushBack(TestObj3);
  ListObject.PushBack(nil);
  ListObject.PushBack(nil);

  TestObj4 := TListTestObject.Create;

  ListObject.SetItemAt(1, TestObj4);
  ListObject.SetItemAt(2, nil);
  ListObject.SetItemAt(3, TestObj4);
  ListObject.SetItemAt(4, nil);

  ListObject.GetCount(Count);
  Assert.AreEqual(Count, Nativeint(5));

  ListObject.GetItemAt(2, BaseObject);
  Assert.IsTrue(BaseObject = nil);
end;

procedure TTest_ListObject.CoreType;
var
  ListObj: IListObject;
  CoreType: TCoreType;
begin
  CreateList(ListObj);
  CoreType := GetCoreType(ListObj);
  Assert.AreEqual(CoreType, ctList);
end;

procedure TTest_ListObject.ToString;
var
  ListObj: IListObject;
  BaseObject1, BaseObject2: IBaseObject;
  Str: PAnsiChar;
begin
  CreateList(ListObj);
  CreateBaseObject(BaseObject1);
  CreateBaseObject(BaseObject2);
  ListObj.PushBack(BaseObject1);
  ListObj.PushBack(BaseObject2);
  ListObj.ToCharPtr(@Str);

  Assert.AreEqual(string(Str), '[ daq::IInspectable, daq::IInspectable ]');

  DaqFreeMemory(Str);
end;

procedure TTest_ListObject.Freeze;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Frozen: Boolean;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Assert.AreEqual(Freezable.Freeze(), OPENDAQ_SUCCESS);
  Assert.AreEqual(Freezable.IsFrozen(Frozen), OPENDAQ_SUCCESS);
  Assert.IsTrue(Frozen);
end;

procedure TTest_ListObject.SetItemAtWhenFrozen;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Res := ListObj.SetItemAt(0, nil);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_ListObject.PushBackWhenFrozen;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Res := ListObj.PushBack(nil);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_ListObject.PushFrontWhenFrozen;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Res := ListObj.PushFront(nil);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_ListObject.PopBackWhenFrozen;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
  BaseObject: IBaseObject;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Res := ListObj.PopBack(BaseObject);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_ListObject.PopFrontWhenFrozen;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
  BaseObject: IBaseObject;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Res := ListObj.popFront(BaseObject);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_ListObject.InsertAtWhenFrozen;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
  BaseObject: IBaseObject;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  CreateBaseObject(BaseObject);
  Res := ListObj.InsertAt(0, nil);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_ListObject.RemoveAtWhenFrozen;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
  BaseObject: IBaseObject;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Res := ListObj.RemoveAt(0, BaseObject);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_ListObject.DeleteAtWhenFrozen;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Res := ListObj.DeleteAt(0);
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_ListObject.ClearWhenFrozen;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Res := ListObj.Clear;
  Assert.AreEqual(Res, OPENDAQ_ERR_FROZEN);
end;

procedure TTest_ListObject.IsFrozenFalse;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Frozen: Boolean;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Assert.AreEqual(Freezable.IsFrozen(Frozen), OPENDAQ_SUCCESS);
  Assert.IsFalse(Frozen);
end;

procedure TTest_ListObject.DoubleFreeze;
var
  ListObj: IListObject;
  Freezable: IFreezable;
  Res: ErrCode;
begin
  CreateList(ListObj);
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();
  Res := Freezable.Freeze();
  Assert.AreEqual(Res, OPENDAQ_IGNORED);
end;

procedure TTest_ListObject.SerializeId;
var
  ListObj: IListObject;
  Serializable: ISerializable;
  Id: PAnsiChar;
  Res: ErrCode;
begin
  CreateList(ListObj);
  Serializable := GetSerializableInterface(ListObj);
  Res := Serializable.GetSerializeId(@Id);
  Assert.AreEqual(Res, OPENDAQ_ERR_NOTIMPLEMENTED);
end;

procedure TTest_ListObject.Setup;
begin

end;

procedure TTest_ListObject.TearDown;
begin

end;

{ TTestObject }

function TListTestObject.BorrowInterface(const IID: TGUID; out Obj): HResult;
begin
  Result := 0;
end;

procedure TListTestObject.Dispose;
begin
end;

function TListTestObject.EqualsObject(Other: IBaseObject; out Equals: Boolean): ErrCode;
begin
  if Other = Self as IBaseObject then
    Equals := True
  else
    Equals := False;

  Result := OPENDAQ_SUCCESS;
end;

function TListTestObject.ToCharPtr(Str: PPAnsiChar): ErrCode;
begin
  Result := OPENDAQ_SUCCESS;
end;

function TListTestObject.GetHashCodeEx(out HashCode: SizeT): ErrCode;
begin
  HashCode := SizeT(Self);
  Result := OPENDAQ_SUCCESS;
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_ListObject);

end.
