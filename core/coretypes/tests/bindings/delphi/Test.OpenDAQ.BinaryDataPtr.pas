unit Test.OpenDAQ.BinaryDataPtr;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type

  [TestFixture]
  TTest_BinaryDataPtr = class(TDSUnitTest)
  private
  public
    [Test]
    procedure Creating;
    [Test]
    procedure CreateEmpty;
    [Test]
    procedure WriteReadBufferSomeData;
  end;

implementation
uses
  WinApi.Windows,
  System.SysUtils,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.BinaryData;

{ TTest_BinaryData }

procedure TTest_BinaryDataPtr.Creating();
var
  BinaryDataObj: IBinaryDataPtr;
begin
  BinaryDataObj := TBinaryDataPtr.Create(5);

  Assert.AreEqual<SizeT>(BinaryDataObj.GetSize(), 5);
  Assert.AreNotEqual(BinaryDataObj.GetAddress(), nil);
end;

procedure TTest_BinaryDataPtr.CreateEmpty();
var
  BinaryDataObj: IBinaryDataPtr;
begin
  Assert.WillRaise(procedure()
    begin
      BinaryDataObj := TBinaryDataPtr.Create(0);
    end,
    ERTInvalidParameterException
  );
end;

procedure TTest_BinaryDataPtr.WriteReadBufferSomeData();
var
  BinaryDataObj: IBinaryDataPtr;
  Data: Pointer;
  TypedData: PInteger;
begin
  BinaryDataObj := TBinaryDataPtr.Create(sizeof(Integer));

  Data := BinaryDataObj.GetAddress();
  Assert.AreNotEqual(Data, nil);

  TypedData := PInteger(Data);

  TypedData^ := 123456789;
  Assert.AreEqual(123456789, TypedData^);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_BinaryDataPtr);

end.
