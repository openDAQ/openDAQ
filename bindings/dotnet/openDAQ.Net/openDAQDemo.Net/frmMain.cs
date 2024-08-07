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

    private const bool LOCKED = true;
    private const bool FREE   = false;

    private readonly BindingList<PropertyItem>  _propertyItems  = new();
    private readonly BindingList<AttributeItem> _attributeItems = new();
    private Instance? _instance;

    private Component? _selectedComponent;


    /// <summary>
    /// Initializes a new instance of the <see cref="frmMain"/> class.
    /// </summary>
    public frmMain()
    {
        InitializeComponent();

        //for easy selected-tab identification
        this.tabSystemOverview.Tag = eTabstrip.SystemOverview;
        this.tabSignals.Tag        = eTabstrip.Signals;
        this.tabChannels.Tag       = eTabstrip.Channels;
        this.tabFunctionBlocks.Tag = eTabstrip.FunctionBlocks;
        this.tabFullTopology.Tag   = eTabstrip.FullTopology;

        ImageList imageList = this.imglTreeImages;
        InitializeImageList(imageList);

        this.treeComponents.HideSelection = false;
        this.treeComponents.ImageList     = this.imglTreeImages;
        this.treeComponents.Nodes.Clear();

        InitializeDataGridView(this.gridProperties);
        InitializeDataGridView(this.gridAttributes);

    }

    #region event handlers

    private void frmMain_Load(object sender, EventArgs e)
    {
        this.showHiddenComponentsToolStripMenuItem.Checked                  = false;
        this.componentsInsteadOfDirectObjectAccessToolStripMenuItem.Checked = true;

        _instance = OpenDAQFactory.Instance();

        if (_instance == null)
            throw new NullReferenceException("Unable to instantiate openDAQ");
    }

    private void frmMain_FormClosing(object sender, FormClosingEventArgs e)
    {
        this.treeComponents.Nodes.Clear();

        _propertyItems.Clear();
        _attributeItems.Clear();

        _instance?.Dispose();
    }

    private void frmMain_Shown(object sender, EventArgs e)
    {
        SetWaitCursor();
        this.Update();

        //binding data late to not to "trash" GUI display beforehand
        this.gridProperties.DataSource = _propertyItems;
        this.gridProperties.Columns[nameof(PropertyItem.LockedImage)].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleCenter;
        this.gridProperties.Refresh();
        this.gridAttributes.DataSource = _attributeItems;
        this.gridAttributes.Columns[nameof(PropertyItem.LockedImage)].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleCenter;
        this.gridAttributes.Refresh();

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
        if (_instance == null)
            return;

        SetWaitCursor();

        using (var frm = new frmAddFunctionBlockDialog(_instance))
        {
            frm.ShowDialog(this);
            SetWaitCursor();
        }

        UpdateTree();

        ResetWaitCursor();
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
        SetWaitCursor();

        try
        {
            _selectedComponent = e.Node?.Tag as Component;

            UpdateProperties(_selectedComponent);
            UpdateAttributes(_selectedComponent);
        }
        finally
        {
            ResetWaitCursor();
        }
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
        this.contextMenuItemTreeComponentsRemove.Enabled = false;

        TreeView  tree         = this.treeComponents;
        TreeNode? selectedNode = tree.SelectedNode;

        if (selectedNode == null)
        {
            //no node selected
            e.Cancel = true;
            return;
        }

        var info = tree.HitTest(tree.PointToClient(Cursor.Position));
        if (info.Node == null)
        {
            //no node clicked
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
                    if ((component.CanCastTo<Device>() && !selectedNode.Equals(tree.Nodes?[0]))
                        || (component.CanCastTo<FunctionBlock>() && !component.CanCastTo<Channel>()))
                    {
                        this.contextMenuItemTreeComponentsRemove.Enabled = true;
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
                this.contextMenuItemTreeComponentsRemove.Enabled = true;
                break;

            //case Channel: //this is also a FunctionBlock
            //case Signal:
            default:
                e.Cancel = true;
                break;
        }
    }

    private void contextMenuItemTreeComponentsRemove_Click(object sender, EventArgs e)
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

    #region gridProperties

    private void gridProperties_CellDoubleClick(object sender, DataGridViewCellEventArgs e)
    {
        if (e.RowIndex < 0)
        {
            MessageBox.Show("No property selected", "Edit", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        var propertyItem = (PropertyItem)((DataGridView)sender).Rows[e.RowIndex].DataBoundItem;

        if (propertyItem.IsLocked)
        {
            MessageBox.Show("Property is locked", $"Edit property \"{propertyItem.Name}\"", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return;
        }

        EditSelectedProperty();
    }

    #region conetxtMenuItemGridPropertiesEdit

    private void gridProperties_CellContextMenuStripNeeded(object sender, DataGridViewCellContextMenuStripNeededEventArgs e)
    {
        if ((e.RowIndex < 0) || (e.ColumnIndex < 0))
            return;

        DataGridView dataGridView = ((DataGridView)sender);

        //select the clicked cell/row for context menu (which opens automatically after this)
        dataGridView.CurrentCell = dataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex];
    }

    private void contextMenuStripGridProperties_Opening(object sender, CancelEventArgs e)
    {
        //init
        this.conetxtMenuItemGridPropertiesEdit.Enabled = false;

        var grid = this.gridProperties;

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

        var selectedProperty = (PropertyItem)grid.SelectedRows[0].DataBoundItem;

        this.conetxtMenuItemGridPropertiesEdit.Enabled = !selectedProperty.IsLocked;
    }

    private void conetxtMenuItemGridPropertiesEdit_Click(object sender, EventArgs e)
    {
        EditSelectedProperty();
    }

    #endregion conetxtMenuItemGridPropertiesEdit

    #endregion gridProperties

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

    #region conetxtMenuItemGridPropertiesEdit

    private void gridAttributes_CellContextMenuStripNeeded(object sender, DataGridViewCellContextMenuStripNeededEventArgs e)
    {
        if ((e.RowIndex < 0) || (e.ColumnIndex < 0))
            return;

        DataGridView dataGridView = ((DataGridView)sender);

        //select the clicked cell/row for context menu (which opens automatically after this)
        dataGridView.CurrentCell = dataGridView.Rows[e.RowIndex].Cells[e.ColumnIndex];
    }

    //private void contextMenuStripGridAttributes_Opening(object sender, CancelEventArgs e)
    //{
    //    //init
    //    this.conetxtMenuItemGridAttributesEdit.Enabled = false;

    //    var grid = this.gridAttributes;

    //    if (grid.SelectedRows.Count == 0)
    //    {
    //        e.Cancel = true;
    //        return;
    //    }

    //    Point cursor = grid.PointToClient(Cursor.Position);
    //    var info = grid.HitTest(cursor.X, cursor.Y);
    //    if (info.Type != DataGridViewHitTestType.Cell)
    //    {
    //        //no node clicked
    //        e.Cancel = true;
    //        return;
    //    }

    //    var selectedProperty = (PropertyItem)grid.SelectedRows[0].DataBoundItem;

    //    this.conetxtMenuItemGridAttributesEdit.Enabled = !selectedProperty.IsReadOnly;
    //}

    //private void conetxtMenuItemGridAttributesEdit_Click(object sender, EventArgs e)
    //{
    //    EditSelectedAttribute();
    //}

    #endregion conetxtMenuItemGridPropertiesEdit

    #endregion gridProperties

    #endregion //event handlers .................................................................................

    #region Common GUI methods

    /// <summary>
    /// Initializes the given <c>DataGridView</c>.
    /// </summary>
    /// <param name="grid">The <c>DataGridView</c> to initialize.</param>
    private static void InitializeDataGridView(DataGridView grid)
    {
        var columnHeadersDefaultCellStyle = grid.ColumnHeadersDefaultCellStyle;

        grid.EnableHeadersVisualStyles                   = false; //enable ColumnHeadersDefaultCellStyle
        columnHeadersDefaultCellStyle.BackColor          = Color.FromKnownColor(KnownColor.ButtonFace);
        columnHeadersDefaultCellStyle.Font               = new Font(grid.Font, FontStyle.Bold);
        columnHeadersDefaultCellStyle.SelectionBackColor = Color.Transparent;
        columnHeadersDefaultCellStyle.SelectionForeColor = Color.Transparent;
        columnHeadersDefaultCellStyle.WrapMode           = DataGridViewTriState.False;
        grid.ColumnHeadersHeightSizeMode                 = DataGridViewColumnHeadersHeightSizeMode.AutoSize;
        grid.AlternatingRowsDefaultCellStyle.BackColor   = Color.FromArgb(0xFF, 0xF9, 0xF9, 0xF9);
        grid.GridColor                                   = Color.FromArgb(0xFF, 0xE0, 0xE0, 0xE0);
        grid.DefaultCellStyle.SelectionBackColor         = Color.Transparent;
        grid.DefaultCellStyle.SelectionForeColor         = Color.Transparent;

        grid.DefaultCellStyle.DataSourceNullValue = null;

        grid.SelectionMode       = DataGridViewSelectionMode.FullRowSelect;
        grid.MultiSelect         = false;
        grid.RowHeadersVisible   = false;
        grid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.AllCells;
        grid.AutoSizeRowsMode    = DataGridViewAutoSizeRowsMode.DisplayedCellsExceptHeaders;
        grid.ShowCellToolTips    = false;
    }

    /// <summary>
    /// Initializes the image list (designer would remove transparency over time).
    /// </summary>
    /// <param name="imageList">The image list.</param>
    private static void InitializeImageList(ImageList imageList)
    {
        imageList.Images.Clear();
        imageList.ImageSize = new Size(24, 24);
        imageList.Images.Add(nameof(GlblRes.circle),         (Bitmap)GlblRes.circle.Clone());
        imageList.Images.Add(nameof(GlblRes.device),         (Bitmap)GlblRes.device.Clone());
        imageList.Images.Add(nameof(GlblRes.folder),         (Bitmap)GlblRes.folder.Clone());
        imageList.Images.Add(nameof(GlblRes.function_block), (Bitmap)GlblRes.function_block.Clone());
        imageList.Images.Add(nameof(GlblRes.channel),        (Bitmap)GlblRes.channel.Clone());
        imageList.Images.Add(nameof(GlblRes.signal),         (Bitmap)GlblRes.signal.Clone());
    }

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

    #region Tree view

    /// <summary>
    /// Updates the <c>TreeView</c>.
    /// </summary>
    private void UpdateTree()
    {
        SetWaitCursor();

        string? selectedNodeName = this.treeComponents.SelectedNode?.Name;

        this.treeComponents.Nodes.Clear();
        _propertyItems.Clear();
        _attributeItems.Clear();

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

        ResetWaitCursor();
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

                case eTabstrip.FunctionBlocks when !component.CanCastTo<Channel>()
                                                   && component.Cast<FunctionBlock>() is FunctionBlock functionBlock:
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
        string componentName   = component.Name;
        string iconKey         = nameof(GlblRes.circle);

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
            var parentNodesCollection = GetNodesCollection(parentId);
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

//ToDo:            PopulateFunctionBlocksInSystemOverviewTree(deviceNode.Nodes, functionBlocks);
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

    #endregion Tree view

    #region Properties view

    /// <summary>
    /// Updates the property grid.
    /// </summary>
    /// <param name="component">The openDAQ <c>Component</c> to get the properties from.</param>
    private void UpdateProperties(Component? component)
    {
        _propertyItems.Clear();

        if (component == null)
            return;

        bool useComponentApproach = this.componentsInsteadOfDirectObjectAccessToolStripMenuItem.Checked
                                    || ((eTabstrip)this.tabControl1.SelectedTab.Tag != eTabstrip.SystemOverview);

        switch (component)
        {
            case Component when useComponentApproach:
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

        this.gridProperties.ClearSelection();
        this.gridProperties.AutoResizeColumns();
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
            if (!property.Visible)
                continue;

            var propertyName            = property.Name;
            var propertyType            = property.ValueType;
            var propertyValueObject     = propertyObject.GetPropertyValue(propertyName);
            var propertyValue           = CoreTypesFactory.GetPropertyValueObject(propertyValueObject, propertyType);
            var propertySelectionValues = property.SelectionValues;

            if (propertySelectionValues != null)
            {
                if (propertySelectionValues.CanCastTo<ListObject<StringObject>>())
                {
                    IList<StringObject> listObject = propertySelectionValues.Cast<ListObject<StringObject>>();
                    propertyValue = listObject[(int)propertyValue];
                }
                else if (propertySelectionValues.CanCastTo<DictObject<IntegerObject, StringObject>>())
                {
                    IDictionary<IntegerObject, StringObject> listObject = propertySelectionValues.Cast<DictObject<IntegerObject, StringObject>>();
                    propertyValue = listObject[(int)propertyValue];
                }
                else
                {
                    propertyValue = "( unknown selection-value type )";
                }
            }

            _propertyItems.Add(new(property.ReadOnly, propertyName, propertyValue.ToString(), property.Unit, property.Description));
        }
    }

    /// <summary>
    /// Open edit-dialog for the selected property (property grid).
    /// </summary>
    private void EditSelectedProperty()
    {
        var selectedComponent    = (Component)this.treeComponents.SelectedNode.Tag;
        var selectedPropertyItem = (PropertyItem)this.gridProperties.SelectedRows[0].DataBoundItem;

        var propertyName            = selectedPropertyItem.Name;
        var property                = selectedComponent.GetProperty(propertyName);
        var propertyValueObject     = selectedComponent.GetPropertyValue(propertyName);
        var propertyValue           = CoreTypesFactory.GetPropertyValueObject(propertyValueObject, property.ValueType);
        var propertySelectionValues = property.SelectionValues;

        var oldPropertyValue = propertyValue;

        string askDialogTitle = $"Edit property \"{propertyName}\"";

        switch (property.ValueType)
        {
            case CoreType.ctBool:
                propertyValue = !propertyValue; //implicit cast BaseObject to and from bool
                break;
            case CoreType.ctInt:
                if (propertySelectionValues != null)
                {
                    if (propertySelectionValues.CanCastTo<ListObject<StringObject>>())
                    {
                        propertyValue = frmInputDialog.AskList(this,
                                                               askDialogTitle,
                                                               "Please select the new value below",
                                                               propertyValue,
                                                               propertySelectionValues.Cast<ListObject<StringObject>>());
                    }
                    else if (propertySelectionValues.CanCastTo<DictObject<IntegerObject, StringObject>>())
                    {
                        propertyValue = frmInputDialog.AskDict(this,
                                                               askDialogTitle,
                                                               "Please select the new value below",
                                                               propertyValue,
                                                               propertySelectionValues.Cast<DictObject<IntegerObject, StringObject>>());
                    }
                    else
                    {
                        MessageBox.Show($"Sorry, opeanDAQ property selection-values of unknown type.",
                                        askDialogTitle, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    }
                }
                else
                {
                    long? min = null; if (property.MinValue != null) min = property.MinValue;
                    long? max = null; if (property.MaxValue != null) max = property.MaxValue;
                    propertyValue = frmInputDialog.AskInteger(this,
                                                              askDialogTitle,
                                                              "Please enter the new value below",
                                                              propertyValue,
                                                              min,
                                                              max);
                }
                break;
            case CoreType.ctFloat:
                {
                    double? min = null; if (property.MinValue != null) min = property.MinValue;
                    double? max = null; if (property.MaxValue != null) max = property.MaxValue;
                    propertyValue = frmInputDialog.AskFloat(this,
                                                            askDialogTitle,
                                                            "Please enter the new value below",
                                                            propertyValue,
                                                            min,
                                                            max);
                }
                break;
            case CoreType.ctString:
                propertyValue = frmInputDialog.AskString(this,
                                                         askDialogTitle,
                                                         "Please enter the new value below",
                                                         propertyValue);
                break;
            case CoreType.ctList:
                propertyValue = frmInputDialog.AskList(this,
                                                       askDialogTitle,
                                                       "Please select the new value below",
                                                       propertyValue,
                                                       propertySelectionValues.Cast<ListObject<StringObject>>());
                break;
            case CoreType.ctDict:
                propertyValue = frmInputDialog.AskDict(this,
                                                       askDialogTitle,
                                                       "Please select the new value below",
                                                       propertyValue,
                                                       propertySelectionValues.Cast<DictObject<IntegerObject, StringObject>>());
                break;
//            case CoreType.ctRatio:
//                break;
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
//            case CoreType.ctEnumeration:
//                break;
            //case CoreType.ctUndefined:
            //    break;
            default:
                MessageBox.Show($"Sorry, opeanDAQ value type '{property.ValueType}' is not editable here.",
                                askDialogTitle, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                break;
        }

        if (propertyValue != oldPropertyValue)
        {
            try
            {
                selectedComponent.SetPropertyValue(propertyName, propertyValue);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Sorry, something went wrong:\n{ex.GetType().Name} - {ex.Message}",
                                askDialogTitle, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);

                //throws exception in native code
                //selectedComponent.SetPropertyValue(propertyName, oldPropertyValue);
            }

            UpdateProperties(selectedComponent);
        }
    }

    #endregion Properties view

    #region Attributes view

    /// <summary>
    /// Updates the attributes grid.
    /// </summary>
    /// <param name="component">The openDAQ <c>Component</c> to get the attributes from.</param>
    private void UpdateAttributes(Component? component)
    {
        _attributeItems.Clear();

        if (component == null)
            return;

        bool useComponentApproach = this.componentsInsteadOfDirectObjectAccessToolStripMenuItem.Checked
                                    || ((eTabstrip)this.tabControl1.SelectedTab.Tag != eTabstrip.SystemOverview);

        ListAttributes(component.Cast<Component>()); //cast to interface so that only Component attributes are visible

        if (component.Cast<Device>() is Device device)
            ListAttributes(device);
        else if (component.Cast<Signal>() is Signal signal)
            ListAttributes(signal);
        else if (component.Cast<InputPort>() is InputPort inputPort)
            ListAttributes(inputPort);
        else if (component.Cast<FunctionBlock>() is FunctionBlock functionBlock)
            ListAttributes(functionBlock);

        this.gridAttributes.ClearSelection();
        this.gridAttributes.AutoResizeColumns();
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.Component"/> attributes.
    /// </summary>
    /// <param name="component">The object with the attributes.</param>
    private void ListAttributes(Component component)
    {
       _attributeItems.Add(new(FREE,   "Name",        "Name",        component.Name,               CoreType.ctString, component));
       _attributeItems.Add(new(FREE,   "Description", "Description", component.Description,        CoreType.ctString, component));
       _attributeItems.Add(new(FREE,   "Active",      "Active",      component.Active.ToString(),  CoreType.ctBool,   component));
       _attributeItems.Add(new(LOCKED, "GlobalId",    "Global ID",   component.GlobalId,           CoreType.ctString, component));
       _attributeItems.Add(new(LOCKED, "LocalId",     "Local ID",    component.LocalId,            CoreType.ctString, component));
       _attributeItems.Add(new(FREE,   "Tags",        "Tags",        component.Tags,               CoreType.ctObject, component));
       _attributeItems.Add(new(LOCKED, "Visible",     "Visible",     component.Visible.ToString(), CoreType.ctBool,   component));
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.Device"/> attributes.
    /// </summary>
    /// <param name="device">The object with the attributes.</param>
    private void ListAttributes(Device device)
    {
        //_attributeItems.Add(new(FREE, "", "", device., CoreType., device));
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.Signal"/> attributes.
    /// </summary>
    /// <param name="signal">The object with the attributes.</param>
    private void ListAttributes(Signal signal)
    {
        string relatedSignalIds = string.Join(", ", signal.RelatedSignals?.Select(sig => sig.GlobalId) ?? Array.Empty<string>());

       _attributeItems.Add(new(FREE,   "Public",               "Public",              signal.Public.ToString(),     CoreType.ctBool,      signal));
       _attributeItems.Add(new(LOCKED, "DomainSignalGlobalId", "Domain Signal ID",    signal.DomainSignal.GlobalId, CoreType.ctString,    signal));
       _attributeItems.Add(new(LOCKED, "RelatedSignalsIDs",    "Related Signals IDs", relatedSignalIds,             CoreType.ctList,      signal));
       _attributeItems.Add(new(LOCKED, "Streamed",             "Streamed",            signal.Streamed.ToString(),   CoreType.ctBool,      signal));
       _attributeItems.Add(new(LOCKED, "LastValue",            "Last Value",          GetValue(signal.LastValue),   CoreType.ctUndefined, signal));
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.InputPort"/> attributes.
    /// </summary>
    /// <param name="inputPort">The object with the attributes.</param>
    private void ListAttributes(InputPort inputPort)
    {
       _attributeItems.Add(new(LOCKED, "SignalGlobalId", "Signal ID",       inputPort.Signal?.GlobalId,          CoreType.ctString, inputPort));
       _attributeItems.Add(new(LOCKED, "RequiresSignal", "Requires Signal", inputPort.RequiresSignal.ToString(), CoreType.ctBool,   inputPort));
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.FunctionBlock"/> attributes.
    /// </summary>
    /// <param name="functionBlock">The object with the attributes.</param>
    private void ListAttributes(FunctionBlock functionBlock)
    {
        //_attributeItems.Add(new(LOCKED, "", "", functionBlock., CoreType., functionBlock));
    }

    /// <summary>
    /// Open edit-dialog for the selected attribute (attribute grid).
    /// </summary>
    private void EditSelectedAttribute()
    {
        var selectedAttributeItem   = (AttributeItem)this.gridAttributes.SelectedRows[0].DataBoundItem;
        var selectedAttributeName   = selectedAttributeItem.Name;
        var selectedAttributeValue  = selectedAttributeItem.Value;
        var selectedAttributeObject = selectedAttributeItem.OpenDaqObject;

        string stringValue = string.Empty;
        long   intValue    = long.MinValue;
        double floatValue  = double.NaN;

        string askDialogTitle       = $"Edit attribute \"{selectedAttributeItem.DisplayName}\"";
        string askDialogCaption     = "Please enter the new value below";
        string askDialogCaptionList = "Please select the new value below";

        //ask for the new attribute value (just return when unchanged)
        switch (selectedAttributeItem.ValueType)
        {
            case CoreType.ctBool:
                //will just toggle
                break;

            case CoreType.ctInt:
                intValue = frmInputDialog.AskInteger(this, askDialogTitle, askDialogCaption, 0);
                if (intValue.ToString() == selectedAttributeValue)
                    return;
                break;

            case CoreType.ctFloat:
                floatValue = frmInputDialog.AskFloat(this, askDialogTitle, askDialogCaption, 0);
                if (floatValue.ToString() == selectedAttributeValue)
                    return;
                break;

            case CoreType.ctString:
                stringValue = frmInputDialog.AskString(this, askDialogTitle, askDialogCaption, selectedAttributeItem.Value);
                if (stringValue == selectedAttributeValue)
                    return;
                break;

            //case CoreType.ctList:
            //    intValue = frmInputDialog.AskList(this, askDialogTitle, askDialogCaptionList, 0, null);
            //    if (intValue == oldIndex)
            //        return;
            //    break;

            //case CoreType.ctDict:
            //    intValue = frmInputDialog.AskDict(this, askDialogTitle, askDialogCaptionList, 0, null);
            //    if (intValue == oldIndex)
            //        return;
            //    break;

            //case CoreType.ctObject:
            //    //ToDo: edit tags (list of strings
            //    break;

            //case CoreType.ctRatio:
            //    break;
            //case CoreType.ctProc:
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
            default:
                MessageBox.Show($"Sorry, opeanDAQ value type '{selectedAttributeItem.ValueType}' is not editable here.",
                                askDialogTitle, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
        }

        //set the new attribute value
        switch (selectedAttributeObject)
        {
            case Device device:
                //nothing
                break;

            case Signal signal:
                switch (selectedAttributeName)
                {
                    case "Public":
                        signal.Public ^= true; //toggle
                        break;
                }
                break;

            case InputPort inputPort:
                //nothing
                break;

            case FunctionBlock functionBlock:
                //nothing
                break;

            case Component component: //inherited by all of the above so do this last
                switch (selectedAttributeName)
                {
                    case "Name":
                        component.Name = stringValue;
                        break;

                    case "Description":
                        component.Description = stringValue;
                        break;

                    case "Active":
                        component.Active ^= true; //toggle
                        break;

                    case "Tags":
                        //ToDo: tags
                        //component.Tags
                        break;
                }
                break;
        }

        UpdateAttributes((Component)selectedAttributeObject);
    }


    #endregion Attributes view

    /// <summary>
    /// Gets the value of the given <see cref="BaseObject"/>.
    /// </summary>
    /// <param name="baseObject">The last value.</param>
    /// <returns>The value or <c>"n/a"</c> when object is <c>null</c>.</returns>
    private static string GetValue(BaseObject baseObject)
    {
        if (baseObject == null)
            return "n/a";

        if (baseObject.CanCastTo<IntegerObject>())
            return ((long)baseObject).ToString();
        else if (baseObject.CanCastTo<FloatObject>())
            return ((double)baseObject).ToString();

        return baseObject.ToString();
    }
}
