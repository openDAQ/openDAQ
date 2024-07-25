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

using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;

using Component = Daq.Core.OpenDAQ.Component;
using GlblRes = global::openDAQDemoNet.Properties.Resources;


namespace openDAQDemoNet;


public partial class frmMain : Form
{
    /// <summary>
    /// For TabStrip item identification (stored in Tag).
    /// </summary>
    private enum eTabstrip
    {
        SystemOverview,
        Signals,
        Channels,
        FunctionBlocks,
        FullTopology
    }

    private readonly BindingList<PropertyItem> _propertyItems = new();
    private Instance? _instance;

    public frmMain()
    {
        InitializeComponent();

        //ToDo:
        this.btnAddFunctionBlock.Enabled = false;

        //for easy selected-tab identification
        this.tabSystemOverview.Tag = eTabstrip.SystemOverview;
        this.tabSignals.Tag = eTabstrip.Signals;
        this.tabChannels.Tag = eTabstrip.Channels;
        this.tabFunctionBlocks.Tag = eTabstrip.FunctionBlocks;
        this.tabFullTopology.Tag = eTabstrip.FullTopology;

        this.treeComponents.HideSelection = false;

        this.treeComponents.Nodes.Clear();

        InitializeDataGridView(this.gridProperties);

        ImageList imageList = this.imglTreeImages;
        InitializeImageList(imageList);

        this.treeComponents.ImageList = this.imglTreeImages;
    }

    #region event handlers

    private void frmMain_Load(object sender, EventArgs e)
    {
        this.showHiddenComponentsToolStripMenuItem.Checked = false;
        this.componentsInsteadOfDirectObjectAccessToolStripMenuItem.Checked = true;

        _instance = OpenDAQFactory.Instance();

        if (_instance == null)
            throw new NullReferenceException("Unable to instantiate openDAQ");
    }

    private void frmMain_FormClosing(object sender, FormClosingEventArgs e)
    {
        this.treeComponents.Nodes.Clear();

        _propertyItems.Clear();

        _instance?.Dispose();
    }

    private void frmMain_Shown(object sender, EventArgs e)
    {
        SetWaitCursor();
        this.Update();

        //binding data late to not to "trash" GUI display beforehand
        this.gridProperties.DataSource = _propertyItems;
        this.gridProperties.Columns[nameof(PropertyItem.ReadOnlyImage)].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleCenter;

        UpdateTree();

        ResetWaitCursor();

        this.treeComponents.Select();
    }

    #region ToolStripMenu

    private void loadConfigurationToolStripMenuItem_Click(object sender, EventArgs e)
    {
        //ToDo: load config
    }

    private void saveConfigurationToolStripMenuItem_Click(object sender, EventArgs e)
    {
        //ToDo: save config
    }

    private void exitToolStripMenuItem_Click(object sender, EventArgs e)
    {
        this.Close();
    }

    private void showHiddenComponentsToolStripMenuItem_Click(object sender, EventArgs e)
    {
        UpdateTree();
    }

    private void componentsInsteadOfDirectObjectAccessToolStripMenuItem_Click(object sender, EventArgs e)
    {
        UpdateTree();
    }

    private void btnAddDevice_Click(object sender, EventArgs e)
    {
        if (_instance == null)
            return;

        SetWaitCursor();

        using (var frm = new frmAddDeviceDialog(_instance))
        {
            frm.ShowDialog(this);
            SetWaitCursor();
        }

        UpdateTree();

        ResetWaitCursor();
    }

    private void btnAddFunctionBlock_Click(object sender, EventArgs e)
    {
        //ToDo: add function block
    }

    private void btnRefresh_Click(object sender, EventArgs e)
    {
        UpdateTree();
    }

    #endregion ToolStripMenu

    private void tabControl1_Selected(object sender, TabControlEventArgs e)
    {
        UpdateTree();
    }

    #region treeComponents

    private void treeComponents_AfterSelect(object sender, TreeViewEventArgs e)
    {
        UpdateProperties(e.Node?.Tag as BaseObject);
    }

    private void treeComponents_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
    {
        //on right-click just select the clicked node as this is not done by default
        if (e.Button == MouseButtons.Right)
            this.treeComponents.SelectedNode = e.Node;
    }

    #region contextMenuStripTreeComponents

