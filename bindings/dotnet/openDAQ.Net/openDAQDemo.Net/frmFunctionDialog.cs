/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


using System.ComponentModel;
using System.Globalization;

using Daq.Core.Objects;
using Daq.Core.Types;

using openDAQDemoNet.Helpers;


namespace openDAQDemoNet;


public partial class frmFunctionDialog : Form
{
    private const int          FLOW_ITEM_HEIGHT = 23;
  //private const AnchorStyles ANCHOR_L         = AnchorStyles.Left;
    private const AnchorStyles ANCHOR_LR        = AnchorStyles.Left | AnchorStyles.Right;

    private readonly Function?  _function;
    private readonly Procedure? _procedure;
    private readonly CoreType   _returnType;

    private int _tableLayoutRowHeight = -1;
    private int _tableLayoutMarginLR  = -1;

    private frmFunctionDialog(Function? function, Procedure? procedure, Property property)
    {
        if ((function != null) && (procedure != null))
            throw new ArgumentException("Either function or procedure must be null."); //bad style in ctor

        _function   = function;
        _procedure  = procedure;
        _returnType = property.CallableInfo.ReturnType;

        InitializeComponent();

        InitializeTableLayoutPanel(this.tableArguments);

        if (_procedure != null)
        {
            //modify title
            this.Text = this.Text.Replace("function", "procedure", StringComparison.InvariantCulture);

            //no result
            this.lblResult.Visible = false;
            this.txtResult.Visible = false;
        }

        UpdateTitleAndArguments(property);
    }

    private void btnExecute_Click(object sender, EventArgs e)
    {
        this.txtResult.Text = "Executing...";
        this.txtResult.Refresh();

        BaseObject? result = null;

        switch (tableArguments.RowCount)
        {
            case 0:
                if (_function != null)
                    result = _function.Call(null);
                else
                    _procedure?.Dispatch(null);
                break;

            case 1:
                if (_function != null)
                    result = _function.Call(GetBaseObject());
                else
                    _procedure?.Dispatch(GetBaseObject());
                break;

            case > 1:
                {
                    var list = CoreTypesFactory.CreateList<BaseObject>();
                    for (int rowNo = 0; rowNo < this.tableArguments.RowCount; ++rowNo)
                    {
                        BaseObject? item = GetBaseObject(rowNo);
                        list.Add(item ?? string.Empty);
                    }
                    if (_function != null)
                        result = _function.Call((BaseObject)list);
                    else
                        _procedure?.Dispatch((BaseObject)list);
                }
                break;
        }

        if (_function != null)
            this.txtResult.Text = GetString(result);


        //--- local functions --------------------------------------------------

        BaseObject? GetBaseObject(int rowNo = 0)
        {
            ComboBox comboBox = (ComboBox)this.tableArguments.Controls[$"cmbValue{rowNo}"]!;

            return (CoreType)comboBox.Tag! switch
            {
                CoreType.ctBool          => CoreTypesFactory.CreateBoolean(bool.Parse(comboBox.Text)),
                CoreType.ctInt           => CoreTypesFactory.CreateInteger(long.Parse(comboBox.Text)),
                CoreType.ctFloat         => CoreTypesFactory.CreateFloat(double.Parse(comboBox.Text, NumberStyles.Number, CultureInfo.InvariantCulture)),
                CoreType.ctString        => CoreTypesFactory.CreateString(comboBox.Text),
              //CoreType.ctList          =>
              //CoreType.ctDict          =>
              //CoreType.ctRatio         =>
              //CoreType.ctProc          =>
              //CoreType.ctObject        =>
              //CoreType.ctBinaryData    =>
              //CoreType.ctFunc          =>
              //CoreType.ctComplexNumber =>
              //CoreType.ctStruct        =>
              //CoreType.ctEnumeration   =>
              //CoreType.ctUndefined     =>
                _                        => null,
            };
        }

        string GetString(BaseObject? result)
        {
            //the _returnType contains the type of the result
            //but here we use the ToString() function to get the string representation

            if (result == null)
                return "no result";

            //if (result.Cast<BoolObject>() is BoolObject boolObject)
            //    return boolObject.ToString();

            //if (result.Cast<IntegerObject>() is IntegerObject integerObject)
            //    return integerObject.ToString();

            //if (result.Cast<FloatObject>() is FloatObject floatObject)
            //    return floatObject.ToString();

            //if (result.Cast<StringObject>() is StringObject stringObject)
            //    return stringObject.ToString();

            return $"{result} ({_returnType.ToString().Substring(2)})";
        }
    }

