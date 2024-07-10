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
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using Daq.Core.OpenDAQ;
using Daq.Core.Types;


namespace openDAQDemoNet
{
    public partial class frmAddDeviceDialog : Form
    {
        public class ChildDevice
        {
            public ChildDevice(bool used, string name, string connectionString)
            {
                this.Used = used;
                this.Name = name;
                this.ConnectionString = connectionString;
            }

            /// <summary>
            /// Gets a value indicating whether this <see cref="ChildDevice"/> is used.
            /// </summary>
            /// <value>
            ///   <c>true</c> if used; otherwise, <c>false</c>.
            /// </value>
            [DisplayName("Used")]
            public bool Used { get; private set; }

            /// <summary>
            /// Gets the device name.
            /// </summary>
            [DisplayName("Name")]
            public string Name { get; private set; }

            /// <summary>
            /// Gets the connection string.
            /// </summary>
            [DisplayName("Connection string")]
            public string ConnectionString { get; private set; }

            /// <summary>
            /// Sets the used flag (private property).
            /// </summary>
            internal void SetUsed()
            {
                this.Used = true;
            }
        }

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

            this.gridChildDevices.ColumnHeadersDefaultCellStyle.Font = new Font(this.gridChildDevices.Font, FontStyle.Bold);
            this.gridChildDevices.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            //this.gridChildDevices.RowsDefaultCellStyle.BackColor            = Color.Gray;
            this.gridChildDevices.AlternatingRowsDefaultCellStyle.BackColor = Color.FromArgb(0xFF, 0xF0, 0xF0, 0xF0);
            this.gridChildDevices.GridColor = Color.FromArgb(0xFF, 0xE0, 0xE0, 0xE0);

            this.gridChildDevices.DataSource = _childDevices;

            this.gridChildDevices.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
            this.gridChildDevices.MultiSelect = false;
            this.gridChildDevices.RowHeadersVisible = false;
            this.gridChildDevices.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.AllCells;
            this.gridChildDevices.AutoSizeRowsMode = DataGridViewAutoSizeRowsMode.DisplayedCellsExceptHeaders;

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
            SetWaitCursor();
            this.Update();

            PopulateParentDevices(_instance);
            PopulateChildDevices();

            ResetWaitCursor();
        }

        /// <summary>
        /// Handles the Click event of the <c>refreshToolStripMenuItem</c> control.<br/>
        /// It re-populates the <c>DataGridView</c> by retrieving the device information (scan).
        /// </summary>
        /// <param name="sender">The source of the event.</param>
        /// <param name="e">The <see cref="EventArgs"/> instance containing the event data.</param>
        private void refreshToolStripMenuItem_Click(object sender, EventArgs e)
        {
            PopulateChildDevices();
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
                MessageBox.Show("No child device selected");
                return;
            }

            ChildDevice childDevice = _childDevices[e.RowIndex];

            if (childDevice.Used)
            {
                MessageBox.Show("Device already in use");
                return;
            }

            string connectionString = childDevice.ConnectionString;

            if (ConnectAndAddDeviceToParent(connectionString))
            {
                childDevice.SetUsed();
                this.gridChildDevices.Refresh(); //Update() does not paint the check-mark somehow
            }
        }

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
                MessageBox.Show("No connection string given");
                return;
            }

            //search the connection string in the child device list
            ChildDevice? childDevice = _childDevices.FirstOrDefault(device => device.ConnectionString?.Equals(connectionString, StringComparison.InvariantCultureIgnoreCase) ?? false);

            //found that child device in grid and it is already used?
            if (childDevice?.Used ?? false)
            {
                MessageBox.Show("Device already in use");
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
        /// Sets the wait cursor.
        /// </summary>
        private void SetWaitCursor()
        {
            Cursor.Current = Cursors.WaitCursor;
            this.Cursor = Cursors.WaitCursor;
            this.UseWaitCursor = true;
        }

        /// <summary>
        /// Resets the wait cursor.
        /// </summary>
        private void ResetWaitCursor()
        {
            this.UseWaitCursor = false;
            this.Cursor = Cursors.Default;
            Cursor.Current = Cursors.Default;
            base.ResetCursor();
        }

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
            this.gridChildDevices.DataSource = null;
            this.gridChildDevices.Refresh();

            SetWaitCursor();

            _childDevices.Clear();

            foreach (var deviceInfo in _instance.AvailableDevices)
            {
                AddChildDeviceToGrid(deviceInfo.Name, deviceInfo.ConnectionString);
            }

            this.gridChildDevices.DataSource = _childDevices;

            ResetWaitCursor();
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
                MessageBox.Show("No parent device selected");
                return false;
            }

            //check if the device is already connected
            ChildDevice? childDevice = _childDevices.FirstOrDefault(device => device.ConnectionString?.Equals(connectionString, StringComparison.InvariantCultureIgnoreCase) ?? false);

            SetWaitCursor();

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
                ResetWaitCursor();
            }

            return true;
        }
    }
}
