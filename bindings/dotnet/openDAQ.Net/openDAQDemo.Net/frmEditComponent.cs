using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using Daq.Core.OpenDAQ;

using Component = Daq.Core.OpenDAQ.Component;


namespace openDAQDemoNet;


public partial class frmEditComponent : Form
{
    private readonly Component                  _component;
    private readonly BindingList<AttributeItem> _attributeItems = new();

    public frmEditComponent(Component component)
    {
        InitializeComponent();

        _component = component;

        frmMain.InitializeDataGridView(this.gridAttributes);
    }

    private void frmEditComponent_Shown(object sender, EventArgs e)
    {
        SetWaitCursor();
        this.Update();

        frmMain.UpdateAttributes(_component, _attributeItems);

        //binding data late to not to "trash" GUI display beforehand
        this.gridAttributes.DataSource = _attributeItems;
        this.gridAttributes.Columns[nameof(PropertyItem.LockedImage)].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleCenter;
        this.gridAttributes.Refresh();
        this.gridAttributes.ClearSelection();
        this.gridAttributes.AutoResizeColumns();

        ResetWaitCursor();
    }

    private void frmEditComponent_FormClosing(object sender, FormClosingEventArgs e)
    {
        _attributeItems.Clear();
    }

    #region gridAttributes

    private void gridAttributes_CellDoubleClick(object sender, DataGridViewCellEventArgs e)
    {
        if (e.RowIndex < 0)
        {
            MessageBox.Show("No attribute selected", "Edit", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        var attributeItem = (AttributeItem)((DataGridView)sender).Rows[e.RowIndex].DataBoundItem;

        if (attributeItem.IsLocked)
        {
            MessageBox.Show("Attribute is locked", $"Edit attribute \"{attributeItem.DisplayName}\"", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        AttributeItem selectedAttributeObject = (AttributeItem)this.gridAttributes.SelectedRows[0].DataBoundItem;

        frmMain.EditSelectedAttribute(this, selectedAttributeObject);
        frmMain.UpdateAttributes((Component)selectedAttributeObject.OpenDaqObject, _attributeItems);
    }

    #region conetxtMenuItemGridAttributesEdit

    private void gridAttributes_CellContextMenuStripNeeded(object sender, DataGridViewCellContextMenuStripNeededEventArgs e)
    {
        if ((e.RowIndex < 0) || (e.ColumnIndex < 0))
            return;

        DataGridView dataGridView = ((DataGridView)sender);

        //select the clicked cell/row for context menu (which opens automatically after this)
        dataGridView.CurrentCell = dataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex];
    }

    private void contextMenuStripGridAttributes_Opening(object sender, CancelEventArgs e)
    {
        //init
        this.conetxtMenuItemGridAttributesEdit.Enabled = false;

        //get the DataGridView on which the context menu has been triggered
        var grid = (DataGridView)((ContextMenuStrip)sender).SourceControl;

        if (grid.SelectedRows.Count == 0)
        {
            e.Cancel = true;
            return;
        }

        Point cursor = grid.PointToClient(Cursor.Position);
        var info = grid.HitTest(cursor.X, cursor.Y);
        if (info.Type != DataGridViewHitTestType.Cell)
        {
            //no node clicked
            e.Cancel = true;
            return;
        }

        var selectedProperty = (AttributeItem)grid.SelectedRows[0].DataBoundItem;

        this.conetxtMenuItemGridAttributesEdit.Enabled = !selectedProperty.IsLocked;
    }

    private void conetxtMenuItemGridAttributesEdit_Click(object sender, EventArgs e)
    {
        var toolStripMenuItem = (ToolStripMenuItem)sender;
        var sourceControl = ((ContextMenuStrip)toolStripMenuItem.Owner).SourceControl;

        if (sourceControl == this.gridAttributes)
        {
            frmMain.EditSelectedAttribute(this, (AttributeItem)this.gridAttributes.SelectedRows[0].DataBoundItem);
            frmMain.UpdateAttributes(_component, _attributeItems);
        }
    }

    #endregion conetxtMenuItemGridAttributesEdit

    #endregion gridAttributes


    #region Common GUI methods

    /// <summary>
    /// Sets the wait cursor.
    /// </summary>
    private void SetWaitCursor()
    {
        Cursor.Current     = Cursors.WaitCursor;
        this.Cursor        = Cursors.WaitCursor;
        this.UseWaitCursor = true;
    }

    /// <summary>
    /// Resets to the default cursor.
    /// </summary>
    private void ResetWaitCursor()
    {
        this.UseWaitCursor = false;
        this.Cursor        = Cursors.Default;
        Cursor.Current     = Cursors.Default;
        base.ResetCursor();
    }

    #endregion Common GUI methods
}
