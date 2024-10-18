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


using System.ComponentModel;
using System.Windows.Forms;

using Daq.Core.OpenDAQ;
using Daq.Core.Types;

using openDAQDemoNet.Helpers;


namespace openDAQDemoNet;


public partial class frmAddDeviceDialog : Form
{
    /// <summary>The openDAQ instance.</summary>
    private readonly Instance _instance;
    /// <summary>The list of used (connected) device instances which can be parent for new connections. To determine the used-state in <c>DataGridView</c>.</summary>
    private readonly List<Device> _usedDevices = new();
    /// <summary>The list of available device information (from device scan). Bound to <c>DataGridView</c>.</summary>
    private readonly BindingList<ChildDevice> _childDevices = new();

    /// <summary>
    /// Initializes a new instance of the <see cref="frmAddDeviceDialog"/> class.
    /// </summary>
    /// <param name="instance">The openDAQ instance.</param>
    public frmAddDeviceDialog(Instance instance)
    {
        InitializeComponent();

        this.treeParentDevices.HideSelection = false;

        GuiHelper.InitializeDataGridView(this.gridChildDevices);

        this._instance = instance;
    }

    #region event handlers

    /// <summary>
    /// Handles the Shown event of the <c>frmAddDeviceDialog</c> control.<br/>
    /// Populates the <c>TreeView</c> and the <c>DataGridView</c>.
    /// </summary>
    /// <param name="sender">The source of the event.</param>
    /// <param name="e">The <see cref="EventArgs"/> instance containing the event data.</param>
    private void frmAddDeviceDialog_Shown(object sender, EventArgs e)
    {
        GuiHelper.SetWaitCursor(this);
        this.Update();

        PopulateParentDevices(_instance);
        PopulateChildDevices();

        //binding data late to not to "trash" GUI display beforehand
        this.gridChildDevices.DataSource = _childDevices;
        this.gridChildDevices.Columns["Used"].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleCenter;

        GuiHelper.Update(this.gridChildDevices);

        GuiHelper.ResetWaitCursor(this);
    }

    #region Context Menu

    private void contextMenuStripChildDevices_Opening(object sender, CancelEventArgs e)
    {
        if (this.gridChildDevices.SelectedRows.Count == 0)
            return;

        //set enabled state of "Add device" menu according to its "Used" flag
        int rowIndex = this.gridChildDevices.SelectedRows[0].Index;
        this.contextMenuItemChildDevicesAddDevice.Enabled = !_childDevices[rowIndex].IsUsed;
    }

    private void contextMenuItemChildDevicesAddDevice_Click(object sender, EventArgs e)
    {
        //this.txtConnectionString.Text has already been set after (right-)clicking on the row
        this.btnAdd.PerformClick();
    }

    /// <summary>
    /// Handles the Click event of the <c>refreshToolStripMenuItem</c> control.<br/>
    /// It re-populates the <c>DataGridView</c> by retrieving the device information (scan).
    /// </summary>
    /// <param name="sender">The source of the event.</param>
    /// <param name="e">The <see cref="EventArgs"/> instance containing the event data.</param>
    private void contextMenuItemChildDevicesRefresh_Click(object sender, EventArgs e)
    {
        PopulateChildDevices();
    }

    #endregion

    #region DataGridView

