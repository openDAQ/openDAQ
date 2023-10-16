unit MUIDrawGridUnit;

interface
uses
  MUITypes,
  System.Classes,
  Dsrt.Core.CoreTypes;

type
  TDSTimeDisplay = (tdDefault, tdLocal, tdUTC, tdTelemetry);
  TDSDrawGridCellType = (ctManual, ctText, ctEditText, ctNumber, ctEditNumber, ctButton, ctUDButton, ctColor, ctCombobox, ctLiveValue, ctCheckBox, ctSearchCombo, ctGroupControl, ct2DLiveValue);
  TDSDrawGridNumberFormat = (cfInteger, cfCustomFormat, cfDateTime, cfFileSize);
  TDSDrawGridCellState = (csNormal, csError, csWarning, csOK);
  TDSActionType = (atPrimary, atSecondary, atDouble);
  TDSDrawGridOption = (dsgQuickSorting, dsgColMoving, dsgColSizing, dsgCellSelect, dsgRowSelect, dsgPasteTable, dsgMouseAlwaysSelect);

  TDSColor = LongWord;

  TDSModifiersState = TShiftState;

  IDrawGridSearchItem = interface(IBaseObject)
    ['{8B8D03EC-A615-5215-BD27-1DFF31777C17}']
  end;

  IDrawGridCellComboItem = interface(IBaseObject)
    ['{13DC9FB8-5C87-5A86-A210-744438A6C63E}']
    function GetSearchItem(out Item : IDrawGridSearchItem) : ErrCode; stdcall;
    function GetText(out Text : IString) : ErrCode; stdcall;
  end;

  IDrawGridEventArgs = interface(IEventArgs)
    ['{66A78343-F275-5D12-8812-8465687B98DC}']
    function GetRow(out Row : Int64) : ErrCode; stdcall;
    function GetColumn(out Column : Int64) : ErrCode; stdcall;
  end;

  IDrawGridCellPropsArgs = interface(IDrawGridEventArgs)
    ['{478087E4-31CB-5710-874A-6D06BCED3971}']
    function GetAcceptsInput(out Input : Boolean) : ErrCode; stdcall;

    function GetText(out Text : IString) : ErrCode; stdcall;
    function SetText(Text : IString) : ErrCode; stdcall;

    function GetEditText(out EditText : IString) : ErrCode; stdcall;
    function SetEditText(Text : IString) : ErrCode; stdcall;

    function GetHint(out Hint : IString) : ErrCode; stdcall;
    function SetHint(Hint : IString) : ErrCode; stdcall;

    function GetNumber(out Number : Double) : ErrCode; stdcall;
    function SetNumber(Number : Double) : ErrCode; stdcall;

    function GetEditNumber(out EditNumber : Double) : ErrCode; stdcall;
    function SetEditNumber(Number : Double) : ErrCode; stdcall;

    function GetNumberUnit(out NumberUnit : IString) : ErrCode; stdcall;
    function SetNumberUnit(AUnit : IString) : ErrCode; stdcall;

    function GetAutoEnumEditText(out AutoEditEnum : Boolean) : ErrCode; stdcall;
    function SetAutoEnumEditText(Auto : Boolean) : ErrCode; stdcall;

    function GetTextColor(out Color : TDSColor) : ErrCode; stdcall;
    function SetTextColor(Color : TDSColor) : ErrCode; stdcall;

    function GetCellColor(out Color : TDSColor) : ErrCode; stdcall;
    function SetCellColor(Color : TDSColor) : ErrCode; stdcall;

    function GetIsButtonDown(out Down : Boolean) : ErrCode; stdcall;
    function SetIsButtonDown(Down : Boolean) : ErrCode; stdcall;

    function GetIsEnabled(out Enabled : Boolean) : ErrCode; stdcall;
    function SetIsEnabled(Enabled : Boolean) : ErrCode; stdcall;

    function GetCellType(out CellType : TDSDrawGridCellType) : ErrCode; stdcall;
    function SetCellType(CellType : TDSDrawGridCellType) : ErrCode; stdcall;

    function GetTextAlign(out Align : TDSTextAlign) : ErrCode; stdcall;
    function SetTextAlign(Align : TDSTextAlign) : ErrCode; stdcall;

    function GetIsGroup(out IsGroup : Boolean) : ErrCode; stdcall;
    function SetIsGroup(Group : Boolean) : ErrCode; stdcall;

    function GetIsSorted(out IsSorted : Boolean) : ErrCode; stdcall;
    function SetIsSorted(Sorted : Boolean) : ErrCode; stdcall;

    function GetNanFieldText(out NanText : IString) : ErrCode; stdcall;
    function SetNanFieldText(Text : IString) : ErrCode; stdcall;

    function GetCellState(out State : TDSDrawGridCellState) : ErrCode; stdcall;
    function SetCellState(State : TDSDrawGridCellState) : ErrCode; stdcall;

    function GetIsDateUTC(out UTC : Boolean) : ErrCode; stdcall;
    function SetIsDateUTC(UTC : Boolean) : ErrCode; stdcall;
  end;

  IDrawGridComboItemsArgs = interface(IDrawGridEventArgs)
    ['{56F17D79-18B2-5683-BFD4-3204CCF2BDCA}']
    function AddItem(Text : IString; Item : IDrawGridSearchItem; out Index : RtInt) : ErrCode; stdcall;
    function Add(Text : IString; out Index : RtInt) : ErrCode; stdcall;

    function Insert(Index : RtInt; Text : IString) : ErrCode; stdcall;
    function IndexOf(Text : IString; out Index : RtInt) : ErrCode; stdcall;

    function GetItems(out Items : IListObject) : ErrCode; stdcall;
  end;

  IDrawGridLiveValueArgs = interface(IDrawGridEventArgs)
    ['{786C1798-E0A9-5F5C-BB17-5BF28AA82369}']
    function GetShowAverage(out ShowAverage : Boolean) : ErrCode; stdcall;
    function SetShowAverage(ShowAverage : Boolean) : ErrCode; stdcall;

    function GetLimitMinimum(out LimitMinimum : Double) : ErrCode; stdcall;
    function SetLimitMinimum(LimitMinimum : Double) : ErrCode; stdcall;

    function GetLimitMaximum(out LimitMaximum : Double) : ErrCode; stdcall;
    function SetLimitMaximum(LimitMaximum : Double) : ErrCode; stdcall;

    function GetOverloadMinimum(out OverloadMinimum : Double) : ErrCode; stdcall;
    function SetOverloadMinimum(OverloadMinimum : Double) : ErrCode; stdcall;

    function GetOverloadMaximum(out OverloadMaximum : Double) : ErrCode; stdcall;
    function SetOverloadMaximum(OverloadMaximum : Double) : ErrCode; stdcall;

    function GetCurrentMinimum(out CurrentMinimum : Double) : ErrCode; stdcall;
    function SetCurrentMinimum(CurrentMinimum : Double) : ErrCode; stdcall;

    function GetCurrentMaximum(out CurrentMaximum : Double) : ErrCode; stdcall;
    function SetCurrentMaximum(CurrentMaximum : Double) : ErrCode; stdcall;

    function GetUnit(out AUnit : IString) : ErrCode; stdcall;
    function SetUnit(AUnit : IString) : ErrCode; stdcall;

    function GetPrecision(out Precision : RtInt) : ErrCode; stdcall;
    function SetPrecision(Precision : RtInt) : ErrCode; stdcall;

    function GetWarningString(out WarningString : IString) : ErrCode; stdcall;
    function SetWarningString(WarningString : IString) : ErrCode; stdcall;

    function GetStringValue(out Value : IString) : ErrCode; stdcall;
    function SetStringValue(Value : IString) : ErrCode; stdcall;

    function GetShowOverloadWarning(out OverloadWarning : Boolean) : ErrCode; stdcall;
    function SetShowOverloadWarning(OverloadWarning : Boolean) : ErrCode; stdcall;

    function GetShowLimitWarning(out LimitWarning : Boolean) : ErrCode; stdcall;
    function SetShowLimitWarning(LimitWarning : Boolean) : ErrCode; stdcall;

    function GetShowLimitValues(out LimitValues : Boolean) : ErrCode; stdcall;
    function SetShowLimitValues(LimitValues : Boolean) : ErrCode; stdcall;
  end;

  IDrawGrid2DLiveValueArgs = interface(IDrawGridEventArgs)
    ['{E0A633EA-29BC-5119-8FED-E9A66F60DEB9}']
    function GetNumberOfValues(out NumValues : RtInt) : ErrCode; stdcall;
    function SetNumberOfValues(NumValues : RtInt) : ErrCode; stdcall;

    function GetValues(out Values : IListObject) : ErrCode; stdcall;

    function GetMinimum(out Minimum : Double) : ErrCode; stdcall;
    function SetMinimum(Minimum : Double) : ErrCode; stdcall;

    function GetMaximum(out Maximum : Double) : ErrCode; stdcall;
    function SetMaximum(Maximum : Double) : ErrCode; stdcall;
  end;

  IDrawGridCellInputArgs = interface(IDrawGridEventArgs)
    ['{7C8EDA90-D102-5A78-8CA9-D9926D1C4A84}']
    function GetText(out Text : IString) : ErrCode; stdcall;
    function GetColor(out Color : TDSColor) : ErrCode; stdcall;
    function GetItemIndex(out Index : RtInt) : ErrCode; stdcall;
    function GetSearchItem(out Item : IDrawGridSearchItem) : ErrCode; stdcall;

    function GetIsValid(out Valid : Boolean) : ErrCode; stdcall;
    function SetIsValid(Valid : Boolean) : ErrCode; stdcall;

    function GetIsPasting(out Pasting : Boolean) : ErrCode; stdcall;
  end;

  IDrawGridCellActionArgs = interface(IDrawGridEventArgs)
    ['{6B8D4191-7852-544D-93BD-097D8CE6BDC1}']
    function GetOverrideAction(out OverrideAction : Boolean) : ErrCode; stdcall;
    function SetOverrideAction(OverrideAction : Boolean) : ErrCode; stdcall;

    function GetAbortMultiAction(out AbortAction : Boolean) : ErrCode; stdcall;
    function SetAbortMultiAction(AbortAction : Boolean) : ErrCode; stdcall;

    function GetActionType(out ActionType : TDSActionType) : ErrCode; stdcall;
    function GetModifierKeys(out Modifiers : TShiftState) : ErrCode; stdcall;

    function GetButtonDownState(out State : Boolean) : ErrCode; stdcall;
    function GetIsPasting(out Pasting : Boolean) : ErrCode; stdcall;
  end;

  IDrawGridCellActionStartStopArgs = interface(IDrawGridEventArgs)
    ['{F1460EC3-7A9E-5B25-BE10-005D2F914541}']
    function GetActionType(out ActionType : TDSActionType) : ErrCode; stdcall;
    function GetModifierKeys(out Modifiers : TDSModifiersState) : ErrCode; stdcall;
    function GetIsStart(out Start : Boolean) : ErrCode; stdcall;
  end;

  IDrawGridCellSelectedArgs = interface(IDrawGridEventArgs)
    ['{473F98C6-02E8-5E3B-B9C3-C12A758975FB}']
    function GetIsSelected(out Selected : Boolean) : ErrCode; stdcall;
  end;

  IDrawGridCellMultiSelectStartStop = interface(IEventArgs)
    ['{A7E757DE-19E6-559B-8327-BFB5E61468E6}']
    function GetIsStart(out Start : Boolean) : ErrCode; stdcall;
  end;

  IDrawGridCellPopupMenuArgs = interface(IDrawGridEventArgs)
    ['{E4C9CDC9-2CBA-54FC-9C2B-C253A4EFDB6E}']
    // TODO
  end;

  IDrawGridColumn = interface(IBaseObject)
    ['{6FD9DF6F-BFB1-5783-95E3-AFE94465D4C4}']
    function GetName(out Name : IString) : ErrCode; stdcall;
    function SetName(Name : IString) : ErrCode; stdcall;

    function GetColumnType(out ColType : TDSDrawGridCellType) : ErrCode; stdcall;
    function SetColumnType(ColType : TDSDrawGridCellType) : ErrCode; stdcall;
	
    function GetNumberFormat(out Format : TDSDrawGridNumberFormat) : ErrCode; stdcall;
    function SetNumberFormat(Format : TDSDrawGridNumberFormat) : ErrCode; stdcall;
	
    function GetDateIsUTC(out UTC : Boolean): ErrCode; stdcall;
    function SetDateIsUTC(UTC : Boolean): ErrCode; stdcall;
	
    function GetCustomFormat(out Format : IString) : ErrCode; stdcall;
    function SetCustomFormat(Format : IString) : ErrCode; stdcall;
	
    function GetDefaultFormat(out Format : IString) : ErrCode; stdcall;
    function SetDefaultFormat(Format : IString) : ErrCode; stdcall;
	
    function GetAllowMultiselect(out MultiSelect : Boolean): ErrCode; stdcall;
    function SetAllowMultiselect(MultiSelect : Boolean): ErrCode; stdcall;
	
    function GetTextAlign(out Align : TDSTextAlign): ErrCode; stdcall;
    function SetTextAlign(Align : TDSTextAlign): ErrCode; stdcall;
	
    function GetShowUnitOnEdit(out UnitsOnEdit : Boolean): ErrCode; stdcall;
    function SetShowUnitOnEdit(UnitsOnEdit : Boolean): ErrCode; stdcall;
	
    function GetNanFieldText(out NanFiled : IString) : ErrCode; stdcall;
    function SetNanFieldText(NanFiled : IString) : ErrCode; stdcall;
	
    function GetCellState(out State : TDSDrawGridCellState): ErrCode; stdcall;
    function SetCellState(State : TDSDrawGridCellState): ErrCode; stdcall;

    function GetAutoWidth(out AutoWidth : Boolean): ErrCode; stdcall;
    function SetAutoWidth(AutoWidth : Boolean): ErrCode; stdcall;

    function GetVisible(out Visible : Boolean): ErrCode; stdcall;
    function SetVisible(Visible : Boolean): ErrCode; stdcall;
	
    function GetUnavailable(out Unavailable : Boolean): ErrCode; stdcall;
    function SetUnavailable(Unavailable : Boolean): ErrCode; stdcall;
	
    function GetSortIndex(out SortIndex : RtInt): ErrCode; stdcall;
    function SetSortIndex(SortIndex : RtInt): ErrCode; stdcall;
	
    function GetWidth(out Width : RtInt): ErrCode; stdcall;
    function SetWidth(Width : RtInt): ErrCode; stdcall;

    function GetDefaultVisible(out Visible : Boolean): ErrCode; stdcall;
    function SetDefaultVisible(Visible : Boolean): ErrCode; stdcall;
	
    function GetDefaultSortIndex(out SortIndex : RtInt): ErrCode; stdcall;
    function SetDefaultSortIndex(SortIndex : RtInt): ErrCode; stdcall;
	
    function GetDefaultWidth(out Width : RtInt): ErrCode; stdcall;
    function SetDefaultWidth(Width : RtInt): ErrCode; stdcall;

    function GetUniqueKey(out UniqueKey : IString) : ErrCode; stdcall;
    function SetUniqueKey(UniqueKey : IString) : ErrCode; stdcall;
  end;

  IDrawGrid = interface(IControl)
    ['{F161449B-E1DD-51F7-B8BE-1C9991C7C308}']

    function GetColumns(out Columns : IListObject) : ErrCode; stdcall;
    function GetColumn(Index : RtInt; out Column : IDrawGridColumn) : ErrCode; stdcall;

    function SetColumn(Index: RtInt; Name: IString; ColType: TDSDrawGridCellType; Visible: Boolean; Width: RtInt; UniqueKey: IString) : ErrCode; stdcall;
    function ApplyColumns() : ErrCode; stdcall;

    function ApplyRows() : ErrCode; stdcall;
    function ApplyFilter(FilterStr: IString; Force: Boolean = False) : ErrCode; stdcall;

    function RefreshGroups() : ErrCode; stdcall;

    function SetRowCount(RowCount: RtInt) : ErrCode; stdcall;
    function SetColumnCount(ColCount: RtInt) : ErrCode; stdcall;
    function SetGridSize(RowCount : RtInt; ColumnCount: RtInt) : ErrCode; stdcall;

    function SelectCell(Column : RtInt; Row: RtInt; out Selected : Boolean): ErrCode; stdcall;

    function InvalidateColumn(RealCol: RtInt) : ErrCode; stdcall;
    function InvalidateRow(RealRow: RtInt) : ErrCode; stdcall;

    function GetCellAt(X : RtInt; Y: RtInt; out RealRow : RtInt; out RealCol: RtInt) : ErrCode; stdcall;

    function SelectRow(RealRow: RtInt; MoveAnchor: Boolean) : ErrCode; stdcall;

    function IsCellSelected(Col : RtInt; Row: RtInt; out Selected : Boolean): ErrCode; stdcall;
    function IsRowSelected(Row: RtInt; out Selected : Boolean): ErrCode; stdcall;

    function MoveCursorRowAndClearSelection(RelativeRowOffset: RtInt) : ErrCode; stdcall;
    function SelectRows(SelectedRows: IListObject) : ErrCode; stdcall;
    function Resort() : ErrCode; stdcall;
    function Sort(Column: RtInt; Incremental: Boolean) : ErrCode; stdcall;

    function GetFirstSelectableColumnIndex(out Index : RtInt): ErrCode; stdcall;
	
	{ Get / Set }

    function GetFilter(out Value: IString) : ErrCode; stdcall;
    function SetFilter(const Value: IString) : ErrCode; stdcall;

    function GetSortedColumn(RealCol: RtInt; out SortedCol : RtInt): ErrCode; stdcall;
    function GetSortedRow(RealRow: RtInt; out SortedRow : RtInt): ErrCode; stdcall;

    function GetRealColumn(SortedCol: RtInt; out RealCol : RtInt): ErrCode; stdcall;
    function GetRealRow(SortedRow: RtInt; out RealRow : RtInt): ErrCode; stdcall;

    function GetCursorRow(out RowIndex : RtInt) : ErrCode; stdcall;
    function GetCursorColumn(out ColIndex : RtInt) : ErrCode; stdcall;
    function GetIsMultiselect(out MultiSelect : Boolean) : ErrCode; stdcall;

    function GetCell(Col : RtInt; Row : RtInt; out Text : IString) : ErrCode; stdcall;

    function GetColumnCount(out Count : RtInt) : ErrCode; stdcall;
    function GetRowCount(out Count : RtInt) : ErrCode; stdcall;

    function GetStaticColumnCount(out Count : RtInt) : ErrCode; stdcall;

    function GetStaticRowCount(out Count : RtInt) : ErrCode; stdcall;
    function GetFilteredRowCount(out Count : RtInt) : ErrCode; stdcall;

    function GetDefaultRowHeight(out Height : RtInt) : ErrCode; stdcall;
    function SetDefaultRowHeight(Height : RtInt) : ErrCode; stdcall;

    function GetCaptionRowHeight(out Height : RtInt) : ErrCode; stdcall;
    function SetCaptionRowHeight(CaptionHeight : RtInt) : ErrCode; stdcall;

    function GetSortColumn(out SortedIndex : RtInt) : ErrCode; stdcall;
    function SetSortColumn(const Value: RtInt) : ErrCode; stdcall;

    function GetSortIncremental(out SortIncremental : Boolean) : ErrCode; stdcall;

    function GetTimeDisplay(out TimeDisplay : TDSTimeDisplay) : ErrCode; stdcall;
    function SetTimeDisplay(Display : TDSTimeDisplay) : ErrCode; stdcall;

    function GetGridVersion(out Version : RtInt) : ErrCode; stdcall;
    function SetGridVersion(Version : RtInt) : ErrCode; stdcall;

    //function GetGridGroups() : TDSGridGroups;

    // PUBLISHED
    function GetFixedColumns(out FixedCols : RtInt) : ErrCode; stdcall;
    function SetFixedColumns(FixedCols : RtInt) : ErrCode; stdcall;

    function GetFixedRows(out FixedRows : RtInt) : ErrCode; stdcall;
    function SetFixedRows(FixedRows : RtInt) : ErrCode; stdcall;

    //[default(dsgColSizing, dsgRowSelect, dsgCellSelect)]
    function GetOptions(out Options : TDSDrawGridOption) : ErrCode; stdcall;
    function SetOptions(Options : TDSDrawGridOption) : ErrCode; stdcall;

    { Events }

    function AddOnSortChanged(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnSortChanged(Listener : IEventHandler) : ErrCode; stdcall;

    function AddOnCellGetProps(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellGetProps(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellGetComboItems(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellGetComboItems(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellGetLiveValue(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellGetLiveValue(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellGet2DLiveValue(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellGet2DLiveValue(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellAction(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellAction(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellActionStartStop(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellActionStartStop(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellInput(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellInput(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellDrawManual(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellDrawManual(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellSelected(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellSelected(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellMultiSelectStartStop(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellMultiSelectStartStop(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellsDeselect(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellsDeselect(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnCellPopupMenu(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnCellPopupMenu(Listener : IEventHandler) : ErrCode; stdcall;
    
    function AddOnSetGroups(Listener : IEventHandler) : ErrCode; stdcall;
    function RemoveOnSetGroups(Listener : IEventHandler) : ErrCode; stdcall;
  end;

implementation

end.
