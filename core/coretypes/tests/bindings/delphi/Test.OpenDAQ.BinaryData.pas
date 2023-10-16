unit Test.OpenDAQ.BinaryData;

interface

uses
  DunitX.TestFramework, DS.UT.DSUnitTestUnit, OpenDAQ.CoreTypes;

type

  [TestFixture]
  TTest_BinaryData = class(TDSUnitTest)
  private
  public
    [Test]
    procedure Creating;
    [Test]
    procedure CreateEmpty;
    [Test]
    procedure NullParameter;
    [Test]
    procedure WriteReadBufferSomeData;
  end;

implementation
uses
  WinApi.Windows,
  System.SysUtils,
  DS.UT.DSUnitTestEngineUnit,
  OpenDAQ.CoreTypes.Errors;

{ TTest_BinaryData }

procedure TTest_BinaryData.Creating;
var
  BinaryDataObj: IBinaryData;
  Size: SizeT;
  Data: Pointer;
begin
	Assert.AreEqual(CreateBinaryData(BinaryDataObj, 5), OPENDAQ_SUCCESS);

  Assert.AreEqual(BinaryDataObj.GetSize(Size), OPENDAQ_SUCCESS);
	Assert.AreEqual(Size, SizeT(5));

  Assert.AreEqual(BinaryDataObj.GetAddress(Data), OPENDAQ_SUCCESS);
  Assert.AreNotEqual(Data, nil);
end;

procedure TTest_BinaryData.CreateEmpty;
var
  BinaryDataObj: IBinaryData;
begin
  Assert.AreEqual(CreateBinaryData(BinaryDataObj, 0), OPENDAQ_ERR_INVALIDPARAMETER);
end;

procedure TTest_BinaryData.NullParameter;
var
  BinaryDataObj: IBinaryData;
begin
  Assert.AreEqual(CreateBinaryData(BinaryDataObj, 5), OPENDAQ_SUCCESS);

  Assert.AreEqual(BinaryDataObj.GetSize(SizeT(nil^)), OPENDAQ_ERR_ARGUMENT_NULL);
  Assert.AreEqual(BinaryDataObj.GetAddress(Pointer(nil^)), OPENDAQ_ERR_ARGUMENT_NULL);
end;

procedure TTest_BinaryData.WriteReadBufferSomeData;
var
  BinaryDataObj: IBinaryData;
  Data: Pointer;
  TypedData: PInteger;
begin
  Assert.AreEqual(CreateBinaryData(BinaryDataObj, sizeof(Integer)), OPENDAQ_SUCCESS);

  Assert.AreEqual(BinaryDataObj.GetAddress(Data), OPENDAQ_SUCCESS);
  Assert.AreNotEqual(Data, nil);

  TypedData := PInteger(Data);

  TypedData^ := 123456789;
  Assert.AreEqual(123456789, TypedData^);
end;

initialization
  TDSUnitTestEngine.RegisterUnitTest(TTest_BinaryData);

end.
