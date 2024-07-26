namespace openDAQDemoNet
{
    partial class frmMain
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            components = new System.ComponentModel.Container();
            TreeNode treeNode1 = new TreeNode("treeComponents");
            treeComponents = new TreeView();
            contextMenuStripTreeComponents = new ContextMenuStrip(components);
            contextMenuItemTreeComponentsRemove = new ToolStripMenuItem();
            menuStrip1 = new MenuStrip();
            fileToolStripMenuItem = new ToolStripMenuItem();
            loadConfigurationToolStripMenuItem = new ToolStripMenuItem();
            saveConfigurationToolStripMenuItem = new ToolStripMenuItem();
            toolStripSeparator1 = new ToolStripSeparator();
            exitToolStripMenuItem = new ToolStripMenuItem();
            viewToolStripMenuItem = new ToolStripMenuItem();
            showHiddenComponentsToolStripMenuItem = new ToolStripMenuItem();
            componentsInsteadOfDirectObjectAccessToolStripMenuItem = new ToolStripMenuItem();
            toolStrip1 = new ToolStrip();
            btnAddDevice = new ToolStripButton();
            btnAddFunctionBlock = new ToolStripButton();
            btnRefresh = new ToolStripButton();
            tabControl1 = new TabControl();
            tabSystemOverview = new TabPage();
            tabSignals = new TabPage();
            tabChannels = new TabPage();
            tabFunctionBlocks = new TabPage();
            tabFullTopology = new TabPage();
            splitContainer1 = new SplitContainer();
            gridProperties = new DataGridView();
            contextMenuStripGridProperties = new ContextMenuStrip(components);
            conetxtMenuItemGridPropertiesEdit = new ToolStripMenuItem();
            imglTreeImages = new ImageList(components);
            contextMenuStripTreeComponents.SuspendLayout();
            menuStrip1.SuspendLayout();
            toolStrip1.SuspendLayout();
            tabControl1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)splitContainer1).BeginInit();
            splitContainer1.Panel1.SuspendLayout();
            splitContainer1.Panel2.SuspendLayout();
            splitContainer1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)gridProperties).BeginInit();
            contextMenuStripGridProperties.SuspendLayout();
            SuspendLayout();
            // 
            // treeComponents
            // 
            treeComponents.ContextMenuStrip = contextMenuStripTreeComponents;
            treeComponents.Dock = DockStyle.Fill;
            treeComponents.Location = new Point(3, 3);
            treeComponents.Name = "treeComponents";
            treeNode1.Name = "Node0";
            treeNode1.Text = "treeComponents";
            treeComponents.Nodes.AddRange(new TreeNode[] { treeNode1 });
            treeComponents.Size = new Size(297, 542);
            treeComponents.TabIndex = 2;
            treeComponents.AfterSelect += treeComponents_AfterSelect;
            treeComponents.NodeMouseClick += treeComponents_NodeMouseClick;
            // 
            // contextMenuStripTreeComponents
            // 
            contextMenuStripTreeComponents.Items.AddRange(new ToolStripItem[] { contextMenuItemTreeComponentsRemove });
            contextMenuStripTreeComponents.Name = "contextMenuStrip1";
            contextMenuStripTreeComponents.Size = new Size(181, 48);
            contextMenuStripTreeComponents.Opening += contextMenuStripTreeComponents_Opening;
            // 
            // contextMenuItemTreeComponentsRemove
            // 
            contextMenuItemTreeComponentsRemove.Name = "contextMenuItemTreeComponentsRemove";
            contextMenuItemTreeComponentsRemove.Size = new Size(180, 22);
            contextMenuItemTreeComponentsRemove.Text = "Remove";
            contextMenuItemTreeComponentsRemove.Click += contextMenuItemTreeComponentsRemove_Click;
            // 
            // menuStrip1
            // 
            menuStrip1.Items.AddRange(new ToolStripItem[] { fileToolStripMenuItem, viewToolStripMenuItem });
            menuStrip1.Location = new Point(0, 0);
            menuStrip1.Name = "menuStrip1";
            menuStrip1.Size = new Size(784, 24);
            menuStrip1.TabIndex = 4;
            menuStrip1.Text = "menuStrip1";
            // 
            // fileToolStripMenuItem
            // 
            fileToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { loadConfigurationToolStripMenuItem, saveConfigurationToolStripMenuItem, toolStripSeparator1, exitToolStripMenuItem });
            fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            fileToolStripMenuItem.Size = new Size(37, 20);
            fileToolStripMenuItem.Text = "File";
            // 
            // loadConfigurationToolStripMenuItem
            // 
            loadConfigurationToolStripMenuItem.Name = "loadConfigurationToolStripMenuItem";
            loadConfigurationToolStripMenuItem.Size = new Size(175, 22);
            loadConfigurationToolStripMenuItem.Text = "Load configuration";
            loadConfigurationToolStripMenuItem.Click += loadConfigurationToolStripMenuItem_Click;
            // 
            // saveConfigurationToolStripMenuItem
            // 
            saveConfigurationToolStripMenuItem.Name = "saveConfigurationToolStripMenuItem";
            saveConfigurationToolStripMenuItem.Size = new Size(175, 22);
            saveConfigurationToolStripMenuItem.Text = "Save configuration";
            saveConfigurationToolStripMenuItem.Click += saveConfigurationToolStripMenuItem_Click;
            // 
            // toolStripSeparator1
            // 
            toolStripSeparator1.Name = "toolStripSeparator1";
            toolStripSeparator1.Size = new Size(172, 6);
            // 
            // exitToolStripMenuItem
            // 
            exitToolStripMenuItem.Name = "exitToolStripMenuItem";
            exitToolStripMenuItem.Size = new Size(175, 22);
            exitToolStripMenuItem.Text = "Exit";
            exitToolStripMenuItem.Click += exitToolStripMenuItem_Click;
            // 
            // viewToolStripMenuItem
            // 
            viewToolStripMenuItem.DropDownItems.AddRange(new ToolStripItem[] { showHiddenComponentsToolStripMenuItem, componentsInsteadOfDirectObjectAccessToolStripMenuItem });
            viewToolStripMenuItem.Name = "viewToolStripMenuItem";
            viewToolStripMenuItem.Size = new Size(44, 20);
            viewToolStripMenuItem.Text = "View";
            // 
            // showHiddenComponentsToolStripMenuItem
            // 
            showHiddenComponentsToolStripMenuItem.CheckOnClick = true;
            showHiddenComponentsToolStripMenuItem.Name = "showHiddenComponentsToolStripMenuItem";
            showHiddenComponentsToolStripMenuItem.Size = new Size(304, 22);
            showHiddenComponentsToolStripMenuItem.Text = "Show hidden components";
            showHiddenComponentsToolStripMenuItem.Click += showHiddenComponentsToolStripMenuItem_Click;
            // 
            // componentsInsteadOfDirectObjectAccessToolStripMenuItem
            // 
            componentsInsteadOfDirectObjectAccessToolStripMenuItem.CheckOnClick = true;
            componentsInsteadOfDirectObjectAccessToolStripMenuItem.Name = "componentsInsteadOfDirectObjectAccessToolStripMenuItem";
            componentsInsteadOfDirectObjectAccessToolStripMenuItem.Size = new Size(304, 22);
            componentsInsteadOfDirectObjectAccessToolStripMenuItem.Text = "Components instead of direct object access";
            componentsInsteadOfDirectObjectAccessToolStripMenuItem.Click += componentsInsteadOfDirectObjectAccessToolStripMenuItem_Click;
            // 
            // toolStrip1
            // 
            toolStrip1.Items.AddRange(new ToolStripItem[] { btnAddDevice, btnAddFunctionBlock, btnRefresh });
            toolStrip1.Location = new Point(0, 24);
            toolStrip1.Name = "toolStrip1";
            toolStrip1.Size = new Size(784, 25);
            toolStrip1.TabIndex = 5;
            toolStrip1.Text = "toolStrip1";
            // 
            // btnAddDevice
            // 
            btnAddDevice.Name = "btnAddDevice";
            btnAddDevice.Size = new Size(70, 22);
            btnAddDevice.Text = "Add device";
            btnAddDevice.Click += btnAddDevice_Click;
            // 
            // btnAddFunctionBlock
            // 
            btnAddFunctionBlock.Name = "btnAddFunctionBlock";
            btnAddFunctionBlock.Size = new Size(113, 22);
            btnAddFunctionBlock.Text = "Add function block";
            btnAddFunctionBlock.Click += btnAddFunctionBlock_Click;
            // 
            // btnRefresh
            // 
            btnRefresh.Name = "btnRefresh";
            btnRefresh.Size = new Size(50, 22);
            btnRefresh.Text = "Refresh";
            btnRefresh.Click += btnRefresh_Click;
            // 
            // tabControl1
            // 
            tabControl1.Controls.Add(tabSystemOverview);
            tabControl1.Controls.Add(tabSignals);
            tabControl1.Controls.Add(tabChannels);
            tabControl1.Controls.Add(tabFunctionBlocks);
            tabControl1.Controls.Add(tabFullTopology);
            tabControl1.Dock = DockStyle.Top;
            tabControl1.Location = new Point(0, 49);
            tabControl1.Name = "tabControl1";
            tabControl1.SelectedIndex = 0;
            tabControl1.Size = new Size(784, 23);
            tabControl1.TabIndex = 6;
            tabControl1.Selected += tabControl1_Selected;
            // 
            // tabSystemOverview
            // 
            tabSystemOverview.Location = new Point(4, 24);
            tabSystemOverview.Name = "tabSystemOverview";
            tabSystemOverview.Padding = new Padding(3);
            tabSystemOverview.Size = new Size(776, 0);
            tabSystemOverview.TabIndex = 0;
            tabSystemOverview.Text = "System overview";
            tabSystemOverview.UseVisualStyleBackColor = true;
            // 
            // tabSignals
            // 
            tabSignals.Location = new Point(4, 24);
            tabSignals.Name = "tabSignals";
            tabSignals.Padding = new Padding(3);
            tabSignals.Size = new Size(776, 0);
            tabSignals.TabIndex = 1;
            tabSignals.Text = "Signals";
            tabSignals.UseVisualStyleBackColor = true;
            // 
            // tabChannels
            // 
            tabChannels.Location = new Point(4, 24);
            tabChannels.Name = "tabChannels";
            tabChannels.Size = new Size(776, 0);
            tabChannels.TabIndex = 2;
            tabChannels.Text = "Channels";
            tabChannels.UseVisualStyleBackColor = true;
            // 
            // tabFunctionBlocks
            // 
            tabFunctionBlocks.Location = new Point(4, 24);
            tabFunctionBlocks.Name = "tabFunctionBlocks";
            tabFunctionBlocks.Size = new Size(776, 0);
            tabFunctionBlocks.TabIndex = 3;
            tabFunctionBlocks.Text = "Function blocks";
            tabFunctionBlocks.UseVisualStyleBackColor = true;
            // 
            // tabFullTopology
            // 
            tabFullTopology.Location = new Point(4, 24);
            tabFullTopology.Name = "tabFullTopology";
            tabFullTopology.Size = new Size(776, 0);
            tabFullTopology.TabIndex = 4;
            tabFullTopology.Text = "Full topology";
            tabFullTopology.UseVisualStyleBackColor = true;
            // 
            // splitContainer1
            // 
            splitContainer1.Dock = DockStyle.Fill;
            splitContainer1.Location = new Point(0, 72);
            splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            splitContainer1.Panel1.Controls.Add(treeComponents);
            splitContainer1.Panel1.Padding = new Padding(3, 3, 0, 3);
            // 
            // splitContainer1.Panel2
            // 
            splitContainer1.Panel2.Controls.Add(gridProperties);
            splitContainer1.Panel2.Padding = new Padding(0, 3, 3, 3);
            splitContainer1.Size = new Size(784, 548);
            splitContainer1.SplitterDistance = 300;
            splitContainer1.TabIndex = 0;
            // 
            // gridProperties
            // 
            gridProperties.AllowUserToAddRows = false;
            gridProperties.AllowUserToDeleteRows = false;
            gridProperties.BackgroundColor = SystemColors.Window;
            gridProperties.ContextMenuStrip = contextMenuStripGridProperties;
            gridProperties.Dock = DockStyle.Fill;
            gridProperties.Location = new Point(0, 3);
            gridProperties.Name = "gridProperties";
            gridProperties.ReadOnly = true;
            gridProperties.RowTemplate.Height = 25;
            gridProperties.Size = new Size(477, 542);
            gridProperties.TabIndex = 4;
            gridProperties.CellContextMenuStripNeeded += gridProperties_CellContextMenuStripNeeded;
            gridProperties.CellDoubleClick += gridProperties_CellDoubleClick;
            // 
            // contextMenuStripGridProperties
            // 
            contextMenuStripGridProperties.Items.AddRange(new ToolStripItem[] { conetxtMenuItemGridPropertiesEdit });
            contextMenuStripGridProperties.Name = "contextMenuStrip1";
            contextMenuStripGridProperties.Size = new Size(193, 26);
            contextMenuStripGridProperties.Opening += contextMenuStripGridProperties_Opening;
            // 
            // conetxtMenuItemGridPropertiesEdit
            // 
            conetxtMenuItemGridPropertiesEdit.Name = "conetxtMenuItemGridPropertiesEdit";
            conetxtMenuItemGridPropertiesEdit.ShortcutKeyDisplayString = "<double-click>";
            conetxtMenuItemGridPropertiesEdit.Size = new Size(192, 22);
            conetxtMenuItemGridPropertiesEdit.Text = "Edit...";
            conetxtMenuItemGridPropertiesEdit.Click += conetxtMenuItemGridPropertiesEdit_Click;
            // 
            // imglTreeImages
            // 
            imglTreeImages.ColorDepth = ColorDepth.Depth8Bit;
            imglTreeImages.ImageSize = new Size(16, 16);
            imglTreeImages.TransparentColor = Color.Transparent;
            // 
            // frmMain
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(784, 620);
            Controls.Add(splitContainer1);
            Controls.Add(tabControl1);
            Controls.Add(toolStrip1);
            Controls.Add(menuStrip1);
            MinimumSize = new Size(800, 400);
            Name = "frmMain";
            StartPosition = FormStartPosition.CenterScreen;
            Text = "openDAQ Demo .NET";
            FormClosing += frmMain_FormClosing;
            Load += frmMain_Load;
            Shown += frmMain_Shown;
            contextMenuStripTreeComponents.ResumeLayout(false);
            menuStrip1.ResumeLayout(false);
            menuStrip1.PerformLayout();
            toolStrip1.ResumeLayout(false);
            toolStrip1.PerformLayout();
            tabControl1.ResumeLayout(false);
            splitContainer1.Panel1.ResumeLayout(false);
            splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)splitContainer1).EndInit();
            splitContainer1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)gridProperties).EndInit();
            contextMenuStripGridProperties.ResumeLayout(false);
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion
        private TreeView treeComponents;
        private MenuStrip menuStrip1;
        private ToolStripMenuItem fileToolStripMenuItem;
        private ToolStripMenuItem viewToolStripMenuItem;
        private ToolStrip toolStrip1;
        private ToolStripButton btnAddDevice;
        private ToolStripButton btnAddFunctionBlock;
        private ToolStripButton btnRefresh;
        private TabControl tabControl1;
        private TabPage tabSystemOverview;
        private TabPage tabSignals;
        private TabPage tabChannels;
        private TabPage tabFunctionBlocks;
        private TabPage tabFullTopology;
        private SplitContainer splitContainer1;
        private ImageList imglTreeImages;
        private ToolStripMenuItem loadConfigurationToolStripMenuItem;
        private ToolStripMenuItem saveConfigurationToolStripMenuItem;
        private ToolStripSeparator toolStripSeparator1;
        private ToolStripMenuItem exitToolStripMenuItem;
        private ToolStripMenuItem showHiddenComponentsToolStripMenuItem;
        private ToolStripMenuItem componentsInsteadOfDirectObjectAccessToolStripMenuItem;
        private ContextMenuStrip contextMenuStripTreeComponents;
        private ToolStripMenuItem contextMenuItemTreeComponentsRemove;
        private DataGridView gridProperties;
        private ContextMenuStrip contextMenuStripGridProperties;
        private ToolStripMenuItem conetxtMenuItemGridPropertiesEdit;
    }
}
