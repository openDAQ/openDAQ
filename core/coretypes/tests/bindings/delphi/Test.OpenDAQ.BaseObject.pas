unit Test.OpenDAQ.BaseObject;

interface
uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type
  [TestFixture]
  TTest_BaseObject = class(TDSUnitTest)
  private
    FObjectCount: Nativeuint;
    FMemoryLeakExpected: Boolean;
  public
    [Setup]
    procedure Setup; override;
    [TearDown]
    procedure TearDown; override;
    [Test]
    procedure Create;
    [Test]
    procedure AddReleaseRef;
    [Test]
    procedure MemoryLeak;
    [Test]
    procedure ManualDispose;
    [Test]
    procedure Hashing;
    [Test]
    procedure Equality;
    [Test]
    procedure QueryInterface;
    [Test]
    procedure BorrowInterface;
    [Test]
    procedure ToCharPtr;
  end;

implementation

uses
  DS.UT.DSUnitTestEngineUnit, WinApi.Windows, OpenDAQ.CoreTypes.Errors, SysUtils;

{ TTest_BaseObject }

procedure TTest_BaseObject.AddReleaseRef;
var
  BaseObject: IBaseObject;
  Res: Cardinal;
begin
  Res := CreateBaseObject(BaseObject);
  Assert.AreEqual(Res, OPENDAQ_SUCCESS);

  Res := BaseObject._AddRef;
  Assert.AreEqual(2, Res);
  BaseObject._AddRef;
  BaseObject._AddRef;
  BaseObject._Release;
  BaseObject._Release;
  BaseObject._AddRef;
  BaseObject._Release;
  Assert.AreEqual(1, BaseObject._Release);
end;

procedure TTest_BaseObject.Create;
var
  BaseObject: IBaseObject;
  Res: Cardinal;
begin
  Res := CreateBaseObject(BaseObject);
  Assert.AreEqual(Res, OPENDAQ_SUCCESS);
  Assert.IsTrue(BaseObject <> nil);
end;


procedure TTest_BaseObject.Equality;
var
  BaseObject1, BaseObject2: IBaseObject;
  Eq: Boolean;
begin
  CreateBaseObject(BaseObject1);
  CreateBaseObject(BaseObject2);

  Eq := False;
  BaseObject1.EqualsObject(BaseObject1, Eq);
  Assert.IsTrue(Eq);

  BaseObject1.EqualsObject(nil, Eq);
  Assert.IsFalse(Eq);

  BaseObject1.EqualsObject(BaseObject2, Eq);
  Assert.IsFalse(Eq);

  BaseObject2.EqualsObject(BaseObject1, Eq);
  Assert.IsFalse(Eq);

  BaseObject2.EqualsObject(BaseObject2, Eq);
  Assert.IsTrue(Eq);
end;

procedure TTest_BaseObject.Hashing;
var
  BaseObject1, BaseObject2: IBaseObject;
  HashCode1, HashCode2: Nativeint;
begin
  CreateBaseObject(BaseObject1);
  CreateBaseObject(BaseObject2);

  Assert.AreEqual(BaseObject1.GetHashCodeEx(HashCode1), OPENDAQ_SUCCESS);
  Assert.AreEqual(BaseObject2.GetHashCodeEx(HashCode2), OPENDAQ_SUCCESS);

  Assert.IsTrue((HashCode1 <> 0) and (HashCode2 <> 0) and (HashCode1 <> HashCode2));
end;

procedure TTest_BaseObject.ManualDispose;
var
  BaseObject: IBaseObject;
begin
  CreateBaseObject(BaseObject);
  BaseObject.Dispose;
end;

procedure TTest_BaseObject.MemoryLeak;
var
  BaseObject: IBaseObject;
begin
  FMemoryLeakExpected := True;
  CreateBaseObject(BaseObject);

  BaseObject._AddRef;
end;

procedure TTest_BaseObject.QueryInterface;
const
  SomeRandomGuid: TGuid = '{F84EFA0D-99C9-46F6-AC46-BAACEF0DF3E6}';
var
  BaseObject1, BaseObject2, BaseObject3: IBaseObject;
  Unk: IUnknown;
  Eq: Boolean;
begin
  CreateBaseObject(BaseObject1);

  Assert.AreEqual(OPENDAQ_SUCCESS, BaseObject1.QueryInterface(IBaseObject, Pointer(BaseObject2)));

  Eq := False;
  BaseObject1.EqualsObject(BaseObject2, Eq);
  Assert.IsTrue(Eq);

  Assert.AreEqual(OPENDAQ_SUCCESS, BaseObject1.QueryInterface(IUnknown, Pointer(Unk)));
  Assert.AreEqual(4, Unk._AddRef);
  Assert.AreEqual(3, Unk._Release);

  Assert.AreEqual(OPENDAQ_ERR_NOINTERFACE, Cardinal(BaseObject1.QueryInterface(SomeRandomGuid, Pointer(BaseObject3))));
end;

// BorrowInterface should not be called in Delphi, because interface is automatically decrement when it goes out of scope.
// if used anyway, it should increment reference before it goes out of scope
procedure TTest_BaseObject.BorrowInterface;
const
  SomeRandomGuid: TGuid = '{F84EFA0D-99C9-46F6-AC46-BAACEF0DF3E6}';
var
  BaseObject1, BaseObject2, BaseObject3: IBaseObject;
  Eq: Boolean;
begin
	CreateBaseObject(BaseObject1);

  Assert.AreEqual(OPENDAQ_SUCCESS, BaseObject1.BorrowInterface(IBaseObject, Pointer(BaseObject2)));

  Eq := False;
  BaseObject1.EqualsObject(BaseObject2, Eq);
  Assert.IsTrue(Eq);

  Assert.AreEqual(OPENDAQ_ERR_NOINTERFACE, Cardinal(BaseObject1.QueryInterface(SomeRandomGuid, Pointer(BaseObject3))));

  // see above comment
  BaseObject2._AddRef;
end;


procedure TTest_BaseObject.ToCharPtr;
var
  BaseObject: IBaseObject;
  Str: PAnsiChar;
begin
  CreateBaseObject(BaseObject);
  BaseObject.ToCharPtr(@Str);

  Assert.AreEqual(string(Str), 'daq::IInspectable');

  DaqFreeMemory(Str);
end;

procedure TTest_BaseObject.Setup;
begin
  FObjectCount := DaqGetTrackedObjectCount;
end;

procedure TTest_BaseObject.TearDown;
var
  NewObjectCount, Diff: Nativeuint;
  Str: string;
begin
  if not FMemoryLeakExpected then
  begin
    NewObjectCount := DaqGetTrackedObjectCount;
    Diff := NewObjectCount - FObjectCount;
    Str := Format('%d object(s) still alive', [Diff]);
    if (NewObjectCount <> FObjectCount) then
    begin
      DaqPrintTrackedObjects;
      Assert.Fail(Str);
    end;
  end;
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_BaseObject);

end.
