unit Test.OpenDAQ.ListPtr;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  TListPtrTestObject = class(TInterfacedObject, IBaseObject)
  public
    function BorrowInterface(const IID: TGUID; out Obj): HResult; stdcall;
    procedure Dispose; stdcall;

    function GetHashCodeEx(out HashCode: SizeT): ErrCode; stdcall;
    function EqualsObject(Other: IBaseObject; out Equals: Boolean): ErrCode; stdcall;
    function ToCharPtr(Str: PPAnsiChar): ErrCode; stdcall;
  end;

  [TestFixture]
  TTest_ListPtr = class(TDSUnitTest)
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
    procedure ForEach;
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
    [Test]
    procedure Indexer;
    [Test]
    procedure CastPtr;
  end;

implementation
uses
  WinApi.Windows,
  OpenDAQ.List,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.Freezable,
  OpenDAQ.Serializable;

{ TTest_BaseObject }

procedure TTest_ListPtr.CastPtr;
var
  List: IObjectPtr<IFloat>;
  List2: IListPtr<IBoolean>;
  List3: IListObject;
  Count: SizeT;
begin
  List := TListPtr<IFloat>.Create() as IObjectPtr<IFloat>;
  List2 := List as IListPtr<IBoolean>;
  List3 := List as IListObject;

  List3.GetCount(Count);

  Count := 5;
end;

procedure TTest_ListPtr.Clear();
var
  ListPtr: IListPtr<IBaseObject>;
  TestObj1, TestObj2, TestObj3: IBaseObject;
begin
  ListPtr := TListPtr<IBaseObject>.Create();
  TestObj1 := TListPtrTestObject.Create();
  TestObj2 := TListPtrTestObject.Create();
  TestObj3 := TListPtrTestObject.Create();

  ListPtr.PushBack(TestObj1);
  ListPtr.PushBack(TestObj2);
  ListPtr.PushBack(TestObj3);

  ListPtr.Clear();
  Assert.AreEqual<SizeT>(ListPtr.GetCount(), 0);
end;

procedure TTest_ListPtr.DeleteRemove();
var
  ListObject: IListPtr<IBaseObject>;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObject: IBaseObject;
  Eq: Boolean;
begin
  ListObject := TListPtr<IBaseObject>.Create();
  TestObj1 := TListPtrTestObject.Create();
  TestObj2 := TListPtrTestObject.Create();
  TestObj3 := TListPtrTestObject.Create();

  ListObject.PushBack(TestObj1);
  ListObject.PushBack(TestObj2);
  ListObject.PushBack(TestObj3);
  ListObject.PushBack(nil);
  ListObject.PushBack(nil);

  TestObject := ListObject.RemoveAt(1);
  ListObject.DeleteAt(1);

  Eq := False;
  TestObj2.EqualsObject(TestObject, Eq);
  Assert.IsTrue(Eq);

  Assert.AreEqual<SizeT>(ListObject.GetCount(), 3);

  ListObject.DeleteAt(2);
  TestObject := ListObject.RemoveAt(1);
  Assert.IsTrue(TestObject = nil);
end;

procedure TTest_ListPtr.Indexer();
var
  ListObject: IListPtr<IBaseObject>;
  TestObj1, TestObj2, TestObj3: IBaseObject;
begin
  ListObject := TListPtr<IBaseObject>.Create();
  TestObj1 := TListPtrTestObject.Create();
  TestObj2 := TListPtrTestObject.Create();
  TestObj3 := TListPtrTestObject.Create();

  ListObject.PushBack(TestObj1);
  ListObject.PushBack(TestObj2);
  ListObject.PushBack(TestObj3);

  Assert.AreEqual<IBaseObject>(ListObject[0], TestObj1);

  ListObject[2] := TestObj1;
  Assert.AreEqual<IBaseObject>(ListObject[2], TestObj1);
  Assert.AreNotEqual<IBaseObject>(ListObject[2], TestObj3);
end;

procedure TTest_ListPtr.Insert();
var
  ListObject: IListPtr<IBaseObject>;
  TestObj1, TestObj2, TestObj3, TestObj4: IBaseObject;
  TestObject: IBaseObject;
  Eq: Boolean;
