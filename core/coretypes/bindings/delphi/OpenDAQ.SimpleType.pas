//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//
//     RTGen (DelphiGenerator v4.0.1) on 03.10.2023 14:06:55.
// </auto-generated>
//------------------------------------------------------------------------------
unit OpenDAQ.SimpleType;

interface
uses
  OpenDAQ.CoreTypes,
  OpenDAQ.ObjectPtr,
  OpenDAQ.TType;

type
  {$MINENUMSIZE 4}

  ISimpleType = interface(IType)
  ['{0C4C5701-5051-549A-957C-21391854F6F4}']

  end;

  ISimpleTypePtr = interface(ITypePtr<ISimpleType>)
  ['{83e46186-d19a-5e7b-ad2a-1b74caaf1b31}']

  end;

  TSimpleTypePtr = class(TTypePtr<ISimpleType>, ISimpleTypePtr, ISimpleType)
  public
    constructor Create(Obj: IBaseObject); overload; override;
    constructor Create(Obj: ISimpleType); overload;

    // Factory constructors
    constructor Create(CoreType: TCoreType) overload;

  private
  end;

  function CreateSimpleType(out Obj: ISimpleType; CoreType: TCoreType): ErrCode; cdecl;

implementation
uses
  OpenDAQ.CoreTypes.Errors,
  OpenDAQ.Exceptions,
  OpenDAQ.CoreTypes.Config,
  OpenDAQ.SmartPtrRegistry;

  function CreateSimpleType(out Obj: ISimpleType; CoreType: TCoreType): ErrCode; external DSCoreTypesDLL name 'createSimpleType';

constructor TSimpleTypePtr.Create(Obj: ISimpleType);
begin
  inherited Create(Obj);
end;

constructor TSimpleTypePtr.Create(Obj: IBaseObject);
begin
  inherited Create(Obj);
end;

constructor TSimpleTypePtr.Create(CoreType: TCoreType);
var
  RawInterface: ISimpleType;
  Err: ErrCode;
begin
  Err := OpenDAQ.SimpleType.CreateSimpleType(RawInterface, CoreType);
  CheckRtErrorInfo(Err);

  inherited Create(RawInterface);
end;



initialization
  TSmartPtrRegistry.RegisterPtr(ISimpleType, ISimpleTypePtr, TSimpleTypePtr);

finalization
  TSmartPtrRegistry.UnregisterPtr(ISimpleType);

end.
