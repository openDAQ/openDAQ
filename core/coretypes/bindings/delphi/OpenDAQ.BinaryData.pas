unit OpenDAQ.BinaryData;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr;
  
type
  IBinaryDataPtr = interface(IObjectPtr<IBinaryData>)
  ['{629A1895-613D-4531-94F9-3D73E61FAB00}']
    function GetAddress() : Pointer;
    function GetSize(): SizeT;  
  end;

  TBinaryDataPtr = class(TObjectPtr<IBinaryData>, IBinaryDataPtr, IBinaryData)
  public
    constructor Create(ByteLength : SizeT); overload;
    constructor Create(Obj : IBaseObject); overload; override;
    constructor Create(Obj : IBinaryData); overload;

    function GetAddress() : Pointer;
    function GetSize(): SizeT;

  private
    function IBinaryData.GetAddress = Interface_GetAddress;
    function IBinaryData.GetSize = Interface_GetSize;

    function Interface_GetAddress(var Address: Pointer): ErrCode stdcall;
    function Interface_GetSize(var Size: SizeT): ErrCode stdcall;
  end;

implementation
uses
  OpenDAQ.Exceptions,
  OpenDAQ.SmartPtrRegistry;

{ TBinaryDataPtr }

constructor TBinaryDataPtr.Create(ByteLength: SizeT);
var
  Data : IBinaryData;
  Err : ErrCode;
begin
  Err := CreateBinaryData(Data, ByteLength);
  CheckRtErrorInfo(Err);

  inherited Create(Data);
end;

constructor TBinaryDataPtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TBinaryDataPtr.Create(Obj: IBinaryData);
begin
  inherited Create(Obj);
end;

function TBinaryDataPtr.GetAddress() : Pointer;
var
  Err : ErrCode;
  Ptr : Pointer;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetAddress(Ptr);
  CheckRtErrorInfo(Err);
  
  Result := Ptr;
end;

function TBinaryDataPtr.GetSize(): SizeT;
var
  Err : ErrCode;
  Size : SizeT;
begin
  if not Assigned(FObject) then
    raise ERTInvalidParameterException.Create('Interface object is nil.');

  Err := FObject.GetSize(Size);
  CheckRtErrorInfo(Err);

  Result := Size;
end;

function TBinaryDataPtr.Interface_GetAddress(var Address: Pointer): ErrCode;
begin
  Result := FObject.GetAddress(Address);
end;

function TBinaryDataPtr.Interface_GetSize(var Size: SizeT): ErrCode;
begin
  Result := FObject.GetSize(Size);
end;

initialization
  TSmartPtrRegistry.RegisterPtr(IBinaryData, IBinaryDataPtr, TBinaryDataPtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(IBinaryData);

end.