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
using System.Windows.Forms;

using Daq.Core.OpenDAQ;
using Daq.Core.Types;

using openDAQDemoNet.Helpers;


namespace openDAQDemoNet;


public partial class frmAddFunctionBlockDialog : Form
{
    /// <summary>The openDAQ instance.</summary>
    private readonly Instance _instance;
    /// <summary>The list of available function blocks. Bound to <c>DataGridView</c>.</summary>
    private readonly BindingList<FunctionBlockInfo> _functionBlocks = new();

    /// <summary>
    /// Initializes a new instance of the <see cref="frmAddFunctionBlockDialog"/> class.
    /// </summary>
    /// <param name="instance">The openDAQ instance.</param>
    public frmAddFunctionBlockDialog(Instance instance)
    {
        InitializeComponent();

        this.treeParentDevices.HideSelection = false;

        GuiHelper.InitializeDataGridView(this.gridFunctionBlocks);

        this._instance = instance;
    }

    #region event handlers

    /// <summary>
    /// Handles the Shown event of the <c>frmAddFunctionBlockDialog</c> control.<br/>
    /// Populates the <c>TreeView</c> and the <c>DataGridView</c>.
    /// </summary>
    /// <param name="sender">The source of the event.</param>
    /// <param name="e">The <see cref="EventArgs"/> instance containing the event data.</param>
    private void frmAddFunctionBlockDialog_Shown(object sender, EventArgs e)
    {
        GuiHelper.SetWaitCursor(this);
        this.Update();

        PopulateParentDevices(_instance);
        PopulateFunctionBlocks();

        //binding data late to not to "trash" GUI display beforehand
        this.gridFunctionBlocks.DataSource = _functionBlocks;

        GuiHelper.Update(this.gridFunctionBlocks);

        GuiHelper.ResetWaitCursor(this);
    }

    #region Context Menu

    private void contextMenuStripChildDevices_Opening(object sender, CancelEventArgs e)
    {
        //if (this.gridFunctionBlocks.SelectedRows.Count == 0)
        //    return;

        ////set enabled state of "Add device" menu according to its "Used" flag
        //int rowIndex = this.gridFunctionBlocks.SelectedRows[0].Index;
        //this.contextMenuItemChildDevicesAddDevice.Enabled = !_functionBlocks[rowIndex].IsUsed;
    }

    private void contextMenuItemChildDevicesAddDevice_Click(object sender, EventArgs e)
    {
        int rowIndex = this.gridFunctionBlocks.SelectedRows[0].Index;
        CreateAndAddFunctionBlockToParent(_functionBlocks[rowIndex].Id);
    }

    /// <summary>
    /// Handles the Click event of the <c>refreshToolStripMenuItem</c> control.<br/>
    /// It re-populates the <c>DataGridView</c> by retrieving the device information (scan).
    /// </summary>
    /// <param name="sender">The source of the event.</param>
    /// <param name="e">The <see cref="EventArgs"/> instance containing the event data.</param>
    private void contextMenuItemChildDevicesRefresh_Click(object sender, EventArgs e)
    {
        PopulateFunctionBlocks();
    }

    #endregion

    #region DataGridView

    private void gridFunctionBlocks_CellContextMenuStripNeeded(object sender, DataGridViewCellContextMenuStripNeededEventArgs e)
    {
        DataGridView dataGridView = ((DataGridView)sender);

        //select the clicked cell/row for context menu (which opens automatically after this)
        dataGridView.CurrentCell = dataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex];
    }

    /// <summary>
    /// Handles the CellDoubleClick event of the <c>gridFunctionBlocks</c> control.<br/>
    /// Connect the device and add it to the selected node in the <c>TreeView</c> when not already used.
    /// </summary>
    /// <param name="sender">The source of the event.</param>
    /// <param name="e">The <see cref="DataGridViewCellEventArgs"/> instance containing the event data.</param>
    private void gridFunctionBlocks_CellDoubleClick(object sender, DataGridViewCellEventArgs e)
    {
        if (e.RowIndex < 0)
        {
            MessageBox.Show("No child device selected", "Add", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        int rowIndex = this.gridFunctionBlocks.SelectedRows[0].Index;
        CreateAndAddFunctionBlockToParent(_functionBlocks[rowIndex].Id);
    }

    #endregion

    #endregion //event handlers .................................................................................

    /// <summary>
    /// Populates the parent devices tree (recursion).
    /// </summary>
    /// <param name="device">The device to be added.</param>
    /// <param name="rootNode">The root node to be populated (<c>null</c> on initial call).</param>
    private void PopulateParentDevices(Device device, TreeNode? rootNode = null)
    {
        //get the nodes collection to be populated (rootNode will be null for the first entry)
        TreeNodeCollection nodesCollection = rootNode?.Nodes ?? this.treeParentDevices.Nodes;

        //first call of recursion?
        if (rootNode == null)
            nodesCollection.Clear();

        //create new tree node
        var deviceNode = nodesCollection.Add(device.Name);
        deviceNode.Tag = device;

        IListObject<Device> childDevices = device.GetDevices();

        //has child devices?
        if (childDevices?.Count > 0)
        {
            //recursion
            foreach (var childDevice in childDevices)
                PopulateParentDevices(childDevice, deviceNode);
        }

        //first call of recursion?
        if (rootNode == null)
        {
            this.treeParentDevices.ExpandAll();
            this.treeParentDevices.Update();
            this.treeParentDevices.SelectedNode = deviceNode;
        }
    }

    /// <summary>
    /// Populates the child devices in the <c>DataGridView</c>.
    /// </summary>
    private void PopulateFunctionBlocks()
    {
        bool isWaitCursorAlreadyOn = this.UseWaitCursor;

        if (!isWaitCursorAlreadyOn)
            GuiHelper.SetWaitCursor(this);

        _functionBlocks.Clear();
        this.gridFunctionBlocks.Refresh(); //clear in GUI

        var availableFunctionBlockTypes = _instance.AvailableFunctionBlockTypes;
        foreach (var functionBlockType in availableFunctionBlockTypes.Values)
        {
            _functionBlocks.Add(new FunctionBlockInfo(functionBlockType));
        }

        if (!isWaitCursorAlreadyOn)
            GuiHelper.ResetWaitCursor(this);
    }

    /// <summary>
    /// Creates a function block from the given <paramref name="typeId"/> and adds it to the selected parent device in the tree.
    /// </summary>
    /// <param name="typeId">The function-block type-ID.</param>
    /// <returns></returns>
    private bool CreateAndAddFunctionBlockToParent(string typeId)
    {
        TreeNode? selectedParentDeviceNode = this.treeParentDevices.SelectedNode;
        Device?   selectedParentDevice     = selectedParentDeviceNode?.Tag as Device;

        if (selectedParentDevice == null)
        {
            MessageBox.Show("No parent device selected", "Add", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
        }

        GuiHelper.SetWaitCursor(this);

        try
        {
            //add the function block
            var functionBlock = selectedParentDevice.AddFunctionBlock(typeId);

            //add to tree
            TreeNode deviceNode = selectedParentDeviceNode!.Nodes.Add(functionBlock.Name);
            deviceNode.Tag = functionBlock;

            this.treeParentDevices.ExpandAll();
        }
        catch (Exception ex) //most probably an OpenDaqException
        {
            MessageBox.Show($"Error ´creating function block:\n{ex.Message}", "Function-block error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
        }
        finally
        {
            GuiHelper.ResetWaitCursor(this);
        }

        return true;
    }
}