    private void btnClose_Click(object sender, EventArgs e)
    {
        this.Close();
    }

    /// <summary>
    /// Initializes the input-port view.
    /// </summary>
    private void InitializeTableLayoutPanel(TableLayoutPanel tableLayoutPanel)
    {
        _tableLayoutMarginLR  = tableLayoutPanel.Margin.Left + tableLayoutPanel.Margin.Right;
        _tableLayoutRowHeight = tableLayoutPanel.Margin.Top + FLOW_ITEM_HEIGHT + tableLayoutPanel.Margin.Bottom;

        tableLayoutPanel.GrowStyle = TableLayoutPanelGrowStyle.FixedSize; //we take care of RowCount ourselves

        //RowCount / RowStyles dynamically set in UpdateInputPorts()
        GuiHelper.Clear(tableLayoutPanel);

        tableLayoutPanel.ColumnStyles.Clear();
        tableLayoutPanel.ColumnCount = 3;
        tableLayoutPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, FLOW_ITEM_HEIGHT + _tableLayoutMarginLR));
        tableLayoutPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));//SizeType.AutoSize));
        tableLayoutPanel.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 50F));//SizeType.AutoSize));

        GuiHelper.SetInfoLabelVisibility(this.lblNoArguments, isVisible: true);
    }

    /// <summary>
    /// Updates the title and arguments table.
    /// </summary>
    /// <param name="property">The property.</param>
    private void UpdateTitleAndArguments(Property property)
    {
        this.Text += $" »{property.Name}«";

        if (!string.IsNullOrWhiteSpace(property.Description))
            this.txtDescription.Text = property.Description;

        GuiHelper.Clear(this.tableArguments);

        if (property.CallableInfo.Arguments != null)
        {
            GuiHelper.SetInfoLabelVisibility(this.lblNoArguments, isVisible: false);

            foreach (var argument in property.CallableInfo.Arguments)
            {
                AddArgument(argument);
            }
        }
    }

    /// <summary>
    /// Adds the given argument.
    /// </summary>
    /// <param name="argumentInfo">The argument info.</param>
    private void AddArgument(ArgumentInfo argumentInfo) //ToDo: what about argument with selection list?
    {
        //string longestDataSourceKey = signalsDataSource
        //                              .Select(kvp => kvp.Key)
        //                              .Aggregate((longest, next) => (next.Length > longest.Length) ? next : longest);
        int dropDownWidth = 100;//TextRenderer.MeasureText(longestDataSourceKey, this.tableArguments.Font).Width;

        //create controls
        var lblCaption = CreateCaptionLabel(argumentInfo.Name, ContentAlignment.MiddleRight);
        var cmbValue   = CreateComboBox(/*signalsDataSource*/GetDefaultValue(argumentInfo.Type), dropDownWidth);
        var lblType    = CreateCaptionLabel($"({argumentInfo.Type.ToString().Substring(2)})", ContentAlignment.MiddleCenter);

        //add row
        ++this.tableArguments.RowCount;
        this.tableArguments.RowStyles.Add(new RowStyle(SizeType.Absolute, _tableLayoutRowHeight));
        int rowNo = this.tableArguments.RowCount - 1;

        cmbValue.Name = $"cmbValue{rowNo}";
        cmbValue.Tag  = argumentInfo.Type;

        //add controls
        this.tableArguments.Controls.Add(lblCaption, 0, rowNo);
        this.tableArguments.Controls.Add(cmbValue,   1, rowNo);
        this.tableArguments.Controls.Add(lblType,    2, rowNo);

        //resize label column if necessary
        int preferredWidth = this.tableArguments.Margin.Left + lblCaption.PreferredWidth + this.tableArguments.Margin.Right;
        if (this.tableArguments.ColumnStyles[0].Width < preferredWidth)
            this.tableArguments.ColumnStyles[0].Width = preferredWidth;

        //add event handlers
        cmbValue.Validating += cmbValue_Validating;
        //cmbValue.SelectedIndexChanged += comboBoxSignals_SelectedIndexChanged;

        //store original selection
        ////cmbValue.Tag = selectedIndex;

        //only after control has been added to GUI (TableLayoutPanel) we can set SelectedIndex
        //cmbValue.SelectedIndex = selectedIndex;


        //--- local functions --------------------------------------------------

        static Label CreateCaptionLabel(string caption, ContentAlignment alignment)
        {
            return new Label()
            {
                AutoSize  = false,
                Text      = caption,
                TextAlign = alignment,
                Anchor    = ANCHOR_LR
            };
        }

        static ComboBox CreateComboBox(object? dataSource, int dropDownWidth)
        {
            if (dataSource is IBindingList bindingList)
            {
                return new ComboBox
                {
                    DropDownStyle = ComboBoxStyle.DropDownList,
                    Anchor        = ANCHOR_LR,
                    Height        = FLOW_ITEM_HEIGHT,
                    DropDownWidth = dropDownWidth,
                    DataSource    = bindingList,
                    DisplayMember = "Key",
                    ValueMember   = "Value"
                };
            }

            return new ComboBox
            {
                DropDownStyle = ComboBoxStyle.Simple,
                Anchor        = ANCHOR_LR,
                Height        = FLOW_ITEM_HEIGHT,
                Text          = dataSource?.ToString() ?? string.Empty
            };
        }

        object? GetDefaultValue(CoreType type)
        {
            return type switch
            {
                CoreType.ctBool          => default(bool),
                CoreType.ctInt           => default(long),
                CoreType.ctFloat         => default(double),
                CoreType.ctString        => string.Empty,
              //CoreType.ctList          => default,
              //CoreType.ctDict          => default,
              //CoreType.ctRatio         => default,
              //CoreType.ctProc          => default,
              //CoreType.ctObject        => default,
              //CoreType.ctBinaryData    => default,
              //CoreType.ctFunc          => default,
              //CoreType.ctComplexNumber => default,
              //CoreType.ctStruct        => default,
              //CoreType.ctEnumeration   => default,
              //CoreType.ctUndefined     => default,
                _                        => null
            };
        }

        void cmbValue_Validating(object? sender, CancelEventArgs e)
        {
            if (sender is not ComboBox comboBox)
                return;

            var text = comboBox.Text;

            switch ((CoreType)comboBox.Tag!)
            {
                case CoreType.ctBool:
                    e.Cancel = !bool.TryParse(text, out _);
                    break;
                case CoreType.ctInt:
                    e.Cancel = !long.TryParse(text, out _);
                    break;
                case CoreType.ctFloat:
                    e.Cancel = !double.TryParse(text, NumberStyles.Number, CultureInfo.InvariantCulture, out _);
                    break;
              //case CoreType.ctString:
              //    break;
              //case CoreType.ctList:
              //    break;
              //case CoreType.ctDict:
              //    break;
              //case CoreType.ctRatio:
              //    break;
              //case CoreType.ctProc:
              //    break;
              //case CoreType.ctObject:
              //    break;
              //case CoreType.ctBinaryData:
              //    break;
              //case CoreType.ctFunc:
              //    break;
              //case CoreType.ctComplexNumber:
              //    break;
              //case CoreType.ctStruct:
              //    break;
              //case CoreType.ctEnumeration:
              //    break;
              //case CoreType.ctUndefined:
              //    break;
              //default:
              //    break;
            }

            comboBox.BackColor = e.Cancel ? Color.LightCoral : SystemColors.Window;
        }

        //void comboBoxSignals_SelectedIndexChanged(object? sender, EventArgs e)
        //{
        //    if (sender is not ComboBox comboBox)
        //        return;

        //    //get the link Button
        //    var cellPos = this.tableArguments.GetCellPosition(comboBox);
        //    if (this.tableArguments.GetControlFromPosition(cellPos.Column + 1, cellPos.Row) is not Button btnLink)
        //        return;

        //    int originalSelection = (int)comboBox.Tag;
        //    bool isOriginalSelected = (originalSelection == comboBox.SelectedIndex);
        //    bool isInputPortConnected = (originalSelection > 0);

        //    btnLink.Enabled = !isOriginalSelected;
        //    btnLink.Image = GetLinkImage(isLinked: isInputPortConnected);
        //}
    }

    /// <summary>
    /// Opens the function dialog.
    /// </summary>
    /// <param name="owner">The GUI owner.</param>
    /// <param name="property">The property.</param>
    /// <param name="function">The function.</param>
    public static void OpenFunction(IWin32Window owner,
                                    Property property,
                                    Function function)
    {
        using (var dlg = new frmFunctionDialog(function, null, property))
        {
            dlg.ShowDialog(owner);
        }
    }

    /// <summary>
    /// Opens the procedure dialog.
    /// </summary>
    /// <param name="owner">The GUI owner.</param>
    /// <param name="property">The property.</param>
    /// <param name="procedure">The procedure.</param>
    public static void OpenProcedure(IWin32Window owner,
                                    Property property,
                                    Procedure procedure)
    {
        using (var dlg = new frmFunctionDialog(null, procedure, property))
        {
            dlg.ShowDialog(owner);
        }
    }
}