begin
  ListObject := TListPtr<IBaseObject>.Create();

  TestObj1 := TListPtrTestObject.Create();
  ListObject.PushBack(TestObj1);

  TestObj2 := TListPtrTestObject.Create();
  ListObject.PushBack(TestObj2);

  TestObj3 := TListPtrTestObject.Create();
  ListObject.PushBack(TestObj3);

  TestObj4 := TListPtrTestObject.Create();
  ListObject.InsertAt(1, TestObj4);

  TestObject := ListObject.GetItemAt(1);

  Eq := False;
  TestObj4.EqualsObject(TestObject, Eq);
  Assert.IsTrue(Eq);

  ListObject.InsertAt(3, nil);
  TestObject := ListObject.GetItemAt(3);
  Assert.IsTrue(TestObject = nil);

  Assert.AreEqual<SizeT>(ListObject.GetCount(), 5);
end;

procedure TTest_ListPtr.OutOfRange();
var
  ListPtr: IListPtr<IBaseObject>;
  TestObject: IBaseObject;
begin
  ListPtr := TListPtr<IBaseObject>.Create();

  Assert.WillRaise(procedure()
    begin
      ListPtr.DeleteAt(0);
    end,
    ERTOutOfRangeException
  );

  Assert.WillRaise(procedure()
    begin
      ListPtr.RemoveAt(0);
    end,
    ERTOutOfRangeException
  );

  Assert.WillRaise(procedure()
    begin
      ListPtr.InsertAt(0, TestObject);
    end,
    ERTOutOfRangeException
  );

  Assert.WillRaise(procedure()
    begin
      TestObject := ListPtr.GetItemAt(0);
    end,
    ERTOutOfRangeException
  );

  Assert.WillRaise(procedure()
    begin
      ListPtr.SetItemAt(0, TestObject);
    end,
    ERTOutOfRangeException
  );

  Assert.WillRaise(procedure()
    begin
      TestObject := ListPtr.PopFront();
    end,
    ERTNotFoundException
  );

  Assert.WillRaise(procedure()
    begin
      TestObject := ListPtr.PopBack();
    end,
    ERTNotFoundException
  );
end;

procedure TTest_ListPtr.ForEach;
var
  ListObj: IListPtr<IInteger>;
  Value: IInteger;
  Sum: RtInt;
begin
  ListObj := TListPtr<IInteger>.Create();

  for Value in ListObj do
    Assert.Fail();

  ListObj.PushBack(1);
  ListObj.PushBack(2);
  ListObj.PushBack(3);

  Sum := 0;
  for Value in ListObj do
    Sum := Sum + BaseObjectToInt(Value);

  Assert.AreEqual<RtInt>(6, Sum);
end;

procedure TTest_ListPtr.Popping();
var
  ListObject: IListPtr<IBaseObject>;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObject: IBaseObject;
  Eq: Boolean;
