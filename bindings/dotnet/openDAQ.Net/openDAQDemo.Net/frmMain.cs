/*
 * Copyright 2022-2024 openDAQ d. o. o.
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

using Daq.Core.Objects;
using Daq.Core.OpenDAQ;
using Daq.Core.Types;

using Task = System.Threading.Tasks.Task; //ToDo: rename openDAQ Task to TaskObject

using GlblRes = global::openDAQDemoNet.Properties.Resources;
using System.Windows.Forms;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;
using System.Security.Policy;
using System.Windows.Forms.VisualStyles;


namespace openDAQDemoNet;


public partial class frmMain : Form
{
    Instance? _instance;

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

    public frmMain()
    {
        InitializeComponent();

        //ToDo:
        this.btnAddFunctionBlock.Enabled = false;

        //for easy selected tab identification
        this.tabSystemOverview.Tag = eTabstrip.SystemOverview;
        this.tabSignals.Tag        = eTabstrip.Signals;
        this.tabChannels.Tag       = eTabstrip.Channels;
        this.tabFunctionBlocks.Tag = eTabstrip.FunctionBlocks;
        this.tabFullTopology.Tag   = eTabstrip.FullTopology;

        this.treeComponents.HideSelection = false;

        this.treeComponents.Nodes.Clear();
        this.listComponent.Items.Clear();

        this.imglTreeImages.Images.Clear();
        this.imglTreeImages.ImageSize = new Size(24, 24);
        this.imglTreeImages.Images.Add(nameof(GlblRes.circle),         (Bitmap)GlblRes.circle.Clone());
        this.imglTreeImages.Images.Add(nameof(GlblRes.device),         (Bitmap)GlblRes.device.Clone());
        this.imglTreeImages.Images.Add(nameof(GlblRes.folder),         (Bitmap)GlblRes.folder.Clone());
        this.imglTreeImages.Images.Add(nameof(GlblRes.function_block), (Bitmap)GlblRes.function_block.Clone());
        this.imglTreeImages.Images.Add(nameof(GlblRes.channel),        (Bitmap)GlblRes.channel.Clone());
        this.imglTreeImages.Images.Add(nameof(GlblRes.signal),         (Bitmap)GlblRes.signal.Clone());

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
        this.listComponent.Items.Clear();

        _instance?.Dispose();
    }

    private void frmMain_Shown(object sender, EventArgs e)
    {
        SetWaitCursor();
        this.Update();

        TreeUpdate();

        ResetWaitCursor();

        this.treeComponents.Select();
    }

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
        TreeUpdate();
    }

    private void componentsInsteadOfDirectObjectAccessToolStripMenuItem_Click(object sender, EventArgs e)
    {
        TreeUpdate();
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

        TreeUpdate();

        ResetWaitCursor();
    }

    private void btnAddFunctionBlock_Click(object sender, EventArgs e)
    {
        //ToDo: add function block
    }

    private void btnRefresh_Click(object sender, EventArgs e)
    {
        TreeUpdate();
    }

    private void tabControl1_Selected(object sender, TabControlEventArgs e)
    {
        TreeUpdate();
    }

    private void treeSystemOverview_AfterSelect(object sender, TreeViewEventArgs e)
    {
        this.listComponent.Items.Clear();

        bool useComponentApproach = this.componentsInsteadOfDirectObjectAccessToolStripMenuItem.Checked
                                    || ((eTabstrip)this.tabControl1.SelectedTab.Tag != eTabstrip.SystemOverview);

        switch (e.Node?.Tag)
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

    #endregion //event handlers .................................................................................

    private void SetWaitCursor()
    {
        Cursor.Current     = Cursors.WaitCursor;
        this.Cursor        = Cursors.WaitCursor;
        this.UseWaitCursor = true;
    }

    private void ResetWaitCursor()
    {
        this.UseWaitCursor = false;
        this.Cursor        = Cursors.Default;
        Cursor.Current     = Cursors.Default;
        base.ResetCursor();
    }

    private void TreeUpdate()
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
        TreeNode? foundNode = !string.IsNullOrEmpty(selectedNodeName)
                              ? this.treeComponents.Nodes.Find(selectedNodeName, searchAllChildren: true).FirstOrDefault()
                              : (this.treeComponents.Nodes.Count > 0)
                                ? this.treeComponents.Nodes[0]
                                : null;
        this.treeComponents.SelectedNode = foundNode;
    }

    #region Components approach (as in Python Demo)

    private void TreeTraverseComponentsRecursive(Component? component)
    {
        if (component == null)
            return;

        Folder folder = component.Cast<Folder>();

        // tree view only in topology mode + parent exists
        //parent_id = '' if display_type not in (
        //    DisplayType.UNSPECIFIED, DisplayType.TOPOLOGY, DisplayType.SYSTEM_OVERVIEW, None) or component.parent is None else component.parent.global_id
        string parentId = component.Parent?.GlobalId ?? string.Empty;

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

                case eTabstrip.Signals:
                    if (component.CanCastTo<Signal>())
                    { }
                    break;

                case eTabstrip.Channels:
                    if (component.CanCastTo<Channel>())
                    { }
                    break;

                case eTabstrip.FunctionBlocks:
                    if (component.CanCastTo<FunctionBlock>())
                    { }
                    break;

                case eTabstrip.FullTopology:
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

        if (component.Cast<Channel>() is Channel channel)
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
        else //skipping unknown type components
            skip = true;

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

    private TreeNode AddNode(TreeNodeCollection nodesCollection, string? key, string text, string imageKey)
    {
        return nodesCollection.Add(key, text, imageKey, imageKey);
    }

    private void ListProperties(PropertyObject propertyObject)
    {
        var properties = propertyObject.AllProperties;

        foreach (var property in properties)
        {
            string     propertyName        = property.Name;
            CoreType   propertyType        = property.ValueType;
            BaseObject propertyValueObject = propertyObject.GetPropertyValue(propertyName);
            BaseObject propertyValue       = CoreTypesFactory.GetPropertyValueObject(propertyValueObject, propertyType);

            this.listComponent.Items.Add($"{propertyName,-25} = {propertyValue}");
        }
    }
}
