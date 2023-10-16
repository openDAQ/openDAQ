unit MUITypes;

interface
uses
  Dsrt.Core.CoreTypes;

type
  TVisiblity = (visVisible, visCollapsed, visHidden);
  TDSTextAlign = (teDefault, teLeft, teCenter, teRight);

  IControl = interface(IBaseObject)
    ['{0DBB6A0B-B7C7-57A3-B8FF-1CF455C6DE06}']
    function getName(out name : IString) : ErrCode; stdcall;
    function getType(out name : IString) : ErrCode; stdcall;

    function setWidth(width : SizeT) : ErrCode; stdcall;
    function getWidth(out width : SizeT) : ErrCode; stdcall;

    function setHeight(height : SizeT) : ErrCode; stdcall;
    function getHeight(out height : SizeT) : ErrCode; stdcall;

    function getMarginTop(out marginTop : SizeT) : ErrCode; stdcall;
    function setMarginTop(marginTop : SizeT) : ErrCode; stdcall;

    function setMarginBottom(marginTop : SizeT) : ErrCode; stdcall;
    function getMarginBottom(out marginTop : SizeT) : ErrCode; stdcall;

    function getMarginRight(out marginTop : SizeT) : ErrCode; stdcall;
    function setMarginRight(marginTop : SizeT) : ErrCode; stdcall;

    function getMarginLeft(out marginTop : SizeT) : ErrCode; stdcall;
    function setMarginLeft(marginTop : SizeT) : ErrCode; stdcall;

    function getIsEnabled(out isEnabled : Boolean) : ErrCode; stdcall;
    function setIsEnabled(isEnabled : Boolean) : ErrCode; stdcall;

    function getVisiblity(out visibliy : TVisiblity) : ErrCode; stdcall;
    function setVisiblity(visibliy : TVisiblity) : ErrCode; stdcall;

    function setBackground(color : Cardinal) : ErrCode; stdcall;
    function getBackground(out color : Cardinal) : ErrCode; stdcall;

    function invalidate() : ErrCode; stdcall;
  end;

  IEventArgs = interface(IBaseObject)
    ['{033CE044-C292-5601-BE84-827AD864D6A7}']
    function getEventId(out eventId : RtInt) : ErrCode; stdcall;
    function getEventName(out eventName : IString) : ErrCode; stdcall;
  end;

  IEventHandler = interface(IBaseObject)
    ['{B6DEE98F-F7EF-5659-A3D9-5739D25B62A5}']
    function handleEvent(emitter : IControl; eventArgs : IEventArgs) : ErrCode; stdcall;
  end;

  IParentControl = interface(IControl)
    ['{D8A17490-5C9C-5563-B08F-6B44961AF514}']
    function findControl(Name : IString; out Control : IControl) : ErrCode; stdcall;
    function getChildCount(out count : SizeT) : ErrCode; stdcall;
  end;

  ILayoutControl = interface(IParentControl)
    ['{4652FC72-4BA4-5648-A768-E9487FA9F04F}']
    function addControl(control : IControl): ErrCode; stdcall;
  end;

  IContentControl = interface(IParentControl)
    ['{6347A9AF-9099-5024-8934-CAC899288D2F}']
    function setControl(control : IControl): ErrCode; stdcall;
    function getControl(out control : IControl): ErrCode; stdcall;
  end;

  IWindow = interface(IContentControl)
    ['{AD26377F-E14D-569C-BEE2-98D0F7F3C7A7}']
    function setUI(mui : PAnsiChar) : ErrCode; stdcall;
  end;

  IButton = interface(IControl)
    ['{961AB5B3-F178-505E-8346-F5E84322C6EF}']
    function setCaption(caption : IString) : ErrCode; stdcall;
    function getCaption(out caption : IString) : ErrCode; stdcall;

    function addOnClick(eventHandler : IEventHandler) : ErrCode; stdcall;
    function removeOnClick(eventHandler : IEventHandler) : ErrCode; stdcall;
  end;

  ITextBox = interface(IControl)
    ['{293D6CB9-B2FA-53EE-AE31-9911A08071BF}']
    function setText(labelText : IString) : ErrCode; stdcall;
    function getText(out labelText : IString) : ErrCode; stdcall;

    function getPlaceholder(out placeholder : IString) : ErrCode; stdcall;
    function setPlaceholder(placeholder : IString) : ErrCode; stdcall;

    function addOnTextChanged(eventHandler : IEventHandler) : ErrCode; stdcall;
    function removeOnTextChanged(eventHandler : IEventHandler) : ErrCode; stdcall;
  end;

  IComboBox = interface(IControl)
    ['{629F8449-3C91-5687-8F8C-F7D208249BFB}']

    function getItems(out list : IListObject) : ErrCode; stdcall;
    function getCount(out Size: SizeT): ErrCode; stdcall;

    function getIndexOf(item : IString; out index : RtInt) : ErrCode; stdcall;

    function getItemAt(Index: SizeT; out item : IString): ErrCode; stdcall;
    function setItemAt(Index: SizeT; item: IString): ErrCode; stdcall;

    function addItem(item : IString) : ErrCode; stdcall;
    function insertItemAt(Index: SizeT; item: IString): ErrCode; stdcall;
    function deleteItemAt(Index: SizeT): ErrCode; stdcall;

    function clear(): ErrCode; stdcall;

    function clearSelection() : ErrCode; stdcall;

    function getSelectedIndex(out index : SizeT) : ErrCode; stdcall;
    function setSelectedIndex(index : SizeT) : ErrCode; stdcall;

    function getSelectedItem(out item : IString) : ErrCode; stdcall;
    function setSelectedItem(item : IString) : ErrCode; stdcall;

    function addOnChange(eventHandler : IEventHandler) : ErrCode; stdcall;
    function removeOnChange(eventHandler : IEventHandler) : ErrCode; stdcall;
  end;

  IToggleButton = interface(IControl)
    ['{59438B66-42AA-5754-B13B-E2459BCB74F3}']
    function setLabel(labelText : IString) : ErrCode; stdcall;
    function getLabel(out labelText : IString) : ErrCode; stdcall;

    function setIsChecked(checked : Boolean) : ErrCode; stdcall;
    function getIsChecked(out checked : Boolean) : ErrCode; stdcall;

    function addOnCheckedChanged(eventHandler : IEventHandler) : ErrCode; stdcall;
    function removeOnCheckedChanged(eventHandler : IEventHandler) : ErrCode; stdcall;
  end;

  IRadioButton = interface(IToggleButton)
    ['{96F9D6DC-9532-55FA-917A-75325CED2D39}']
  end;

  ICheckBox = interface(IToggleButton)
    ['{A17966FE-C3A6-5213-B9B1-FD4B5731FC65}']
  end;

  ILabel = interface(IControl)
    ['{325E2061-9424-5B4D-A97C-B8A1B58C399F}']

    function setText(labelText : IString) : ErrCode; stdcall;
    function getText(out labelText : IString) : ErrCode; stdcall;
  end;

  ICaptionLabel = interface(ILabel)
    ['{2BDC6326-71CF-5858-A59C-C0030265C54F}']
  end;

  ICaptionPanel = interface(IContentControl)
    ['{10043C49-9148-5FEE-8918-3A888C6C6838}']

    function setTitle(panelTitle : IString) : ErrCode; stdcall;
    function getTitle(out panelTitle : IString) : ErrCode; stdcall;
  end;

  IStackPanel = interface(ILayoutControl)
    ['{9B16D9F7-FD2A-5CB3-A89B-F65FF4FEB96C}']
  end;

  IMUI = interface(IBaseObject)
    ['{9B16D9F7-FD2A-5CB3-A89B-F65FF4FEB96C}']
    function getWindow(out window : IWindow) : ErrCode; stdcall;
  end;

  IGridLayout = interface(ILayoutControl)
    ['{B0124421-8768-5E46-909B-C620B60608BE}']
  end;

  function RtToString(value : IString) : string;

implementation
  uses
    Dsrt.Core.CoreTypes.Errors;

  function RtToString(Value : IString) : string;
  var
    Ptr : PAnsiChar;
    Error : ErrCode;
  begin
    if not Assigned(Value) then
      CheckError(DSRT_ERR_INVALIDPARAMETER);

    Error := value.GetCharPtr(@Ptr);
    CheckError(Error);

    Result := string(UTF8String(Ptr));
  end;
end.