begin
  ListObject := TListPtr<IBaseObject>.Create();
  TestObj1 := TListPtrTestObject.Create();
  TestObj2 := TListPtrTestObject.Create();
  TestObj3 := TListPtrTestObject.Create();

  ListObject.PushBack(TestObj1);
  ListObject.PushBack(TestObj2);
  ListObject.PushBack(TestObj3);

  TestObject := ListObject.PopBack();

  Assert.AreEqual<SizeT>(ListObject.GetCount(), 2);

  Eq := False;
  TestObj3.EqualsObject(TestObject, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_ListPtr.PoppingFront();
var
  ListObject: IListPtr<IBaseObject>;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObject: IBaseObject;
  Eq: Boolean;
begin
  ListObject := TListPtr<IBaseObject>.Create();
  TestObj1 := TListPtrTestObject.Create();
  TestObj2 := TListPtrTestObject.Create();
  TestObj3 := TListPtrTestObject.Create();

  ListObject.PushFront(TestObj1);
  ListObject.PushFront(TestObj2);
  ListObject.PushFront(TestObj3);

  TestObject := ListObject.PopFront();

  Assert.AreEqual<SizeT>(ListObject.GetCount(), 2);

  Eq := False;
  TestObj3.EqualsObject(TestObject, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_ListPtr.Pushing();
var
  ListObject: IListPtr<IBaseObject>;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObj: IBaseObject;
  Eq: Boolean;
begin
  ListObject := TListPtr<IBaseObject>.Create();
  TestObj1 := TListPtrTestObject.Create();
  TestObj2 := TListPtrTestObject.Create();
  TestObj3 := TListPtrTestObject.Create();

  Assert.AreEqual<SizeT>(ListObject.GetCount(), 0);

  ListObject.PushBack(TestObj1);
  Assert.AreEqual<SizeT>(ListObject.GetCount(), 1);

  ListObject.PushBack(TestObj2);
  Assert.AreEqual<SizeT>(ListObject.GetCount(), 2);

  ListObject.PushBack(TestObj3);
  Assert.AreEqual<SizeT>(ListObject.GetCount(), 3);

  TestObj := ListObject.GetItemAt(2);

  Eq := False;
  TestObj3.EqualsObject(TestObj, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_ListPtr.PushingFront();
var
  ListObject: IListPtr;
  TestObj1, TestObj2, TestObj3: IBaseObject;
  TestObj: IBaseObject;
  Count: SizeT;
  Eq: Boolean;
begin
  ListObject := TListPtr<IBaseObject>.Create();
  TestObj1 := TListPtrTestObject.Create();
  TestObj2 := TListPtrTestObject.Create();
  TestObj3 := TListPtrTestObject.Create();

  Count := ListObject.GetCount();
  Assert.AreEqual<SizeT>(Count, 0);

  ListObject.PushFront(TestObj1);
  Count := ListObject.GetCount();
  Assert.AreEqual<SizeT>(Count, 1);

  ListObject.PushFront(TestObj2);
  Count := ListObject.GetCount();
  Assert.AreEqual<SizeT>(Count, 2);

  ListObject.PushFront(TestObj3);
  Count := ListObject.GetCount();
  Assert.AreEqual<SizeT>(Count, 3);

  TestObj := ListObject.GetItemAt(0);

  Eq := False;
  TestObj3.EqualsObject(TestObj, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_ListPtr.SetItem();
var
  ListObject: IListPtr<IBaseObject>;
  TestObj1, TestObj2, TestObj3, TestObj4: IBaseObject;
  BaseObject: IBaseObject;
begin
  ListObject := TListPtr<IBaseObject>.Create();
  TestObj1 := TListPtrTestObject.Create;
  TestObj2 := TListPtrTestObject.Create;
  TestObj3 := TListPtrTestObject.Create;

  ListObject.PushBack(TestObj1);
  ListObject.PushBack(TestObj2);
  ListObject.PushBack(TestObj3);
  ListObject.PushBack(nil);
  ListObject.PushBack(nil);

  TestObj4 := TListPtrTestObject.Create();

  ListObject.SetItemAt(1, TestObj4);
  ListObject.SetItemAt(2, nil);
  ListObject.SetItemAt(3, TestObj4);
  ListObject.SetItemAt(4, nil);

  Assert.AreEqual<SizeT>(ListObject.GetCount(), 5);

  BaseObject := ListObject.GetItemAt(2);
  Assert.IsTrue(BaseObject = nil);
end;

procedure TTest_ListPtr.CoreType();
var
  ListObj: IListPtr<IBaseObject>;
  CoreType: TCoreType;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  CoreType := GetCoreType(ListObj);

  Assert.AreEqual(CoreType, ctList);
end;

procedure TTest_ListPtr.ToString();
var
  ListObj: IListPtr<IBaseObject>;
  BaseObject1, BaseObject2: IBaseObject;
  Str: string;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  CreateBaseObject(BaseObject1);
  CreateBaseObject(BaseObject2);

  ListObj.PushBack(BaseObject1);
  ListObj.PushBack(BaseObject2);
  Str := ListObj.ToString();

  Assert.AreEqual(Str, '[ daq::IInspectable, daq::IInspectable ]');
end;

procedure TTest_ListPtr.Freeze();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);

  Assert.WillNotRaiseAny(procedure()
    begin
      Freezable.Freeze();
    end
  );
  Assert.IsTrue(Freezable.IsFrozen());
end;

procedure TTest_ListPtr.SetItemAtWhenFrozen();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      ListObj.SetItemAt(0, nil);
    end,
    ERTFrozenException
  );
end;

procedure TTest_ListPtr.PushBackWhenFrozen();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      ListObj.PushBack(nil);
    end,
    ERTFrozenException
  );
end;

procedure TTest_ListPtr.PushFrontWhenFrozen();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      ListObj.PushFront(nil);
    end,
    ERTFrozenException
  );