    private void contextMenuStripTreeComponents_Opening(object sender, CancelEventArgs e)
    {
        //init
        this.contextMenuStripMenuItemTreeRemove.Enabled = false;

        TreeNode? selectedNode = this.treeComponents.SelectedNode;

        if (selectedNode == null)
        {
            //no node selected
            e.Cancel = true;
            return;
        }

        //disable all menu items beforehand
        foreach (var item in this.contextMenuStripTreeComponents.Items.OfType<ToolStripMenuItem>())
            item.Enabled = false;

        bool useComponentApproach = this.componentsInsteadOfDirectObjectAccessToolStripMenuItem.Checked
                                    || ((eTabstrip)this.tabControl1.SelectedTab.Tag != eTabstrip.SystemOverview);

        //ContextMenu only for Device and FunctionBlock nodes (but not a Channel)
        BaseObject? baseObject = selectedNode.Tag as BaseObject;
        switch (baseObject)
        {
            case Component component when useComponentApproach:
                {
                    if ((component.CanCastTo<Device>() && !selectedNode.Equals(this.treeComponents.Nodes?[0]))
                        || (component.CanCastTo<FunctionBlock>() && !component.CanCastTo<Channel>()))
                    {
                        this.contextMenuStripMenuItemTreeRemove.Enabled = true;
                    }
                    else
                    {
                        e.Cancel = true;
                    }
                }
                break;

            //the following cases are meant for the object list approach of the system overview

            case Device:
            case FunctionBlock when !baseObject.CanCastTo<Channel>():
                this.contextMenuStripMenuItemTreeRemove.Enabled = true;
                break;

            //case Channel: //this is also a FunctionBlock
            //case Signal:
            default:
                e.Cancel = true;
                break;
        }
    }

    private void contextMenuStripMenuItemTreeRemove_Click(object sender, EventArgs e)
    {
        TreeNode? selectedNode = this.treeComponents.SelectedNode;

        if (selectedNode == null)
            return;

        //selectedNode.Tag is actually always a Component through inheritance
        Device parentDevice = GetParentDevice((Component)selectedNode.Tag);

        bool useComponentApproach = this.componentsInsteadOfDirectObjectAccessToolStripMenuItem.Checked
                                    || ((eTabstrip)this.tabControl1.SelectedTab.Tag != eTabstrip.SystemOverview);

        bool isRemoved = false;

        BaseObject? baseObject = selectedNode.Tag as BaseObject;

        switch (baseObject)
        {
            case Component component when useComponentApproach:
                {
                    if (component.Cast<Device>() is Device device)
                    {
                        parentDevice.RemoveDevice(device);
                        device.Dispose();
                        isRemoved = true;
                    }
                    else if (!component.CanCastTo<Channel>() && (component.Cast<FunctionBlock>() is FunctionBlock functionBlock))
                    {
                        parentDevice.RemoveFunctionBlock(functionBlock);
                        functionBlock.Dispose();
                        isRemoved = true;
                    }
                }
                break;

            //the following cases are meant for the object list approach of the system overview

            case Device device:
                parentDevice.RemoveDevice(device);
                device.Dispose();
                isRemoved = true;
                break;

            case FunctionBlock functionBlock when !baseObject.CanCastTo<Channel>():
                parentDevice.RemoveFunctionBlock(functionBlock);
                functionBlock.Dispose();
                isRemoved = true;
                break;
        }

        if (isRemoved)
        {
            selectedNode.Tag = null;
            this.treeComponents.SelectedNode = selectedNode.Parent;
            UpdateTree();
        }
    }

    #endregion contextMenuStripTreeComponents

    #endregion treeComponents

    #region listViewProperties

