using Daq.Core.Types;


namespace openDAQDemoNet;


/// <summary>
/// General input dialog for strings, integers, floating-points, lists, dictionaries.
/// </summary>
public partial class frmInputDialog : Form
{
    private Func<bool> _validateFunc = ValidateNoOp;

    private long   _integerValue = long.MinValue;
    private double _floatValue   = double.NaN;

    private object? _minValue = null;
    private object? _maxValue = null;


    /// <summary>
    /// Prevents a default instance of the <see cref="frmInputDialog"/> class from being created.<br/>
    /// Use static <c>AskXYZ()</c> functions instead.
    /// </summary>
    private frmInputDialog()
    {
        InitializeComponent();

        MakeSimpleInput();
    }


    /// <summary>
    /// Handles the FormClosing event of the frmInputDialog control.<para/>
    /// When the dialog is not being canceled, validate the input value.
    /// </summary>
    /// <param name="sender">The source of the event.</param>
    /// <param name="e">The <see cref="FormClosingEventArgs"/> instance containing the event data.</param>
    private void frmInputDialog_FormClosing(object sender, FormClosingEventArgs e)
    {
        if (this.DialogResult == DialogResult.Cancel)
            return;

        //validate input and cancel closing if not valid
        if (!_validateFunc.Invoke())
            e.Cancel = true;
    }


    private void MakeSimpleInput()
    {
        this.comboBoxValue.DropDownStyle      = ComboBoxStyle.Simple;
        //this.comboBoxValue.AutoCompleteMode   = AutoCompleteMode.SuggestAppend;
        //this.comboBoxValue.AutoCompleteSource = AutoCompleteSource.CustomSource;
    }

    private void MakeDropDownListInput(StringObject[] selectionValues, int selectedIndex)
    {
        this.comboBoxValue.Items.AddRange(selectionValues);
        this.comboBoxValue.DropDownStyle = ComboBoxStyle.DropDownList;
        this.comboBoxValue.SelectedIndex = selectedIndex;
    }

    /// <summary>
    /// Default validate function with no operation.
    /// </summary>
    /// <returns>Always <c>true</c>.</returns>
    private static bool ValidateNoOp()
    {
        return true;
    }

