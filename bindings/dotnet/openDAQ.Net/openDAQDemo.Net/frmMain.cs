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
using System.Text;
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

    private const bool         LOCKED           = true;
    private const bool         FREE             = false;
    private const int          FLOW_ITEM_HEIGHT = 24 + 2*4;  //24 for 16x16 -> 2x margin of 4
    private const AnchorStyles ANCHOR_L         = AnchorStyles.Left;
    private const AnchorStyles ANCHOR_LR        = AnchorStyles.Left | AnchorStyles.Right;

    private readonly BindingList<PropertyItem>  _propertyItems  = new();
    private readonly BindingList<AttributeItem> _attributeItems = new();
    private Instance? _instance;

    private int _tableLayoutRowHeight = -1;
    private int _tableLayoutMarginLR  = -1;

    private Component? _selectedComponent;

    private readonly System.Windows.Forms.Timer _outputValueTimer;


    /// <summary>
    /// Initializes a new instance of the <see cref="frmMain"/> class.
    /// </summary>
    public frmMain()
    {
        InitializeComponent();

        _outputValueTimer = new() { Enabled = false, Interval = 1000 };
        _outputValueTimer.Tick += _outputValueTimer_Tick;

        this.Width = 1200;

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
        InitializeInputPortsView();
        InitializeOutputSignalsView();

        SetInputsOutputsAreaVisibility(isVisible: false);
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
        _outputValueTimer.Stop();

        Clear(this.tableInputPorts);
        Clear(this.tableOutputSignals);

        _propertyItems.Clear();
        _attributeItems.Clear();

        this.treeComponents.Nodes.Clear();

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
        using (var dlgLoad = new OpenFileDialog())
        {
            dlgLoad.Title           = "Load configuration";
            dlgLoad.Filter          = "Json files (*.json)|*.json|All files (*.*)|*.*";
            dlgLoad.DefaultExt      = ".json";
            dlgLoad.FileName        = "config.json";
            dlgLoad.CheckFileExists = true;

            if ((dlgLoad.ShowDialog(this) != DialogResult.OK) || string.IsNullOrWhiteSpace(dlgLoad.FileName) || !File.Exists(dlgLoad.FileName))
                return;

            try
            {
                string configString = File.ReadAllText(dlgLoad.FileName);

                _instance!.LoadConfiguration(configString);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"An error occurred:\n{ex.GetType().Name} - {ex.Message}", "Load configuration",
                                MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
            finally
            {
                UpdateTree();
            }
        }
    }

    private void saveConfigurationToolStripMenuItem_Click(object sender, EventArgs e)
    {
        using (var dlgSave = new SaveFileDialog())
        {
            dlgSave.Title           = "Save configuration";
            dlgSave.Filter          = "Json files (*.json)|*.json|All files (*.*)|*.*";
            dlgSave.DefaultExt      = ".json";
            dlgSave.FileName        = "config.json";
            dlgSave.OverwritePrompt = true;

            if ((dlgSave.ShowDialog(this) != DialogResult.OK) || string.IsNullOrWhiteSpace(dlgSave.FileName))
                return;

            try
            {
                string configString = _instance!.SaveConfiguration();
                File.WriteAllText(dlgSave.FileName, configString, Encoding.UTF8);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"An error occurred:\n{ex.GetType().Name} - {ex.Message}", "Save configuration",
                                MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }
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
            _outputValueTimer.Stop();

            _selectedComponent = e.Node?.Tag as Component;

            UpdateProperties(_selectedComponent);
            UpdateAttributes(_selectedComponent);

            if (SetInputsOutputsViewsVisibility(_selectedComponent))
            {
                UpdateInputPorts(_selectedComponent);
                UpdateOutputSignals(_selectedComponent);

                SetInfoLabelVisibility(this.lblNoInputPorts, isVisible: (this.tableInputPorts.RowCount == 0));
                SetInfoLabelVisibility(this.lblNoOutputSignals, isVisible: (this.tableOutputSignals.RowCount == 0));

                if (this.tableOutputSignals.RowCount > 0)
                    _outputValueTimer.Start();
            }
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
                    if ((component.CanCastTo<Device>() && !selectedNode.Equals(tree.Nodes?[0]))       //can remove a Device but not the instance (root)
                        || (component.CanCastTo<FunctionBlock>() && !component.CanCastTo<Channel>())) //can remove a FunctionBlock but not a Channel
                    {
                        this.contextMenuItemTreeComponentsRemove.Enabled = true;
                    }
                    else
                    {
                        e.Cancel = true; //no other menu items yet so we can cancel opening
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
                e.Cancel = true; //no menu items for other objects yet so we can cancel opening
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

        this.conetxtMenuItemGridPropertiesEdit.Enabled = !selectedProperty.IsLocked;
    }

    private void conetxtMenuItemGridPropertiesEdit_Click(object sender, EventArgs e)
    {
        var toolStripMenuItem = (ToolStripMenuItem)sender;
        var gridControl       = ((ContextMenuStrip)toolStripMenuItem.Owner).SourceControl;

        if (gridControl == this.gridProperties)
            EditSelectedProperty();
        else if (gridControl == this.gridAttributes)
            EditSelectedAttribute();
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

    #endregion gridProperties

    private void _outputValueTimer_Tick(object? sender, EventArgs e)
    {
        if (this.tableOutputSignals.RowCount == 0)
        {
            (sender as System.Windows.Forms.Timer)?.Stop();
            return;
        }

        //try to get the Signal.LastValue for each output signal and show in the label
        for (int row = 0; row < this.tableOutputSignals.RowCount; ++ row)
        {
            var label  = this.tableOutputSignals.GetControlFromPosition(1, row) as Label;
            var button = this.tableOutputSignals.GetControlFromPosition(2, row) as Button;
            var signal = button?.Tag as Signal;

            if (label != null)
                label.Text = GetLastValueForSignal(signal);
        }
    }

    #endregion //event handlers .................................................................................

    #region Common GUI methods

    /// <summary>
    /// Initializes the given <c>DataGridView</c>.
    /// </summary>
    /// <param name="grid">The <c>DataGridView</c> to initialize.</param>
    internal static void InitializeDataGridView(DataGridView grid)
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
    /// Gets the preferred width of the specified grid control.
    /// </summary>
    /// <param name="grid">The grid control.</param>
    /// <returns>The preferred width.</returns>
    private static int GetPreferredGridWidth(DataGridView grid)
    {
        return Math.Max(100, grid.GetPreferredSize(Size.Empty).Width);
    }

    /// <summary>
    /// Initializes the image list (designer would remove transparency over time when saving designer changes).
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
        imageList.Images.Add(nameof(GlblRes.link),           (Bitmap)GlblRes.link.Clone());
    }

    /// <summary>
    /// Sets the specified information label visibility.
    /// </summary>
    /// <param name="label">The label.</param>
    /// <param name="isVisible">If set to <c>true</c> the label is visible and <c>Dock.Fill</c>, otherwise <c>false</c>.</param>
    private static void SetInfoLabelVisibility(Label label, bool isVisible)
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

    /// <summary>
    /// Clears the specified table layout panel.
    /// </summary>
    /// <param name="tableLayoutPanel">The table layout panel.</param>
    private static void Clear(TableLayoutPanel tableLayoutPanel)
    {
        tableLayoutPanel.Controls.Clear();
        tableLayoutPanel.RowStyles.Clear();
        tableLayoutPanel.RowCount = 0;
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
    /// Updates the openDAQ-<c>Components</c> <c>TreeView</c>.
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

    /// <summary>
    /// Traverses the components recursively to fill the tree.
    /// </summary>
    /// <param name="component">The component.</param>
    private void TreeTraverseComponentsRecursive(Component? component)
    {
        if (component == null)
            return;

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

    /// <summary>
    /// Adds the openDAQ-<c>Component</c> to the tree.
    /// </summary>
    /// <param name="parentId">The parent ID.</param>
    /// <param name="component">The <c>Component</c>.</param>
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
            iconKey       = nameof(GlblRes.folder);
            componentName = GetStandardFolderName(componentName);
        }
        else if (component.CanCastTo<SyncComponent>())
        {
            iconKey       = nameof(GlblRes.link);
            componentName = GetStandardFolderName(componentName);
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
            var node                  = AddNode(parentNodesCollection, key: componentNodeId, componentName, iconKey);
            node.Tag                  = component;
        }


        //----- local functions ----------------------------------------------

        static string GetStandardFolderName(string folderName)
        {
            return folderName switch
            {
                "Sig"  => "Signals",
                "FB"   => "Function blocks",
                "Dev"  => "Devices",
                "IP"   => "Input ports",
                "IO"   => "Inputs/Outputs'",
                "Sync" => "Synchronization'",
                _      => folderName,
            };
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

    /// <summary>
    /// Populates the system overview tree.
    /// </summary>
    /// <param name="device">The device.</param>
    /// <param name="rootNode">The root node.</param>
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
        IListObject<Signal>  signals  = device.GetSignals();
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

    /// <summary>
    /// Populates the <c>Signals</c> in system overview tree.
    /// </summary>
    /// <param name="nodes">The nodes.</param>
    /// <param name="signals">The signals.</param>
    private void PopulateSignalsInSystemOverviewTree(TreeNodeCollection nodes, IListObject<Signal> signals)
    {
        foreach (var signal in signals)
        {
            var node = AddNode(nodes, key: signal.GlobalId, signal.Name, imageKey: nameof(GlblRes.signal));
            node.Tag = signal;
        }
    }

    /// <summary>
    /// Populates the <c>Channels</c> in system overview tree.
    /// </summary>
    /// <param name="nodes">The nodes.</param>
    /// <param name="channels">The channels.</param>
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

        ListProperties(component);

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
                propertyValue = HandleSelectionValues(propertyValue, propertySelectionValues);

            _propertyItems.Add(new(property.ReadOnly, propertyName, propertyValue.ToString(), property.Unit, property.Description, property));
        }


        //----- local functions ------------------------------------------------

        static BaseObject HandleSelectionValues(BaseObject propertyValue, BaseObject propertySelectionValues)
        {
            if (propertySelectionValues.CanCastTo<ListObject<StringObject>>())
            {
                int propertyValueIndex = (int)propertyValue;

                IList<StringObject> listObject = propertySelectionValues.Cast<ListObject<StringObject>>();

                if ((propertyValueIndex >= 0) && (propertyValueIndex < listObject.Count))
                    propertyValue = listObject[propertyValueIndex];
                else
                    propertyValue = $"n/a ({propertyValueIndex})";
            }
            else if (propertySelectionValues.CanCastTo<DictObject<IntegerObject, StringObject>>())
            {
                int propertyValueIndex = (int)propertyValue;

                IDictionary<IntegerObject, StringObject> dictObject = propertySelectionValues.Cast<DictObject<IntegerObject, StringObject>>();

                if ((propertyValueIndex >= 0) && (propertyValueIndex < dictObject.Count))
                    propertyValue = dictObject[propertyValueIndex];
                else
                    propertyValue = $"n/a ({propertyValueIndex})";
            }
            else
            {
                propertyValue = "( unknown selection-value type )";
            }

            return propertyValue;
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
        var property                = (Property)selectedPropertyItem.OpenDaqObject; //selectedComponent.GetProperty(propertyName);
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
    /// Updates the given attributes grid content.
    /// </summary>
    /// <param name="component">The openDAQ <c>Component</c> to get the attributes from.</param>
    private void UpdateAttributes(Component? component)
    {
        UpdateAttributes(component, _attributeItems);

        this.gridAttributes.ClearSelection();
        this.gridAttributes.AutoResizeColumns();
    }

    /// <summary>
    /// Updates the given attributes grid content.
    /// </summary>
    /// <param name="component">The openDAQ <c>Component</c> to get the attributes from.</param>
    /// <param name="attributeItems">The list to be updated.</param>
    /// <remarks>
    /// This is <c>internal static</c> to offer the functionality to other classes (forms).<br/>
    /// Updating the GUI has to be done outside.
    /// </remarks>
    internal static void UpdateAttributes(Component? component, BindingList<AttributeItem> attributeItems)
    {
        attributeItems.Clear();

        if (component == null)
            return;

        ListAttributes(component.Cast<Component>(), attributeItems); //cast to interface so that only Component attributes are visible

        if (component.Cast<Device>() is Device device)
            ListAttributes(device, attributeItems);
        else if (component.Cast<Signal>() is Signal signal)
            ListAttributes(signal, attributeItems);
        else if (component.Cast<InputPort>() is InputPort inputPort)
            ListAttributes(inputPort, attributeItems);
        else if (component.Cast<FunctionBlock>() is FunctionBlock functionBlock)
            ListAttributes(functionBlock, attributeItems);

        //no <SyncComponent> here as its attributes are also properties
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.Component"/> attributes.
    /// </summary>
    /// <param name="component">The object with the attributes.</param>
    /// <param name="attributeItems">The list to be updated.</param>
    private static void ListAttributes(Component component, BindingList<AttributeItem> attributeItems)
    {
        attributeItems.Add(new(FREE,   "Name",        "Name",        component.Name,               CoreType.ctString, component));
        attributeItems.Add(new(FREE,   "Description", "Description", component.Description,        CoreType.ctString, component));
        attributeItems.Add(new(FREE,   "Active",      "Active",      component.Active.ToString(),  CoreType.ctBool,   component));
        attributeItems.Add(new(LOCKED, "GlobalId",    "Global ID",   component.GlobalId,           CoreType.ctString, component));
        attributeItems.Add(new(LOCKED, "LocalId",     "Local ID",    component.LocalId,            CoreType.ctString, component));
        attributeItems.Add(new(FREE,   "Tags",        "Tags",        component.Tags,               CoreType.ctObject, component));
        attributeItems.Add(new(LOCKED, "Visible",     "Visible",     component.Visible.ToString(), CoreType.ctBool,   component));
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.Device"/> attributes.
    /// </summary>
    /// <param name="device">The object with the attributes.</param>
    /// <param name="attributeItems">The list to be updated.</param>
    private static void ListAttributes(Device device, BindingList<AttributeItem> attributeItems)
    {
        //nothing to list here
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.Signal"/> attributes.
    /// </summary>
    /// <param name="signal">The object with the attributes.</param>
    /// <param name="attributeItems">The list to be updated.</param>
    private static void ListAttributes(Signal signal, BindingList<AttributeItem> attributeItems)
    {
        string relatedSignalIds = string.Join(", ", signal.RelatedSignals?.Select(sig => sig.GlobalId) ?? Array.Empty<string>());

        attributeItems.Add(new(FREE,   "Public",               "Public",              signal.Public.ToString(),     CoreType.ctBool,      signal));
        attributeItems.Add(new(LOCKED, "DomainSignalGlobalId", "Domain Signal ID",    signal.DomainSignal.GlobalId, CoreType.ctString,    signal));
        attributeItems.Add(new(LOCKED, "RelatedSignalsIDs",    "Related Signals IDs", relatedSignalIds,             CoreType.ctList,      signal));
        attributeItems.Add(new(LOCKED, "Streamed",             "Streamed",            signal.Streamed.ToString(),   CoreType.ctBool,      signal));
        attributeItems.Add(new(LOCKED, "LastValue",            "Last Value",          GetValue(signal.LastValue),   CoreType.ctUndefined, signal));
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.InputPort"/> attributes.
    /// </summary>
    /// <param name="inputPort">The object with the attributes.</param>
    /// <param name="attributeItems">The list to be updated.</param>
    private static void ListAttributes(InputPort inputPort, BindingList<AttributeItem> attributeItems)
    {
        attributeItems.Add(new(LOCKED, "SignalGlobalId", "Signal ID",       inputPort.Signal?.GlobalId,          CoreType.ctString, inputPort));
        attributeItems.Add(new(LOCKED, "RequiresSignal", "Requires Signal", inputPort.RequiresSignal.ToString(), CoreType.ctBool,   inputPort));
    }

    /// <summary>
    /// Lists the <see cref="Daq.Core.OpenDAQ.FunctionBlock"/> attributes.
    /// </summary>
    /// <param name="functionBlock">The object with the attributes.</param>
    /// <param name="attributeItems">The list to be updated.</param>
    private static void ListAttributes(FunctionBlock functionBlock, BindingList<AttributeItem> attributeItems)
    {
        //nothing to list here
    }

    /// <summary>
    /// Open edit-dialog for the selected attribute (attribute grid).
    /// </summary>
    private void EditSelectedAttribute()
    {
        var selectedAttributeItem = (AttributeItem)this.gridAttributes.SelectedRows[0].DataBoundItem;

        EditSelectedAttribute(this, selectedAttributeItem);
        UpdateAttributes((Component)selectedAttributeItem.OpenDaqObject);
    }

    /// <summary>
    /// Open edit-dialog for the given attribute.
    /// </summary>
    /// <param name="owner">The owner control for the edit dialog.</param>
    /// <param name="attributeItem">The attribute to edit.</param>
    /// <remarks>
    /// This is <c>internal static</c> to offer the functionality to other classes (forms).<br/>
    /// Updating the GUI has to be done outside.
    /// </remarks>
    internal static void EditSelectedAttribute(IWin32Window owner, AttributeItem attributeItem)
    {
        var selectedAttributeName   = attributeItem.Name;
        var selectedAttributeValue  = attributeItem.Value;
        var selectedAttributeObject = attributeItem.OpenDaqObject;

        string stringValue = string.Empty;
        long   intValue    = long.MinValue;
        double floatValue  = double.NaN;

        string askDialogTitle       = $"Edit attribute \"{attributeItem.DisplayName}\"";
        string askDialogCaption     = "Please enter the new value below";
        //string askDialogCaptionList = "Please select the new value below";

        //ask for the new attribute value (just return when unchanged)
        switch (attributeItem.ValueType)
        {
            case CoreType.ctBool:
                //will just toggle
                break;

            case CoreType.ctInt:
                intValue = frmInputDialog.AskInteger(owner, askDialogTitle, askDialogCaption, 0);
                if (intValue.ToString() == selectedAttributeValue)
                    return;
                break;

            case CoreType.ctFloat:
                floatValue = frmInputDialog.AskFloat(owner, askDialogTitle, askDialogCaption, 0);
                if (floatValue.ToString() == selectedAttributeValue)
                    return;
                break;

            case CoreType.ctString:
                stringValue = frmInputDialog.AskString(owner, askDialogTitle, askDialogCaption, attributeItem.Value);
                if (stringValue == selectedAttributeValue)
                    return;
                break;

            //case CoreType.ctList:
            //    intValue = frmInputDialog.AskList(owner, askDialogTitle, askDialogCaptionList, 0, null);
            //    if (intValue == oldIndex)
            //        return;
            //    break;

            //case CoreType.ctDict:
            //    intValue = frmInputDialog.AskDict(owner, askDialogTitle, askDialogCaptionList, 0, null);
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
                MessageBox.Show($"Sorry, opeanDAQ value type '{attributeItem.ValueType}' is not editable here.",
                                askDialogTitle, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                return;
        }

        //set the new attribute value
        switch (selectedAttributeObject)
        {
            case Instance rootDevice:
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

            //inherited by all of the above so do this last but outside switch()
            //case Component component:
            //    break;
        }

        var component = (Component)selectedAttributeObject;
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
    }

    #endregion Attributes view

    /// <summary>
    /// Sets the visibility of the inputs- and/or outputs-view.
    /// </summary>
    /// <param name="component">The openDAQ <c>Component</c> determining whether to display a view or not.</param>
    /// <returns><c>true</c> when an inputs or outputs view is visible.</returns>
    private bool SetInputsOutputsViewsVisibility(Component? component)
    {
        if (component == null)
        {
            SetInputsOutputsAreaVisibility(isVisible: false);
            return false;
        }

        bool isDevice        = component.CanCastTo<Device>();
        bool isChannel       = component.CanCastTo<Channel>();
        bool isFunctionBlock = component.CanCastTo<FunctionBlock>() && !isChannel;

        SetInputsOutputsAreaVisibility(isVisible: (isDevice || isFunctionBlock));

        if (isFunctionBlock)
        {
            //both, input ports and output signals visible
            this.groupInputPorts.Visible    = true;
            this.groupOutputSignals.Visible = true;

            this.groupOutputSignals.Dock = DockStyle.Bottom;
            this.groupInputPorts.BringToFront();
        }
        else if (isDevice || isChannel)
        {
            //only output signals visible
            this.groupInputPorts.Visible    = false;
            this.groupOutputSignals.Visible = true;

            this.groupOutputSignals.Dock = DockStyle.Fill;
        }
        else
        {
            //neither is visible
            return false;
        }

        return true;
    }

    /// <summary>
    /// Sets the visibility of the inputs/outputs area.
    /// </summary>
    /// <param name="isVisible">If set to <c>true</c> inputs/outputs should be visible.</param>
    private void SetInputsOutputsAreaVisibility(bool isVisible)
    {
        this.splitContainerPropertiesInputs.Panel2Collapsed = !isVisible;

        if (!isVisible)
            return;

        //set splitter to the properties grid width (but only to a maximum of 60% of the container width)
        int preferredSplitterDistance = Math.Min(GetPreferredGridWidth(this.gridProperties), this.splitContainerPropertiesInputs.Width * 60 / 100);
        if (this.splitContainerPropertiesInputs.SplitterDistance < preferredSplitterDistance)
            this.splitContainerPropertiesInputs.SplitterDistance = preferredSplitterDistance;

        this.tableInputPorts.Left  = this.groupInputPorts.Margin.Left;
        this.tableInputPorts.Width = this.groupInputPorts.ClientRectangle.Width - this.groupInputPorts.Margin.Left - this.groupInputPorts.Margin.Right;
        //this.tableInputPorts.PerformLayout();

        this.tableOutputSignals.Left  = this.groupOutputSignals.Margin.Left;
        this.tableOutputSignals.Width = this.groupOutputSignals.ClientRectangle.Width - this.groupOutputSignals.Margin.Left - this.groupOutputSignals.Margin.Right;
        //this.tableOutputSignals.PerformLayout();
    }

    #region Input-ports view

    /// <summary>
    /// Initializes the input-port view.
    /// </summary>
    private void InitializeInputPortsView()
    {
        _tableLayoutMarginLR  = this.tableInputPorts.Margin.Left + this.tableInputPorts.Margin.Right;
        _tableLayoutRowHeight = this.tableInputPorts.Margin.Top + FLOW_ITEM_HEIGHT + this.tableInputPorts.Margin.Bottom;

        this.tableInputPorts.GrowStyle = TableLayoutPanelGrowStyle.FixedSize; //we take care of RowCount ourselves

        //RowCount / RowStyles dynamically set in UpdateInputPorts()
        Clear(this.tableInputPorts);

        this.tableInputPorts.ColumnStyles.Clear();
        this.tableInputPorts.ColumnCount = 4;
        this.tableInputPorts.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, FLOW_ITEM_HEIGHT + _tableLayoutMarginLR));
        this.tableInputPorts.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));//SizeType.AutoSize));
        this.tableInputPorts.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, FLOW_ITEM_HEIGHT + _tableLayoutMarginLR));
        this.tableInputPorts.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, FLOW_ITEM_HEIGHT + _tableLayoutMarginLR));

        SetInfoLabelVisibility(this.lblNoInputPorts, isVisible: true);
    }

    /// <summary>
    /// Updates the input-ports view.
    /// </summary>
    /// <param name="component">The component.</param>
    /// <returns><c>true</c> when input ports have been rendered, otherwise <c>false</c>.</returns>
    private bool UpdateInputPorts(Component? component)
    {
        Clear(this.tableInputPorts);

        //let only FunctionBlock through (no Channel which inherits from FunctionBlock)
        if ((component == null)
            || !component.CanCastTo<FunctionBlock>()
            || component.CanCastTo<Channel>())
        {
            return false;
        }

        var functionBlock = component.Cast<FunctionBlock>();
        var inputPorts    = functionBlock.GetInputPorts();

        SetInputsOutputsAreaVisibility(isVisible: true);

        if ((inputPorts == null) || (inputPorts.Count == 0))
            return true; //true, because it could have input ports

        var rootDevice = GetRootComponent(functionBlock)?.Cast<Device>();
        var allSignals = rootDevice?.GetSignalsRecursive() ?? Enumerable.Empty<Signal>();

        var inputSignals = new List<KeyValuePair<string, Signal?>>();
        inputSignals.Clear();
        inputSignals.Add(new KeyValuePair<string, Signal?>("none", null));
        inputSignals.AddRange(allSignals.Select(signal => new KeyValuePair<string, Signal?>(GetShortId(signal), signal)));

        foreach (var inputPort in inputPorts)
        {
            int selectedSignalIndex = inputSignals.FindIndex(kvp => kvp.Value?.GlobalId == inputPort.Signal?.GlobalId);

            //always create new BindingList<>, otherwise all ComboBoxes are linked and they would change their selected item
            AddInputPort(inputPort, new BindingList<KeyValuePair<string, Signal?>>(inputSignals), selectedSignalIndex);
        }

        return (this.tableInputPorts.RowCount > 0);
    }

    /// <summary>
    /// Adds the given input port with the given signals.
    /// </summary>
    /// <param name="inputPort">The input port.</param>
    /// <param name="signalsDataSource">The data source with the <c>Signal</c>s.</param>
    /// <param name="selectedIndex">Index of the <c>Signal</c> to select.</param>
    private void AddInputPort(InputPort inputPort, BindingList<KeyValuePair<string, Signal?>> signalsDataSource, int selectedIndex)
    {
        string longestDataSourceKey = signalsDataSource
                                      .Select(kvp => kvp.Key)
                                      .Aggregate((longest, next) => (next.Length > longest.Length) ? next : longest);
        int dropDownWidth = TextRenderer.MeasureText(longestDataSourceKey, this.tableInputPorts.Font).Width;

        //create controls
        var lblCaption = CreateCaptionLabel(inputPort.Name);
        var cmbSignals = CreateComboBox(signalsDataSource, dropDownWidth);
        var btnLink    = CreateLinkButton();
        var btnEdit    = CreateEditButton();

        //add row
        ++this.tableInputPorts.RowCount;
        this.tableInputPorts.RowStyles.Add(new RowStyle(SizeType.Absolute, _tableLayoutRowHeight));
        int rowNo = this.tableInputPorts.RowCount - 1;

        //add controls
        this.tableInputPorts.Controls.Add(lblCaption, 0, rowNo);
        this.tableInputPorts.Controls.Add(cmbSignals, 1, rowNo);
        this.tableInputPorts.Controls.Add(btnLink,    2, rowNo);
        this.tableInputPorts.Controls.Add(btnEdit,    3, rowNo);

        //resize label column if necessary
        int preferredWidth = this.tableInputPorts.Margin.Left + lblCaption.PreferredWidth + this.tableInputPorts.Margin.Right;
        if (this.tableInputPorts.ColumnStyles[0].Width < preferredWidth)
            this.tableInputPorts.ColumnStyles[0].Width = preferredWidth;

        //add event handlers
        cmbSignals.SelectedIndexChanged += comboBoxSignals_SelectedIndexChanged;
        btnLink.Click                   += btnLink_Click;
        btnEdit.Click                   += btnEdit_Click;

        //store original selection
        cmbSignals.Tag = selectedIndex;

        //only after control has been added to GUI (TableLayoutPanel) we can set SelectedIndex
        cmbSignals.SelectedIndex = selectedIndex;

        //store the InputPort
        btnLink.Tag = inputPort;
        btnEdit.Tag = inputPort;


        //--- local functions --------------------------------------------------

        static Label CreateCaptionLabel(string caption)
        {
            return new Label()
            {
                AutoSize  = false,
                Text      = caption,
                TextAlign = ContentAlignment.MiddleRight,
                Anchor    = ANCHOR_LR
            };
        }

        static ComboBox CreateComboBox(BindingList<KeyValuePair<string, Signal?>> dataSource, int dropDownWidth)
        {
            return new ComboBox
            {
                DropDownStyle = ComboBoxStyle.DropDownList,
                Anchor        = ANCHOR_LR,
                Height        = FLOW_ITEM_HEIGHT,
                DropDownWidth = dropDownWidth,
                DataSource    = dataSource,
                DisplayMember = "Key",
                ValueMember   = "Value"
            };
        }

        static Button CreateLinkButton()
        {
            return new Button()
            {
                Enabled = false,
                Text    = null,
                Anchor  = ANCHOR_L,
                Width   = FLOW_ITEM_HEIGHT,
                Height  = FLOW_ITEM_HEIGHT,
                Image   = GetLinkImage(isLinked: false)
            };
        }

        static Button CreateEditButton()
        {
            return new Button()
            {
                Enabled = true,
                Text    = null,
                Anchor  = ANCHOR_L,
                Width   = FLOW_ITEM_HEIGHT,
                Height  = FLOW_ITEM_HEIGHT,
                Image   = new Bitmap(GlblRes.settings, 24, 24)
            };
        }

        void comboBoxSignals_SelectedIndexChanged(object? sender, EventArgs e)
        {
            if (sender is not ComboBox comboBox)
                return;

            //get the link Button
            var cellPos = this.tableInputPorts.GetCellPosition(comboBox);
            if (this.tableInputPorts.GetControlFromPosition(cellPos.Column + 1, cellPos.Row) is not Button btnLink)
                return;

            int  originalSelection    = (int)comboBox.Tag;
            bool isOriginalSelected   = (originalSelection == comboBox.SelectedIndex);
            bool isInputPortConnected = (originalSelection > 0);

            btnLink.Enabled = !isOriginalSelected;
            btnLink.Image   = GetLinkImage(isLinked: isInputPortConnected);
        }

        void btnLink_Click(object? sender, EventArgs e)
        {
            if (sender is not Button btnLink)
                return;

            //get the ComboBox
            var cellPos = this.tableInputPorts.GetCellPosition(btnLink);
            if (this.tableInputPorts.GetControlFromPosition(cellPos.Column - 1, cellPos.Row) is not ComboBox comboBox)
                return;

            if (btnLink.Tag is not InputPort inputPort)
                return;

            LinkInputPort(inputPort, comboBox.SelectedValue as Signal);
        }

        void btnEdit_Click(object? sender, EventArgs e)
        {
            if (sender is not Button btnEdit)
                return;

            EditInputPort(btnEdit.Tag as InputPort);
        }
    }

    /// <summary>
    /// Gets the image for the link-button (open or closed link symbol).
    /// </summary>
    /// <param name="isLinked">If set to <c>true</c> get the linked symbol (closed), otherwise the unlinked symbol (open).</param>
    /// <returns>The image.</returns>
    private static Image GetLinkImage(bool isLinked)
    {
        return new Bitmap(isLinked ? GlblRes.link : GlblRes.unlink, 24, 24);
    }

    /// <summary>
    /// Links or unlinks the specified <c>Signal</c> to the specified <c>InputPort</c>.
    /// </summary>
    /// <param name="inputPort">The input port.</param>
    /// <param name="signal">The signal.</param>
    private void LinkInputPort(InputPort inputPort, Signal? signal)
    {
        if (signal == null) //"none"
        {
            inputPort.Disconnect();
        }
        else
        {
            inputPort.Connect(signal);
        }

        UpdateInputPorts(_selectedComponent);
    }

    /// <summary>
    /// Edits the specified <c>InputPort</c> attributes.
    /// </summary>
    /// <param name="inputPort">The input port.</param>
    private void EditInputPort(InputPort? inputPort)
    {
        if (inputPort == null)
            return;

        using (var frm = new frmEditComponent(inputPort))
        {
            frm.ShowDialog(this);
        }

        UpdateInputPorts(_selectedComponent);
    }

    #endregion Input-ports view

    #region Output-signals view

    /// <summary>
    /// Initializes the output-signals view.
    /// </summary>
    private void InitializeOutputSignalsView()
    {
        _tableLayoutMarginLR  = this.tableOutputSignals.Margin.Left + this.tableOutputSignals.Margin.Right;
        _tableLayoutRowHeight = this.tableOutputSignals.Margin.Top + FLOW_ITEM_HEIGHT + this.tableOutputSignals.Margin.Bottom;

        this.tableOutputSignals.GrowStyle = TableLayoutPanelGrowStyle.FixedSize; //we take care of RowCount ourselves

        //RowCount / RowStyles dynamically set in UpdateOutputSignals()
        Clear(this.tableOutputSignals);

        this.tableOutputSignals.ColumnStyles.Clear();
        this.tableOutputSignals.ColumnCount = 3;
        this.tableOutputSignals.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, FLOW_ITEM_HEIGHT + _tableLayoutMarginLR));
        this.tableOutputSignals.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));//SizeType.AutoSize));
        this.tableOutputSignals.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, FLOW_ITEM_HEIGHT + _tableLayoutMarginLR));

        SetInfoLabelVisibility(this.lblNoOutputSignals, isVisible: true);
    }

    /// <summary>
    /// Updates the output-signals view.
    /// </summary>
    /// <param name="component">The component.</param>
    /// <returns><c>true</c> when output signals have been rendered, otherwise <c>false</c>.</returns>
    private bool UpdateOutputSignals(Component? component)
    {
        Clear(this.tableOutputSignals);

        //let only Device and FunctionBlock through (no Channel which inherits from FunctionBlock)
        if ((component == null)
            || (!component.CanCastTo<Device>() && !component.CanCastTo<FunctionBlock>())
            /*|| component.CanCastTo<Channel>()*/)
        {
            return false;
        }

        //component is either Device or FunctionBlock
        var outputSignals = component.Cast<Device>()?.GetSignals()
                            ?? component.Cast<FunctionBlock>().GetSignals();

        SetInputsOutputsAreaVisibility(isVisible: true);

        if ((outputSignals == null) || (outputSignals.Count == 0))
            return true; //true, because it could have output signals

        foreach (var outputSignal in outputSignals)
        {
            AddOutputSignal(outputSignal);
        }

        return (this.tableOutputSignals.RowCount > 0);
    }

    /// <summary>
    /// Adds the given input port with the given signals.
    /// </summary>
    /// <param name="outputSignal">The input port.</param>
    /// <param name="signalsDataSource">The data source with the <c>Signal</c>s.</param>
    /// <param name="selectedIndex">Index of the <c>Signal</c> to select.</param>
    private void AddOutputSignal(Signal outputSignal)
    {
        //create controls
        var lblCaption = CreateCaptionLabel(outputSignal.Name);
        var lblValue   = CreateValueLabel(GetLastValueForSignal(outputSignal));
        var btnEdit    = CreateEditButton();

        //add row
        ++this.tableOutputSignals.RowCount;
        this.tableOutputSignals.RowStyles.Add(new RowStyle(SizeType.Absolute, _tableLayoutRowHeight));
        int rowNo = this.tableOutputSignals.RowCount - 1;

        //add controls
        this.tableOutputSignals.Controls.Add(lblCaption, 0, rowNo);
        this.tableOutputSignals.Controls.Add(lblValue,   1, rowNo);
        this.tableOutputSignals.Controls.Add(btnEdit,    2, rowNo);

        //resize label column if necessary
        int preferredWidth = this.tableOutputSignals.Margin.Left + lblCaption.PreferredWidth + this.tableOutputSignals.Margin.Right;
        if (this.tableOutputSignals.ColumnStyles[0].Width < preferredWidth)
            this.tableOutputSignals.ColumnStyles[0].Width = preferredWidth;

        //add event handlers
        btnEdit.Click += btnEdit_Click;

        //store the OutputSignal
        btnEdit.Tag = outputSignal;


        //--- local functions --------------------------------------------------

        static Label CreateCaptionLabel(string caption)
        {
            return new Label()
            {
                AutoSize  = false,
                Text      = caption,
                TextAlign = ContentAlignment.MiddleRight,
                Anchor    = ANCHOR_LR
            };
        }

        static Label CreateValueLabel(string value)
        {
            return new Label()
            {
                AutoSize  = false,
                Text      = value,
                TextAlign = ContentAlignment.MiddleCenter,
                Anchor    = ANCHOR_LR
            };
        }

        static Button CreateEditButton()
        {
            return new Button()
            {
                Enabled = true,
                Text    = null,
                Anchor  = ANCHOR_L,
                Width   = FLOW_ITEM_HEIGHT,
                Height  = FLOW_ITEM_HEIGHT,
                Image   = new Bitmap(GlblRes.settings, 24, 24)
            };
        }

        void btnEdit_Click(object? sender, EventArgs e)
        {
            if (sender is not Button btnEdit)
                return;

            EditOutputSignal(btnEdit.Tag as Signal);
        }
    }

    /// <summary>
    /// Edits the output signal attributes.
    /// </summary>
    /// <param name="outputSignal">The output signal.</param>
    private void EditOutputSignal(Signal? outputSignal)
    {
        if (outputSignal == null)
            return;

        using (var frm = new frmEditComponent(outputSignal))
        {
            frm.ShowDialog(this);
        }

        UpdateOutputSignals(_selectedComponent);
    }

    #endregion Output-signals view

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

    /// <summary>
    /// Gets the root <see cref="Component"/> for the given component object.
    /// </summary>
    /// <param name="component">The <c>Component</c>.</param>
    /// <returns>The root <c>Component</c>.</returns>
    private static Component GetRootComponent(Component component)
    {
        while (component.Parent != null)
            component = component.Parent;

        return component;
    }

    /// <summary>
    /// Gets the short identifier for the given <see cref="Signal"/>.
    /// </summary>
    /// <param name="signal">The <c>Signal</c>.</param>
    /// <returns>The short identifier.</returns>
    private string GetShortId(Signal? signal)
    {
        if (signal == null)
            return string.Empty;

        // /4aa5533b-e463-4f8b-9d81-7995ed56274c/Dev/RefDev0/Dev/RefDev1/IO/AI/RefCh0/Sig/AI0
        // -> RefDev0/RefDev1/RefCh0/AI0

        var parts       = signal.GlobalId.Split('/', StringSplitOptions.RemoveEmptyEntries).ToList();
        var ignoreParts = new List<string>() { "IO", "FB", "Sig", "Dev" };

        parts.RemoveAt(0); //remove root
        parts.RemoveAll(part => ignoreParts.Contains(part));

        return string.Join('/', parts.ToArray());
    }

    /// <summary>
    /// Gets the last value for the specified signal.
    /// </summary>
    /// <param name="outputSignal">The output signal.</param>
    /// <returns>The last sample.</returns>
    private string GetLastValueForSignal(Signal? outputSignal)
    {
        string lastValue = "N/A";

        if (outputSignal != null)
        {
            try
            {
                lastValue = GetValue(outputSignal.LastValue);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.Print($"Error reading last value: {ex.GetType().Name} - {ex.Message}");
            }
        }

        return lastValue;
    }
}