end;

procedure TTest_ListPtr.PopBackWhenFrozen();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
  BaseObject: IBaseObject;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      BaseObject := ListObj.PopBack();
    end,
    ERTFrozenException
  );
end;

procedure TTest_ListPtr.PopFrontWhenFrozen();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
  BaseObject: IBaseObject;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      BaseObject := ListObj.PopFront();
    end,
    ERTFrozenException
  );
end;

procedure TTest_ListPtr.InsertAtWhenFrozen();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
  BaseObject: IBaseObject;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      CreateBaseObject(BaseObject);
      ListObj.InsertAt(0, BaseObject);
    end,
    ERTFrozenException
  );
end;

procedure TTest_ListPtr.RemoveAtWhenFrozen();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
  BaseObject: IBaseObject;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      BaseObject := ListObj.RemoveAt(0);
    end,
    ERTFrozenException
  );
end;

procedure TTest_ListPtr.DeleteAtWhenFrozen();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      ListObj.DeleteAt(0);
    end,
    ERTFrozenException
  );
end;

procedure TTest_ListPtr.ClearWhenFrozen();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);
  Freezable.Freeze();

  Assert.WillRaise(procedure()
    begin
      ListObj.Clear();
    end,
    ERTFrozenException
  );
end;

procedure TTest_ListPtr.IsFrozenFalse();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);

  Assert.IsFalse(Freezable.IsFrozen());
end;

procedure TTest_ListPtr.DoubleFreeze();
var
  ListObj: IListPtr<IBaseObject>;
  Freezable: IFreezablePtr;
  Intf: IFreezable;
  Err: ErrCode;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Freezable := GetFreezableInterface(ListObj);

  Freezable.Freeze();
  Intf := Freezable as IFreezable;

  Err := Intf.Freeze();
  Assert.AreEqual(Err, OPENDAQ_IGNORED)
end;

procedure TTest_ListPtr.SerializeId();
var
  ListObj: IListPtr<IBaseObject>;
  Serializable: ISerializablePtr;
  SerializeId : PAnsiChar;
begin
  ListObj := TListPtr<IBaseObject>.Create();
  Serializable := GetSerializableInterface(ListObj);

  Assert.WillRaise(procedure()
    begin
      SerializeId := Serializable.GetSerializeId();
      SerializeId := nil;
    end,
    ERTNotImplementedException
  );
end;

procedure TTest_ListPtr.Setup();
begin

end;

procedure TTest_ListPtr.TearDown();
begin

end;

{ TTestObject }

function TListPtrTestObject.BorrowInterface(const IID: TGUID; out Obj): HResult;
begin
  Result := 0;
end;

procedure TListPtrTestObject.Dispose;
begin
end;

function TListPtrTestObject.EqualsObject(Other: IBaseObject; out Equals: Boolean): ErrCode;
begin
  if Other = Self as IBaseObject then
    Equals := True
  else
    Equals := False;

  Result := OPENDAQ_SUCCESS;
end;

function TListPtrTestObject.ToCharPtr(Str: PPAnsiChar): ErrCode;
begin
  Result := OPENDAQ_SUCCESS;
end;

function TListPtrTestObject.GetHashCodeEx(out HashCode: SizeT): ErrCode;
begin
  HashCode := SizeT(Self);
  Result := OPENDAQ_SUCCESS;
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_ListPtr);

end.