    /// <summary>
    /// Validate function for integer input (sets also internal field if valid).
    /// </summary>
    /// <returns><c>true</c> for a valid input, otherwise <c>false</c>.</returns>
    private bool ValidateInteger()
    {
        string valueText     = this.comboBoxValue.Text.Trim();
        var numberStyle      = System.Globalization.NumberStyles.Integer;
        var invariantCulture = System.Globalization.CultureInfo.InvariantCulture;

        if (string.IsNullOrEmpty(valueText)
            || !long.TryParse(valueText, numberStyle, invariantCulture, out _integerValue))
        {
            MessageBox.Show(this, $"The input is not a valid integer value:\n-> \"{valueText}\"\nPlease try again.",
                            this.Text, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
        }

        if ((_minValue is long minValue) && (_maxValue is long maxValue))
        {
            if ((_integerValue < minValue) || (_integerValue > maxValue))
            {
                MessageBox.Show(this, $"The input is not within min/max range.\nPlease try again.",
                                this.Text, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }
        }

        return true;
    }

    /// <summary>
    /// Validate function for floating-point input (sets also internal field if valid).
    /// </summary>
    /// <returns><c>true</c> for a valid input, otherwise <c>false</c>.</returns>
    private bool ValidateFloat()
    {
        string valueText     = this.comboBoxValue.Text.Trim().Replace(',', '.');
        var numberStyle      = System.Globalization.NumberStyles.Float;
        var invariantCulture = System.Globalization.CultureInfo.InvariantCulture;

        if (string.IsNullOrEmpty(valueText)
            || !double.TryParse(valueText, numberStyle, invariantCulture, out _floatValue))
        {
            MessageBox.Show(this, $"The input is not a valid floating-point value:\n-> \"{valueText}\"\nPlease try again.",
                            this.Text, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
        }

        if ((_minValue is double minValue) && (_maxValue is double maxValue))
        {
            if ((_floatValue < minValue) || (_floatValue > maxValue))
            {
                MessageBox.Show(this, $"The input is not within min/max range.\nPlease try again.",
                                this.Text, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return false;
            }
        }

        return true;
    }



    /// <summary>
    /// Asks for a string value.
    /// </summary>
    /// <param name="owner">The owner, where to center this dialog on.</param>
    /// <param name="title">The title.</param>
    /// <param name="caption">The caption.</param>
    /// <param name="originalValue">The original value.</param>
    /// <returns>The new string value or the <paramref name="originalValue"/> if anything failed.</returns>
    public static string AskString(IWin32Window owner,
                                   string title,
                                   string caption,
                                   string originalValue)
    {
        using (var dlg = new frmInputDialog())
        {
            dlg.Text            = title;
            dlg.lblCaption.Text = caption;

            dlg.comboBoxValue.Text = originalValue;

            dlg.lblMinMax.Visible = false;

            if (dlg.ShowDialog(owner) == DialogResult.OK)
                return dlg.comboBoxValue.Text.Trim();
        }

        return originalValue;
    }

    /// <summary>
    /// Asks for an integer value.
    /// </summary>
    /// <param name="owner">The owner, where to center this dialog on.</param>
    /// <param name="title">The title.</param>
    /// <param name="caption">The caption.</param>
    /// <param name="originalValue">The original value.</param>
    /// <param name="min">(optional) The minimum value.</param>
    /// <param name="max">(optional) The maximum value.</param>
    /// <returns>The new integer value or the <paramref name="originalValue"/> if anything failed.</returns>
    public static long AskInteger(IWin32Window owner,
                                  string title,
                                  string caption,
                                  long originalValue,
                                  long? min = null,
                                  long? max = null)
    {
        using (var dlg = new frmInputDialog())
        {
            dlg.Text            = title;
            dlg.lblCaption.Text = caption;

            dlg.comboBoxValue.Text = originalValue.ToString();

            if (min.HasValue && max.HasValue)
            {
                dlg.lblMinMax.Text = $"Minimum = {min}; Maximum =  {max}";
                dlg._minValue      = min;
                dlg._maxValue      = max;
            }
            else
                dlg.lblMinMax.Visible = false;

            dlg._integerValue = originalValue;
            dlg._validateFunc = dlg.ValidateInteger;

            if (dlg.ShowDialog(owner) == DialogResult.OK)
            {
                return dlg._integerValue;
            }
        }

        return originalValue;
    }

    /// <summary>
    /// Asks for a floating point value.
    /// </summary>
    /// <param name="owner">The owner, where to center this dialog on.</param>
    /// <param name="title">The title.</param>
    /// <param name="caption">The caption.</param>
    /// <param name="originalValue">The original value.</param>
    /// <param name="min">(optional) The minimum value.</param>
    /// <param name="max">(optional) The maximum value.</param>
    /// <returns>The new floating point value or the <paramref name="originalValue"/> if anything failed.</returns>
    public static double AskFloat(IWin32Window owner,
                                  string title,
                                  string caption,
                                  double originalValue,
                                  double? min = null,
                                  double? max = null)
    {
        using (var dlg = new frmInputDialog())
        {
            dlg.Text            = title;
            dlg.lblCaption.Text = caption;

            dlg.comboBoxValue.Text = originalValue.ToString();

            if (min.HasValue && max.HasValue)
            {
                dlg.lblMinMax.Text = $"Minimum = {min}; Maximum =  {max}";
                dlg._minValue      = min;
                dlg._maxValue      = max;
            }
            else
                dlg.lblMinMax.Visible = false;

            dlg._floatValue   = originalValue;
            dlg._validateFunc = dlg.ValidateFloat;

            if (dlg.ShowDialog(owner) == DialogResult.OK)
            {
                return dlg._floatValue;
            }
        }

        return originalValue;
    }

    /// <summary>
    /// Asks for a selection from the given list.
    /// </summary>
    /// <param name="owner">The owner, where to center this dialog on.</param>
    /// <param name="title">The title.</param>
    /// <param name="caption">The caption.</param>
    /// <param name="originalValue">The original value.</param>
    /// <param name="selectionValues">The selection items.</param>
    /// <returns>The index of the newly selected list item or the <paramref name="originalValue"/> if anything failed.</returns>
    public static long AskList(IWin32Window owner,
                               string title,
                               string caption,
                               long originalValue,
                               IList<StringObject> selectionValues)
    {
        int    selectedIndex = (int)originalValue;
        if (selectedIndex >= selectionValues.Count)
            selectedIndex = -1;

        using (var dlg = new frmInputDialog())
        {
            dlg.Text            = title;
            dlg.lblCaption.Text = caption;

            dlg.MakeDropDownListInput(selectionValues.ToArray(), selectedIndex);

            dlg.lblMinMax.Visible = false;

            dlg.btnOK.Enabled = (dlg.comboBoxValue.Items.Count > 0);

            if (dlg.ShowDialog(owner) == DialogResult.OK)
                return dlg.comboBoxValue.SelectedIndex;
        }

        return originalValue;
    }

    /// <summary>
    /// Asks for a selection from the given dictionary.
    /// </summary>
    /// <param name="owner">The owner, where to center this dialog on.</param>
    /// <param name="title">The title.</param>
    /// <param name="caption">The caption.</param>
    /// <param name="originalValue">The original value.</param>
    /// <param name="selectionValues">The selection items.</param>
    /// <returns>The key of the newly selected dictionary item or the <paramref name="originalValue"/> if anything failed.</returns>
    public static long AskDict(IWin32Window owner,
                               string title,
                               string caption,
                               long originalValue,
                               IDictionary<IntegerObject, StringObject> selectionValues)
    {
        int    selectedIndex = (int)originalValue;
        if (selectedIndex >= selectionValues.Count)
            selectedIndex = -1;

        using (var dlg = new frmInputDialog())
        {
            dlg.Text            = title;
            dlg.lblCaption.Text = caption;

            dlg.MakeDropDownListInput(selectionValues.Values.ToArray(), selectedIndex);

            dlg.lblMinMax.Visible = false;

            dlg.btnOK.Enabled = (dlg.comboBoxValue.Items.Count > 0);

            if (dlg.ShowDialog(owner) == DialogResult.OK)
                return dlg.comboBoxValue.SelectedIndex;
        }

        return originalValue;
    }
}