    private void gridChildDevices_CellContextMenuStripNeeded(object sender, DataGridViewCellContextMenuStripNeededEventArgs e)
    {
        DataGridView dataGridView = ((DataGridView)sender);

        //select the clicked cell/row for context menu (which opens automatically after this)
        dataGridView.CurrentCell = dataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex];
    }

    /// <summary>
    /// Handles the SelectionChanged event of the <c>gridChildDevices</c> control.<br/>
    /// Populate the <c>txtConnectionString</c> control with the selected item.
    /// </summary>
    /// <param name="sender">The source of the event.</param>
    /// <param name="e">The <see cref="EventArgs"/> instance containing the event data.</param>
    private void gridChildDevices_SelectionChanged(object sender, EventArgs e)
    {
        if (this.gridChildDevices.SelectedRows.Count == 0)
            return;

        //set the selected connection string into the text box
        int rowIndex = this.gridChildDevices.SelectedRows[0].Index;
        this.txtConnectionString.Text = _childDevices[rowIndex].ConnectionString;
    }

    /// <summary>
    /// Handles the CellDoubleClick event of the <c>gridChildDevices</c> control.<br/>
    /// Connect the device and add it to the selected node in the <c>TreeView</c> when not already used.
    /// </summary>
    /// <param name="sender">The source of the event.</param>
    /// <param name="e">The <see cref="DataGridViewCellEventArgs"/> instance containing the event data.</param>
    private void gridChildDevices_CellDoubleClick(object sender, DataGridViewCellEventArgs e)
    {
        if (e.RowIndex < 0)
        {
            MessageBox.Show("No child device selected", "Add", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        ChildDevice childDevice = _childDevices[e.RowIndex];

        if (childDevice.IsUsed)
        {
            MessageBox.Show("Device already in use", "Add", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        string connectionString = childDevice.ConnectionString;

        if (ConnectAndAddDeviceToParent(connectionString))
            childDevice.SetUsed();
    }

    #endregion

    /// <summary>
    /// Handles the Click event of the <c>btnAdd</c> control.<br/>
    /// Connect the device with the given connection string and add it to the selected node in the <c>TreeView</c> when not already used.
    /// </summary>
    /// <param name="sender">The source of the event.</param>
    /// <param name="e">The <see cref="EventArgs"/> instance containing the event data.</param>
    private void btnAdd_Click(object sender, EventArgs e)
    {
        string connectionString = this.txtConnectionString.Text;

        if (string.IsNullOrWhiteSpace(connectionString))
        {
            MessageBox.Show("No connection string given", "Add", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        //search the connection string in the child device list (null when manually entered connection string)
        ChildDevice? childDevice = _childDevices.FirstOrDefault(device => device.ConnectionString?.Equals(connectionString, StringComparison.InvariantCultureIgnoreCase) ?? false);

        //found that child device in grid and it is already used?
        if (childDevice?.IsUsed ?? false)
        {
            MessageBox.Show("Device already in use", "Add", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        if (ConnectAndAddDeviceToParent(connectionString))
        {
            childDevice?.SetUsed(); //mark as used if exists in grid
            this.gridChildDevices.Refresh();
        }
    }

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
        {
            _usedDevices.Clear();
            nodesCollection.Clear();
        }

        //create new tree node
        var deviceNode = nodesCollection.Add(device.Name);
        deviceNode.Tag = device;

        _usedDevices.Add(device);

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
    private void PopulateChildDevices()
    {
        bool isWaitCursorAlreadyOn = this.UseWaitCursor;

        if (!isWaitCursorAlreadyOn)
            GuiHelper.SetWaitCursor(this);

        _childDevices.Clear();
        this.gridChildDevices.Refresh(); //clear in GUI

        foreach (var deviceInfo in _instance.AvailableDevices)
        {
            AddChildDeviceToGrid(deviceInfo.Name, deviceInfo.ConnectionString);
        }

        if (!isWaitCursorAlreadyOn)
            GuiHelper.ResetWaitCursor(this);
    }

    /// <summary>
    /// Adds the child device information to the <c>DataGridView</c>.
    /// </summary>
    /// <param name="name">The device name.</param>
    /// <param name="connectionString">The connection string.</param>
    private void AddChildDeviceToGrid(string name, string connectionString)
    {
        bool isUsed = _usedDevices.Any(parentDevice => parentDevice.Info?.ConnectionString?.Equals(connectionString, StringComparison.InvariantCultureIgnoreCase) ?? false);

        _childDevices.Add(new ChildDevice(isUsed, name, connectionString));
    }

    /// <summary>
    /// Connects the device given by the <paramref name="connectionString"/> and adds it to the selected parent device in the tree.
    /// </summary>
    /// <param name="connectionString">The connection string.</param>
    /// <returns></returns>
    private bool ConnectAndAddDeviceToParent(string connectionString)
    {
        TreeNode? selectedParentDeviceNode = this.treeParentDevices.SelectedNode;
        Device?   selectedParentDevice     = selectedParentDeviceNode?.Tag as Device;

        if (selectedParentDevice == null)
        {
            MessageBox.Show("No parent device selected", "Add", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
        }

        //check if the device is already connected
        ChildDevice? childDevice = _childDevices.FirstOrDefault(device => device.ConnectionString?.Equals(connectionString, StringComparison.InvariantCultureIgnoreCase) ?? false);

        GuiHelper.SetWaitCursor(this);

        try
        {
            //connect the device
            Device newDevice = selectedParentDevice.AddDevice(connectionString);

            //add to tree
            TreeNode deviceNode = selectedParentDeviceNode!.Nodes.Add(newDevice.Name);
            deviceNode.Tag = newDevice;

            //remember for connection-string check
            _usedDevices.Add(newDevice);

            this.treeParentDevices.ExpandAll();
        }
        catch (Exception ex) //most probably an OpenDaqException
        {
            MessageBox.Show($"Error connecting:\n{ex.Message}", "Connection error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
        }
        finally
        {
            GuiHelper.ResetWaitCursor(this);
        }

        return true;
    }
}
