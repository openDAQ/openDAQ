unit OpenDAQ.SmartPtrRegistry;

interface
uses
  OpenDAQ.CoreTypes,
  System.Generics.Collections;
  
type

  TPtrInfo = class
  private
    FPtrClass : SmartPtrClass;
    FPtrInterface : TGUID;
    FInterface : TGUID;
  public
    constructor Create(Intf : TGUID; PtrInterface : TGUID; PtrClass : SmartPtrClass);

    property RtInterface : TGUID read FInterface;
    property PtrClass : SmartPtrClass read FPtrClass;
    property PtrInterface : TGUID read FPtrInterface;
  end;

  TSmartPtrRegistry = class
  private
    class var FInterfaceToSmartPtr : TDictionary<TGUID, TPtrInfo>;
    class var FSmartPtrToInterface : TDictionary<TGUID, TPtrInfo>;
  public
    class procedure RegisterPtr(Intf: TGUID; PtrIntf : TGUID; Ptr : SmartPtrClass); static;
    class procedure UnregisterPtr(RtIntf : TGUID); static;

    class function IsPtrRegistered(Intf : TGUID) : Boolean; static;

    // IIntegerPtr -> IInteger
    class function GetInterfaceFromPtr(Intf : TGUID) : TGUID; static;

    // IInteger -> TIntegerPtr
    // IIntegerPtr -> TIntegerPtr
    class function GetPtrClass(Intf : TGUID) : SmartPtrClass; static;

    // IInteger -> IIntegerPtr
    class function GetPtrInterface(Intf : TGUID) : TGUID; static;
  end;
  
implementation

{ TRTExceptionRegistry }

class function TSmartPtrRegistry.IsPtrRegistered(Intf: TGUID): Boolean;
begin
  Result := FSmartPtrToInterface.ContainsKey(Intf);
end;

class procedure TSmartPtrRegistry.RegisterPtr(Intf: TGUID; PtrIntf : TGUID; Ptr : SmartPtrClass);
var
  Info : TPtrInfo;
begin
  Info := TPtrInfo.Create(Intf, PtrIntf, Ptr);

  FInterfaceToSmartPtr.Add(Intf, Info);
  FSmartPtrToInterface.Add(PtrIntf, Info);
end;

class procedure TSmartPtrRegistry.UnregisterPtr(RtIntf: TGUID);
var
  Info : TPtrInfo;
begin
  Info := FInterfaceToSmartPtr[RtIntf];

  FInterfaceToSmartPtr.Remove(RtIntf);
  FSmartPtrToInterface.Remove(Info.PtrInterface);

  Info.Free();
end;

class function TSmartPtrRegistry.GetInterfaceFromPtr(Intf: TGUID): TGUID;
begin
  Result := FSmartPtrToInterface[Intf].FInterface;
end;

class function TSmartPtrRegistry.GetPtrInterface(Intf: TGUID): TGUID;
begin
  Result := FInterfaceToSmartPtr[Intf].FPtrInterface;
end;

class function TSmartPtrRegistry.GetPtrClass(Intf: TGUID): SmartPtrClass;
begin
  if not FInterfaceToSmartPtr.ContainsKey(Intf) then
    Result := nil
  else
    Result := FInterfaceToSmartPtr[Intf].FPtrClass;

  if not Assigned(Result) and FSmartPtrToInterface.ContainsKey(Intf) then
    Result := FSmartPtrToInterface[Intf].FPtrClass;
end;

{ TPtrInfo }

constructor TPtrInfo.Create(Intf : TGUID; PtrInterface : TGUID; PtrClass : SmartPtrClass);
begin
  FInterface := Intf;
  FPtrInterface := PtrInterface;
  FPtrClass := PtrClass;
end;

initialization
  TSmartPtrRegistry.FInterfaceToSmartPtr := TDictionary<TGUID, TPtrInfo>.Create();
  TSmartPtrRegistry.FSmartPtrToInterface := TDictionary<TGUID, TPtrInfo>.Create();

finalization
  TSmartPtrRegistry.FInterfaceToSmartPtr.Free();
  TSmartPtrRegistry.FSmartPtrToInterface.Free();

end.