    private void gridProperties_CellDoubleClick(object sender, DataGridViewCellEventArgs e)
    {
        if (e.RowIndex < 0)
        {
            MessageBox.Show("No property selected", "Edit", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        PropertyItem propertyItem = _propertyItems[e.RowIndex];

        if (propertyItem.IsReadOnly)
        {
            MessageBox.Show("Property is read-only", "Edit", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        //ToDo: edit property
        MessageBox.Show($"Edit '{propertyItem.Name}' ({propertyItem.Value})", "Edit", MessageBoxButtons.OK, MessageBoxIcon.Question);
    }

    #endregion listViewProperties

    #endregion //event handlers .................................................................................

    /// <summary>
    /// Initializes the given <c>DataGridView</c>.
    /// </summary>
    /// <param name="grid">The <c>DataGridView</c> to initialize.</param>
    private static void InitializeDataGridView(DataGridView grid)
    {
        var columnHeadersDefaultCellStyle = grid.ColumnHeadersDefaultCellStyle;

        grid.EnableHeadersVisualStyles = false; //enable ColumnHeadersDefaultCellStyle
        columnHeadersDefaultCellStyle.BackColor = Color.FromKnownColor(KnownColor.ButtonFace);
        columnHeadersDefaultCellStyle.Font = new Font(grid.Font, FontStyle.Bold);
        columnHeadersDefaultCellStyle.SelectionBackColor = columnHeadersDefaultCellStyle.BackColor;
        columnHeadersDefaultCellStyle.SelectionForeColor = columnHeadersDefaultCellStyle.ForeColor;
        grid.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
        grid.AlternatingRowsDefaultCellStyle.BackColor = Color.FromArgb(0xFF, 0xF9, 0xF9, 0xF9);
        grid.GridColor = Color.FromArgb(0xFF, 0xE0, 0xE0, 0xE0);
        grid.DefaultCellStyle.SelectionBackColor = Color.White;
        grid.DefaultCellStyle.SelectionForeColor = Color.Black;

        grid.DefaultCellStyle.DataSourceNullValue = null;

        grid.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
        grid.MultiSelect = false;
        grid.RowHeadersVisible = false;
        grid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.AllCells;
        grid.AutoSizeRowsMode = DataGridViewAutoSizeRowsMode.DisplayedCellsExceptHeaders;
    }

    private static void InitializeImageList(ImageList imageList)
    {
        imageList.Images.Clear();
        imageList.ImageSize = new Size(24, 24);
        imageList.Images.Add(nameof(GlblRes.circle), (Bitmap)GlblRes.circle.Clone());
        imageList.Images.Add(nameof(GlblRes.device), (Bitmap)GlblRes.device.Clone());
        imageList.Images.Add(nameof(GlblRes.folder), (Bitmap)GlblRes.folder.Clone());
        imageList.Images.Add(nameof(GlblRes.function_block), (Bitmap)GlblRes.function_block.Clone());
        imageList.Images.Add(nameof(GlblRes.channel), (Bitmap)GlblRes.channel.Clone());
        imageList.Images.Add(nameof(GlblRes.signal), (Bitmap)GlblRes.signal.Clone());
    }

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
    /// Resets to the default cursor.
    /// </summary>
    private void ResetWaitCursor()
    {
        this.UseWaitCursor = false;
        this.Cursor = Cursors.Default;
        Cursor.Current = Cursors.Default;
        base.ResetCursor();
    }

    /// <summary>
    /// Updates the <c>TreeView</c>.
    /// </summary>
    private void UpdateTree()
    {
        string? selectedNodeName = this.treeComponents.SelectedNode?.Name;

        this.treeComponents.Nodes.Clear();

        if (this.componentsInsteadOfDirectObjectAccessToolStripMenuItem.Checked || ((eTabstrip)this.tabControl1.SelectedTab.Tag != eTabstrip.SystemOverview))
            TreeTraverseComponentsRecursive(_instance); //components approach
        else
            PopulateSystemOverviewTree(_instance!); //object lists approach

        this.treeComponents.ExpandAll();
        this.treeComponents.Update();

        //select stored node or first or none
        TreeNode? foundNode = null;
        if (!string.IsNullOrEmpty(selectedNodeName))
            foundNode = this.treeComponents.Nodes.Find(selectedNodeName, searchAllChildren: true).FirstOrDefault();
        if ((foundNode == null) && (this.treeComponents.Nodes.Count > 0))
            foundNode = this.treeComponents.Nodes[0];

        this.treeComponents.SelectedNode = foundNode;
    }

    #region Components approach (as in Python Demo)

    private void TreeTraverseComponentsRecursive(Component? component)
    {
        if (component == null)
            return;

        // tree view only in topology mode + parent exists
        //parent_id = '' if display_type not in (
        //    DisplayType.UNSPECIFIED, DisplayType.TOPOLOGY, DisplayType.SYSTEM_OVERVIEW, None) or component.parent is None else component.parent.global_id
        string parentId = component.Parent?.GlobalId ?? string.Empty;

        Folder? folder = component.Cast<Folder>();
        if ((folder == null) || (folder.GetItems().Count > 0))
        {
            switch ((eTabstrip)this.tabControl1.SelectedTab.Tag)
            {
                case eTabstrip.SystemOverview:
                    if (!(component.CanCastTo<InputPort>() || component.CanCastTo<Signal>())
                        && !(component.CanCastTo<Folder>() && (component.Name.Equals("IP") || component.Name.Equals("Sig"))))
                    {
                        TreeAddComponent(parentId, component);
                    }
                    break;

                case eTabstrip.Signals when component.Cast<Signal>() is Signal signal:
                    TreeAddComponent(parentId, signal);
                    break;

                case eTabstrip.Channels when component.Cast<Channel>() is Channel channel:
                    TreeAddComponent(parentId, channel);
                    break;

                case eTabstrip.FunctionBlocks when component.Cast<FunctionBlock>() is FunctionBlock functionBlock:
                    TreeAddComponent(parentId, functionBlock);
                    break;

                case eTabstrip.FullTopology:
                    TreeAddComponent(parentId, component);
                    break;

                default:
                    break;
            }
        }

        if (folder != null)
        {
            if (!(component.CanCastTo<FunctionBlock>() && ((eTabstrip)this.tabControl1.SelectedTab.Tag == eTabstrip.FunctionBlocks)))
            {
                foreach (var folderItem in folder.GetItems())
                    TreeTraverseComponentsRecursive(folderItem);
            }
        }
    }

    private void TreeAddComponent(string parentId, Component component)
    {
        string componentNodeId = component.GlobalId;
        string componentName = component.Name;
        string iconKey = nameof(GlblRes.circle);

        bool skip = !this.showHiddenComponentsToolStripMenuItem.Checked && !component.Visible;

        if (component.CanCastTo<Channel>())
            iconKey = nameof(GlblRes.channel);
        else if (component.CanCastTo<Signal>())
            iconKey = nameof(GlblRes.signal);
        else if (component.CanCastTo<FunctionBlock>())
            iconKey = nameof(GlblRes.function_block);
        else if (component.CanCastTo<InputPort>())
            iconKey = nameof(GlblRes.input_port);
        else if (component.CanCastTo<Device>())
            iconKey = nameof(GlblRes.device);
        else if (component.CanCastTo<Folder>())
        {
            iconKey = nameof(GlblRes.folder);

            if (componentName == "Sig")
                componentName = "Signals";
            else if (componentName == "FB")
                componentName = "Function blocks";
            else if (componentName == "Dev")
                componentName = "Devices";
            else if (componentName == "IP")
                componentName = "Input ports";
            else if (componentName == "IO")
                componentName = "Inputs/Outputs'";
        }
        else
        {
            //skipping unknown type components
            skip = true;
            System.Diagnostics.Debug.Print($"++++> unknown component {componentName} ({componentNodeId})");
        }

        if (!skip)
        {
            TreeNodeCollection parentNodesCollection = GetNodesCollection(parentId);
            var node = AddNode(parentNodesCollection, key: componentNodeId, componentName, iconKey);
            node.Tag = component;
        }
    }

    /// <summary>
    /// Gets the nodes collection for the given <paramref name="nodeId"/>.
    /// </summary>
    /// <param name="nodeId">The ID of the node to get the collection from (may be empty or <c>null</c> to use root node collection).</param>
    /// <returns>The nodes collection.</returns>
    private TreeNodeCollection GetNodesCollection(string nodeId)
    {
        TreeNodeCollection nodesCollection = this.treeComponents.Nodes;

        if (!string.IsNullOrEmpty(nodeId) && (nodesCollection.Count > 0))
        {
            //find the node
            TreeNode? foundNode = nodesCollection.Find(nodeId, searchAllChildren: true)?.FirstOrDefault();
            if (foundNode != null)
                nodesCollection = foundNode.Nodes;
        }

        return nodesCollection;
    }

    #endregion Components approach (as in Python Demo)

    #region Object lists approach

    private void PopulateSystemOverviewTree(Device device, TreeNode? rootNode = null)
    {
        /*
         * client
         * - function blocks
         * - devices
         *   - device
         *     - function blocks
         *     - devices
         *     - inputs/outputs
         * - inputs/outputs
         */

        //get the nodes collection to be populated (rootNode will be null for the first entry)
        TreeNodeCollection nodesCollection = rootNode?.Nodes ?? this.treeComponents.Nodes;

        //first call of recursion?
        if (rootNode == null)
            nodesCollection.Clear();

        //create new tree node
        var deviceNode = AddNode(nodesCollection, key: device.GlobalId, device.Name, imageKey: nameof(GlblRes.device));
        deviceNode.Tag = device;

        IListObject<FunctionBlock> functionBlocks = device.GetFunctionBlocks();

        if (functionBlocks.Count > 0)
        {
            var inputsOutputsRootNode = AddNode(deviceNode.Nodes, key: device.GlobalId + "/FB", "Function blocks", nameof(GlblRes.folder));

            //PopulateFunctionBlocksInSystemOverviewTree(deviceNode.Nodes, functionBlocks);
        }

        IListObject<Device> childDevices = device.GetDevices();

        //has child devices?
        if (childDevices?.Count > 0)
        {
            var devicesRootNode = AddNode(deviceNode.Nodes, key: device.GlobalId + "/Dev", "Devices", nameof(GlblRes.folder));

            //recursion
            foreach (var childDevice in childDevices)
                PopulateSystemOverviewTree(childDevice, devicesRootNode);
        }

        //there are either signals OR channels with signals
        IListObject<Signal> signals = device.GetSignals();
        IListObject<Channel> channels = device.GetChannels();

        if ((signals.Count > 0) || (channels.Count > 0))
        {
            var inputsOutputsRootNode = AddNode(deviceNode.Nodes, key: device.GlobalId + "/IO", "Inputs/Outputs", nameof(GlblRes.folder));

            PopulateSignalsInSystemOverviewTree(inputsOutputsRootNode.Nodes, signals);
            PopulateChannelsInSystemOverviewTree(inputsOutputsRootNode.Nodes, channels);
        }

        //first call of recursion?
        if (rootNode == null)
        {
            this.treeComponents.ExpandAll();
            this.treeComponents.Update();
            this.treeComponents.SelectedNode = deviceNode;
        }
    }

    private void PopulateSignalsInSystemOverviewTree(TreeNodeCollection nodes, IListObject<Signal> signals)
    {
        foreach (var signal in signals)
        {
            var node = AddNode(nodes, key: signal.GlobalId, signal.Name, imageKey: nameof(GlblRes.signal));
            node.Tag = signal;
        }
    }

    private void PopulateChannelsInSystemOverviewTree(TreeNodeCollection nodes, IListObject<Channel> channels)
    {
        foreach (var channel in channels)
        {
            var node = AddNode(nodes, key: channel.GlobalId, channel.Name, imageKey: nameof(GlblRes.channel));
            node.Tag = channel;

            PopulateSignalsInSystemOverviewTree(node.Nodes, channel.GetSignals());
        }
    }

    #endregion Object lists approach

    /// <summary>
    /// Adds a new node to the <c>TreeNodeCollection</c>.
    /// </summary>
    /// <param name="nodesCollection">The nodes collection.</param>
    /// <param name="key">The key.</param>
    /// <param name="text">The text.</param>
    /// <param name="imageKey">The image key.</param>
    /// <returns>The newly created <c>TreeNode</c>.</returns>
    private TreeNode AddNode(TreeNodeCollection nodesCollection, string? key, string text, string imageKey)
    {
        return nodesCollection.Add(key, text, imageKey, imageKey);
    }

    /// <summary>
    /// Gets the parent <c>Device</c> component for the given child <c>Component</c>.
    /// </summary>
    /// <param name="childComponent">The child component.</param>
    /// <returns>The <c>Device</c> or the instance when no parent <c>Device</c> found.</returns>
    private Device GetParentDevice(Component? childComponent)
    {
        childComponent = childComponent?.Parent;

        while (childComponent != null)
        {
            if (childComponent.CanCastTo<Device>())
                return childComponent.Cast<Device>();

            childComponent = childComponent.Parent;
        }

        return _instance!;
    }

    /// <summary>
    /// Updates the property grid.
    /// </summary>
    /// <param name="baseObject">The openDAQ <c>BaseObject</c> to get the properties from.</param>
    private void UpdateProperties(BaseObject? baseObject)
    {
        _propertyItems.Clear();

        if (baseObject == null)
            return;

        bool useComponentApproach = this.componentsInsteadOfDirectObjectAccessToolStripMenuItem.Checked
                                    || ((eTabstrip)this.tabControl1.SelectedTab.Tag != eTabstrip.SystemOverview);

        switch (baseObject)
        {
            case Component component when useComponentApproach:
                ListProperties(component);
                break;

            //the following cases are meant for the object list approach of the system overview

            case Device device:
                ListProperties(device);
                break;

            case Channel channel: //this is also a FunctionBlock
                ListProperties(channel);
                break;

            case Signal signal:
                ListProperties(signal);
                break;

            case FunctionBlock functionBlock:
                ListProperties(functionBlock);
                break;
        }
    }

    /// <summary>
    /// Lists the properties.
    /// </summary>
    /// <param name="propertyObject">The property object.</param>
    private void ListProperties(PropertyObject propertyObject)
    {
        var properties = propertyObject.AllProperties;

        foreach (var property in properties)
        {
            string propertyName = property.Name;
            CoreType propertyType = property.ValueType;
            BaseObject propertyValueObject = propertyObject.GetPropertyValue(propertyName);
            BaseObject propertyValue = CoreTypesFactory.GetPropertyValueObject(propertyValueObject, propertyType);

            _propertyItems.Add(new(property.ReadOnly, propertyName, propertyValue.ToString()));
        }
    }
}
