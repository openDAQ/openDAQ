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


using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


namespace openDAQDemoNet.Helpers;


internal static class GuiHelper
{

    #region Cursor

    /// <summary>
    /// Sets the wait cursor.
    /// </summary>
    /// <param name="control">The control to set the cursor for.</param>
    public static void SetWaitCursor(Control control)
    {
        Cursor.Current        = Cursors.WaitCursor;
        control.Cursor        = Cursors.WaitCursor;
        control.UseWaitCursor = true;
    }

    /// <summary>
    /// Resets to the default cursor.
    /// </summary>
    /// <param name="control">The control to reset the cursor for.</param>
    public static void ResetWaitCursor(Control control)
    {
        control.UseWaitCursor = false;
        control.Cursor        = Cursors.Default;
        Cursor.Current        = Cursors.Default;
        control.ResetCursor();
    }

    #endregion Cursor

    #region DataGridView

    /// <summary>
    /// Initializes the given <c>DataGridView</c>.
    /// </summary>
    /// <param name="grid">The <c>DataGridView</c> to initialize.</param>
    public static void InitializeDataGridView(DataGridView grid)
    {
        var columnHeadersDefaultCellStyle = grid.ColumnHeadersDefaultCellStyle;

        grid.EnableHeadersVisualStyles                          = false; //enable ColumnHeadersDefaultCellStyle
        columnHeadersDefaultCellStyle.BackColor                 = Color.FromKnownColor(KnownColor.ButtonFace);
        columnHeadersDefaultCellStyle.Font                      = new Font(grid.Font, FontStyle.Bold);
        columnHeadersDefaultCellStyle.SelectionBackColor        = columnHeadersDefaultCellStyle.BackColor; //selection not visible
        columnHeadersDefaultCellStyle.SelectionForeColor        = columnHeadersDefaultCellStyle.ForeColor; //selection not visible
        columnHeadersDefaultCellStyle.WrapMode                  = DataGridViewTriState.False;
        grid.ColumnHeadersHeightSizeMode                        = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
        grid.AlternatingRowsDefaultCellStyle.BackColor          = Color.FromArgb(0xFF, 0xF9, 0xF9, 0xF9);
        grid.AlternatingRowsDefaultCellStyle.SelectionBackColor = grid.AlternatingRowsDefaultCellStyle.BackColor; //selection not visible
        grid.AlternatingRowsDefaultCellStyle.SelectionForeColor = grid.AlternatingRowsDefaultCellStyle.ForeColor; //selection not visible
        grid.GridColor                                          = Color.FromArgb(0xFF, 0xE0, 0xE0, 0xE0);
        grid.DefaultCellStyle.SelectionBackColor                = grid.DefaultCellStyle.BackColor; //selection not visible
        grid.DefaultCellStyle.SelectionForeColor                = grid.DefaultCellStyle.ForeColor; //selection not visible
        grid.DefaultCellStyle.Alignment                         = DataGridViewContentAlignment.MiddleLeft;

        grid.RowTemplate.Height = 20;

        grid.BackgroundColor = SystemColors.Window;

        grid.DefaultCellStyle.DataSourceNullValue = null;

        grid.AllowUserToAddRows      = false;
        grid.AllowUserToDeleteRows   = false;
        grid.AllowUserToResizeRows   = false;
        grid.AllowUserToOrderColumns = false;
        grid.SelectionMode           = DataGridViewSelectionMode.FullRowSelect;
        grid.MultiSelect             = false;
        grid.ReadOnly                = true;
        grid.RowHeadersVisible       = false;
        grid.AutoSizeColumnsMode     = DataGridViewAutoSizeColumnsMode.AllCells;
        grid.AutoSizeRowsMode        = DataGridViewAutoSizeRowsMode.DisplayedCellsExceptHeaders;
        grid.ShowCellToolTips        = false;

        grid.ColumnAdded += (s, e) => e.Column.SortMode = DataGridViewColumnSortMode.NotSortable;
    }

    /// <summary>
    /// Updates the specified <c>DataGridView</c>.
    /// </summary>
    /// <param name="grid">The <c>DataGridView</c>.</param>
    public static void Update(DataGridView grid)
    {
        grid.ClearSelection();
        grid.AutoResizeColumns();
    }

    #endregion DataGridView

    #region TableLayoutPanel

    /// <summary>
    /// Clears the specified table layout panel.
    /// </summary>
    /// <param name="tableLayoutPanel">The table layout panel.</param>
    public static void Clear(TableLayoutPanel tableLayoutPanel)
    {
        foreach (Control control in tableLayoutPanel.Controls)
            control.Dispose();

        tableLayoutPanel.Controls.Clear();
        tableLayoutPanel.RowStyles.Clear();
        tableLayoutPanel.RowCount = 0;
    }

    /// <summary>
    /// Sets the specified information label visibility.
    /// </summary>
    /// <param name="label">The label.</param>
    /// <param name="isVisible">If set to <c>true</c> the label is visible and <c>Dock.Fill</c>, otherwise <c>false</c>.</param>
    public static void SetInfoLabelVisibility(Label label, bool isVisible)
    {
        if (!isVisible)
        {
            label.Dock     = DockStyle.None;
            label.AutoSize = true;
            label.Visible  = false;
        }
        else
        {
            label.Visible  = true;
            label.AutoSize = false;
            label.Dock     = DockStyle.Fill;
            label.BringToFront();
            label.Refresh();
        }
    }

    #endregion TableLayoutPanel
}
