/*
 * Copyright 2022-2024 openDAQ d.o.o.
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

using openDAQDemoNet.Helpers;

using Component = Daq.Core.OpenDAQ.Component;


namespace openDAQDemoNet;


public partial class frmEditComponent : Form
{
    private readonly Component                  _component;
    private readonly BindingList<AttributeItem> _attributeItems = [];

    public frmEditComponent(Component component)
    {
        InitializeComponent();

        _component = component;

        GuiHelper.InitializeDataGridView(this.gridAttributes);
    }

    private void frmEditComponent_Shown(object sender, EventArgs e)
    {
        GuiHelper.SetWaitCursor(this);
        this.Update();

        frmMain.UpdateAttributes(_component, _attributeItems);

        //binding data late to not to "trash" GUI display beforehand
        this.gridAttributes.DataSource = _attributeItems;
        this.gridAttributes.Columns[nameof(PropertyItem.LockedImage)].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleCenter;
        this.gridAttributes.Refresh();

        GuiHelper.Update(this.gridAttributes);

        GuiHelper.ResetWaitCursor(this);
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

        EditSelectedAttribute();
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
        var grid = (DataGridView)((ContextMenuStrip)sender).SourceControl!;

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
        var gridControl = ((ContextMenuStrip)toolStripMenuItem.Owner!).SourceControl as DataGridView;

        if (gridControl != this.gridAttributes)
            return;

        EditSelectedAttribute();
    }

    #endregion conetxtMenuItemGridAttributesEdit

    private void EditSelectedAttribute()
    {
        var selectedAttributeItem = (AttributeItem)this.gridAttributes.SelectedRows[0].DataBoundItem;

        frmMain.EditSelectedAttribute(this, selectedAttributeItem);
        frmMain.UpdateAttributes((Component)selectedAttributeItem.OpenDaqObject, _attributeItems);

        this.gridAttributes.ClearSelection();
        this.gridAttributes.AutoResizeColumns();
    }

    #endregion gridAttributes
}
