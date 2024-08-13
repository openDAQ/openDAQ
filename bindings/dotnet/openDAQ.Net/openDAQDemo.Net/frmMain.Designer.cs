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
            TreeNode treeNode2 = new TreeNode("treeComponents");
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
            splitContainerMain = new SplitContainer();
            splitContainerPropertiesAttributes = new SplitContainer();
            splitContainerPropertiesInputs = new SplitContainer();
            gridProperties = new DataGridView();
            contextMenuStripGridProperties = new ContextMenuStrip(components);
            conetxtMenuItemGridPropertiesEdit = new ToolStripMenuItem();
            groupInputPorts = new GroupBox();
            tableInputPorts = new TableLayoutPanel();
            label1 = new Label();
            comboBox1 = new ComboBox();
            button1 = new Button();
            button2 = new Button();
            lblNoInputPorts = new Label();
            groupOutputSignals = new GroupBox();
            tableOutputSignals = new TableLayoutPanel();
            label2 = new Label();
            button3 = new Button();
            label3 = new Label();
            lblNoOutputSignals = new Label();
            gridAttributes = new DataGridView();
            imglTreeImages = new ImageList(components);
            contextMenuStripTreeComponents.SuspendLayout();
            menuStrip1.SuspendLayout();
            toolStrip1.SuspendLayout();
            tabControl1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)splitContainerMain).BeginInit();
            splitContainerMain.Panel1.SuspendLayout();
            splitContainerMain.Panel2.SuspendLayout();
            splitContainerMain.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)splitContainerPropertiesAttributes).BeginInit();
            splitContainerPropertiesAttributes.Panel1.SuspendLayout();
            splitContainerPropertiesAttributes.Panel2.SuspendLayout();
            splitContainerPropertiesAttributes.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)splitContainerPropertiesInputs).BeginInit();
            splitContainerPropertiesInputs.Panel1.SuspendLayout();
            splitContainerPropertiesInputs.Panel2.SuspendLayout();
            splitContainerPropertiesInputs.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)gridProperties).BeginInit();
            contextMenuStripGridProperties.SuspendLayout();
            groupInputPorts.SuspendLayout();
            tableInputPorts.SuspendLayout();
            groupOutputSignals.SuspendLayout();
            tableOutputSignals.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)gridAttributes).BeginInit();
            SuspendLayout();
            // 
            // treeComponents
            // 
            treeComponents.ContextMenuStrip = contextMenuStripTreeComponents;
            treeComponents.Dock = DockStyle.Fill;
            treeComponents.Location = new Point(3, 3);
            treeComponents.Name = "treeComponents";
            treeNode2.Name = "Node0";
            treeNode2.Text = "treeComponents";
            treeComponents.Nodes.AddRange(new TreeNode[] { treeNode2 });
            treeComponents.Size = new Size(397, 683);
            treeComponents.TabIndex = 2;
            treeComponents.AfterSelect += treeComponents_AfterSelect;
            treeComponents.NodeMouseClick += treeComponents_NodeMouseClick;
            // 
            // contextMenuStripTreeComponents
            // 
            contextMenuStripTreeComponents.Items.AddRange(new ToolStripItem[] { contextMenuItemTreeComponentsRemove });
            contextMenuStripTreeComponents.Name = "contextMenuStrip1";
            contextMenuStripTreeComponents.Size = new Size(118, 26);
            contextMenuStripTreeComponents.Opening += contextMenuStripTreeComponents_Opening;
            // 
            // contextMenuItemTreeComponentsRemove
            // 
            contextMenuItemTreeComponentsRemove.Name = "contextMenuItemTreeComponentsRemove";
            contextMenuItemTreeComponentsRemove.Size = new Size(117, 22);
            contextMenuItemTreeComponentsRemove.Text = "Remove";
            contextMenuItemTreeComponentsRemove.Click += contextMenuItemTreeComponentsRemove_Click;
            // 
            // menuStrip1
            // 
            menuStrip1.Items.AddRange(new ToolStripItem[] { fileToolStripMenuItem, viewToolStripMenuItem });
            menuStrip1.Location = new Point(0, 0);
            menuStrip1.Name = "menuStrip1";
            menuStrip1.Size = new Size(984, 24);
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
            toolStrip1.Size = new Size(984, 25);
            toolStrip1.TabIndex = 5;
            toolStrip1.Text = "toolStrip1";
            // 
            // btnAddDevice
            // 
            btnAddDevice.Image = Properties.Resources.device16;
            btnAddDevice.Name = "btnAddDevice";
            btnAddDevice.Size = new Size(86, 22);
            btnAddDevice.Text = "Add device";
            btnAddDevice.Click += btnAddDevice_Click;
            // 
            // btnAddFunctionBlock
            // 
            btnAddFunctionBlock.Image = Properties.Resources.function_block16;
            btnAddFunctionBlock.Name = "btnAddFunctionBlock";
            btnAddFunctionBlock.Size = new Size(129, 22);
            btnAddFunctionBlock.Text = "Add function block";
            btnAddFunctionBlock.Click += btnAddFunctionBlock_Click;
            // 
            // btnRefresh
            // 
            btnRefresh.Image = Properties.Resources.refresh16;
            btnRefresh.Name = "btnRefresh";
            btnRefresh.Size = new Size(66, 22);
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
            tabControl1.Size = new Size(984, 23);
            tabControl1.TabIndex = 6;
            tabControl1.Selected += tabControl1_Selected;
            // 
            // tabSystemOverview
            // 
            tabSystemOverview.Location = new Point(4, 24);
            tabSystemOverview.Name = "tabSystemOverview";
            tabSystemOverview.Padding = new Padding(3);
            tabSystemOverview.Size = new Size(976, 0);
            tabSystemOverview.TabIndex = 0;
            tabSystemOverview.Text = "System overview";
            tabSystemOverview.UseVisualStyleBackColor = true;
            // 
            // tabSignals
            // 
            tabSignals.Location = new Point(4, 24);
            tabSignals.Name = "tabSignals";
            tabSignals.Padding = new Padding(3);
            tabSignals.Size = new Size(976, 0);
            tabSignals.TabIndex = 1;
            tabSignals.Text = "Signals";
            tabSignals.UseVisualStyleBackColor = true;
            // 
            // tabChannels
            // 
            tabChannels.Location = new Point(4, 24);
            tabChannels.Name = "tabChannels";
            tabChannels.Size = new Size(976, 0);
            tabChannels.TabIndex = 2;
            tabChannels.Text = "Channels";
            tabChannels.UseVisualStyleBackColor = true;
            // 
            // tabFunctionBlocks
            // 
            tabFunctionBlocks.Location = new Point(4, 24);
            tabFunctionBlocks.Name = "tabFunctionBlocks";
            tabFunctionBlocks.Size = new Size(976, 0);
            tabFunctionBlocks.TabIndex = 3;
            tabFunctionBlocks.Text = "Function blocks";
            tabFunctionBlocks.UseVisualStyleBackColor = true;
            // 
            // tabFullTopology
            // 
            tabFullTopology.Location = new Point(4, 24);
            tabFullTopology.Name = "tabFullTopology";
            tabFullTopology.Size = new Size(976, 0);
            tabFullTopology.TabIndex = 4;
            tabFullTopology.Text = "Full topology";
            tabFullTopology.UseVisualStyleBackColor = true;
            // 
            // splitContainerMain
            // 
            splitContainerMain.Dock = DockStyle.Fill;
            splitContainerMain.FixedPanel = FixedPanel.Panel1;
            splitContainerMain.Location = new Point(0, 72);
            splitContainerMain.Name = "splitContainerMain";
            // 
            // splitContainerMain.Panel1
            // 
            splitContainerMain.Panel1.Controls.Add(treeComponents);
            splitContainerMain.Panel1.Padding = new Padding(3, 3, 0, 3);
            // 
            // splitContainerMain.Panel2
            // 
            splitContainerMain.Panel2.Controls.Add(splitContainerPropertiesAttributes);
            splitContainerMain.Panel2.Padding = new Padding(0, 3, 3, 3);
            splitContainerMain.Size = new Size(984, 689);
            splitContainerMain.SplitterDistance = 400;
            splitContainerMain.TabIndex = 0;
            // 
            // splitContainerPropertiesAttributes
            // 
            splitContainerPropertiesAttributes.Dock = DockStyle.Fill;
            splitContainerPropertiesAttributes.FixedPanel = FixedPanel.Panel2;
            splitContainerPropertiesAttributes.Location = new Point(0, 3);
            splitContainerPropertiesAttributes.Name = "splitContainerPropertiesAttributes";
            splitContainerPropertiesAttributes.Orientation = Orientation.Horizontal;
            // 
            // splitContainerPropertiesAttributes.Panel1
            // 
            splitContainerPropertiesAttributes.Panel1.Controls.Add(splitContainerPropertiesInputs);
            // 
            // splitContainerPropertiesAttributes.Panel2
            // 
            splitContainerPropertiesAttributes.Panel2.Controls.Add(gridAttributes);
            splitContainerPropertiesAttributes.Size = new Size(577, 683);
            splitContainerPropertiesAttributes.SplitterDistance = 400;
            splitContainerPropertiesAttributes.TabIndex = 6;
            // 
            // splitContainerPropertiesInputs
            // 
            splitContainerPropertiesInputs.Dock = DockStyle.Fill;
            splitContainerPropertiesInputs.FixedPanel = FixedPanel.Panel1;
            splitContainerPropertiesInputs.Location = new Point(0, 0);
            splitContainerPropertiesInputs.Name = "splitContainerPropertiesInputs";
            // 
            // splitContainerPropertiesInputs.Panel1
            // 
            splitContainerPropertiesInputs.Panel1.Controls.Add(gridProperties);
            // 
            // splitContainerPropertiesInputs.Panel2
            // 
            splitContainerPropertiesInputs.Panel2.Controls.Add(groupInputPorts);
            splitContainerPropertiesInputs.Panel2.Controls.Add(groupOutputSignals);
            splitContainerPropertiesInputs.Size = new Size(577, 400);
            splitContainerPropertiesInputs.SplitterDistance = 300;
            splitContainerPropertiesInputs.TabIndex = 8;
            // 
            // gridProperties
            // 
            gridProperties.AllowUserToAddRows = false;
            gridProperties.AllowUserToDeleteRows = false;
            gridProperties.BackgroundColor = SystemColors.Window;
            gridProperties.ContextMenuStrip = contextMenuStripGridProperties;
            gridProperties.Dock = DockStyle.Fill;
            gridProperties.Location = new Point(0, 0);
            gridProperties.MultiSelect = false;
            gridProperties.Name = "gridProperties";
            gridProperties.ReadOnly = true;
            gridProperties.RowTemplate.Height = 25;
            gridProperties.Size = new Size(300, 400);
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
            // groupInputPorts
            // 
            groupInputPorts.Controls.Add(tableInputPorts);
            groupInputPorts.Controls.Add(lblNoInputPorts);
            groupInputPorts.Dock = DockStyle.Fill;
            groupInputPorts.Location = new Point(0, 0);
            groupInputPorts.Name = "groupInputPorts";
            groupInputPorts.Size = new Size(273, 316);
            groupInputPorts.TabIndex = 6;
            groupInputPorts.TabStop = false;
            groupInputPorts.Text = "Input ports";
            // 
            // tableInputPorts
            // 
            tableInputPorts.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            tableInputPorts.AutoSize = true;
            tableInputPorts.ColumnCount = 4;
            tableInputPorts.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 75F));
            tableInputPorts.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            tableInputPorts.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 40F));
            tableInputPorts.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 40F));
            tableInputPorts.Controls.Add(label1, 0, 0);
            tableInputPorts.Controls.Add(comboBox1, 1, 0);
            tableInputPorts.Controls.Add(button1, 2, 0);
            tableInputPorts.Controls.Add(button2, 3, 0);
            tableInputPorts.Location = new Point(3, 22);
            tableInputPorts.Name = "tableInputPorts";
            tableInputPorts.RowCount = 1;
            tableInputPorts.RowStyles.Add(new RowStyle(SizeType.Absolute, 40F));
            tableInputPorts.Size = new Size(262, 40);
            tableInputPorts.TabIndex = 0;
            // 
            // label1
            // 
            label1.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            label1.AutoSize = true;
            label1.Location = new Point(3, 12);
            label1.Name = "label1";
            label1.Size = new Size(69, 15);
            label1.TabIndex = 0;
            label1.Text = "name";
            label1.TextAlign = ContentAlignment.MiddleRight;
            // 
            // comboBox1
            // 
            comboBox1.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            comboBox1.FormattingEnabled = true;
            comboBox1.Location = new Point(78, 8);
            comboBox1.Name = "comboBox1";
            comboBox1.Size = new Size(101, 23);
            comboBox1.TabIndex = 1;
            // 
            // button1
            // 
            button1.Anchor = AnchorStyles.Left;
            button1.Location = new Point(185, 3);
            button1.Name = "button1";
            button1.Size = new Size(34, 34);
            button1.TabIndex = 2;
            button1.UseVisualStyleBackColor = true;
            // 
            // button2
            // 
            button2.Anchor = AnchorStyles.Left;
            button2.Location = new Point(225, 3);
            button2.Name = "button2";
            button2.Size = new Size(34, 34);
            button2.TabIndex = 2;
            button2.UseVisualStyleBackColor = true;
            // 
            // lblNoInputPorts
            // 
            lblNoInputPorts.AutoSize = true;
            lblNoInputPorts.Location = new Point(92, 4);
            lblNoInputPorts.Name = "lblNoInputPorts";
            lblNoInputPorts.Size = new Size(84, 15);
            lblNoInputPorts.TabIndex = 1;
            lblNoInputPorts.Text = "No input ports";
            lblNoInputPorts.TextAlign = ContentAlignment.MiddleCenter;
            // 
            // groupOutputSignals
            // 
            groupOutputSignals.AutoSize = true;
            groupOutputSignals.AutoSizeMode = AutoSizeMode.GrowAndShrink;
            groupOutputSignals.Controls.Add(tableOutputSignals);
            groupOutputSignals.Controls.Add(lblNoOutputSignals);
            groupOutputSignals.Dock = DockStyle.Bottom;
            groupOutputSignals.Location = new Point(0, 316);
            groupOutputSignals.Name = "groupOutputSignals";
            groupOutputSignals.Size = new Size(273, 84);
            groupOutputSignals.TabIndex = 6;
            groupOutputSignals.TabStop = false;
            groupOutputSignals.Text = "Output signals";
            // 
            // tableOutputSignals
            // 
            tableOutputSignals.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            tableOutputSignals.AutoSize = true;
            tableOutputSignals.ColumnCount = 3;
            tableOutputSignals.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 75F));
            tableOutputSignals.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            tableOutputSignals.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 40F));
            tableOutputSignals.Controls.Add(label2, 0, 0);
            tableOutputSignals.Controls.Add(button3, 2, 0);
            tableOutputSignals.Controls.Add(label3, 1, 0);
            tableOutputSignals.Location = new Point(3, 22);
            tableOutputSignals.Name = "tableOutputSignals";
            tableOutputSignals.RowCount = 1;
            tableOutputSignals.RowStyles.Add(new RowStyle(SizeType.Absolute, 40F));
            tableOutputSignals.Size = new Size(262, 40);
            tableOutputSignals.TabIndex = 0;
            // 
            // label2
            // 
            label2.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            label2.AutoSize = true;
            label2.Location = new Point(3, 12);
            label2.Name = "label2";
            label2.Size = new Size(69, 15);
            label2.TabIndex = 0;
            label2.Text = "name";
            label2.TextAlign = ContentAlignment.MiddleRight;
            // 
            // button3
            // 
            button3.Anchor = AnchorStyles.Left;
            button3.Location = new Point(225, 3);
            button3.Name = "button3";
            button3.Size = new Size(34, 34);
            button3.TabIndex = 2;
            button3.UseVisualStyleBackColor = true;
            // 
            // label3
            // 
            label3.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            label3.AutoSize = true;
            label3.Location = new Point(78, 12);
            label3.Name = "label3";
            label3.Size = new Size(141, 15);
            label3.TabIndex = 0;
            label3.Text = "vlaue";
            label3.TextAlign = ContentAlignment.MiddleCenter;
            // 
            // lblNoOutputSignals
            // 
            lblNoOutputSignals.AutoSize = true;
            lblNoOutputSignals.Location = new Point(92, 4);
            lblNoOutputSignals.Name = "lblNoOutputSignals";
            lblNoOutputSignals.Size = new Size(101, 15);
            lblNoOutputSignals.TabIndex = 2;
            lblNoOutputSignals.Text = "No output signals";
            lblNoOutputSignals.TextAlign = ContentAlignment.MiddleCenter;
            // 
            // gridAttributes
            // 
            gridAttributes.AllowUserToAddRows = false;
            gridAttributes.AllowUserToDeleteRows = false;
            gridAttributes.BackgroundColor = SystemColors.Window;
            gridAttributes.ContextMenuStrip = contextMenuStripGridProperties;
            gridAttributes.Dock = DockStyle.Fill;
            gridAttributes.Location = new Point(0, 0);
            gridAttributes.MultiSelect = false;
            gridAttributes.Name = "gridAttributes";
            gridAttributes.ReadOnly = true;
            gridAttributes.RowTemplate.Height = 25;
            gridAttributes.Size = new Size(577, 279);
            gridAttributes.TabIndex = 5;
            gridAttributes.CellContextMenuStripNeeded += gridProperties_CellContextMenuStripNeeded;
            gridAttributes.CellDoubleClick += gridAttributes_CellDoubleClick;
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
            ClientSize = new Size(984, 761);
            Controls.Add(splitContainerMain);
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
            splitContainerMain.Panel1.ResumeLayout(false);
            splitContainerMain.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)splitContainerMain).EndInit();
            splitContainerMain.ResumeLayout(false);
            splitContainerPropertiesAttributes.Panel1.ResumeLayout(false);
            splitContainerPropertiesAttributes.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)splitContainerPropertiesAttributes).EndInit();
            splitContainerPropertiesAttributes.ResumeLayout(false);
            splitContainerPropertiesInputs.Panel1.ResumeLayout(false);
            splitContainerPropertiesInputs.Panel2.ResumeLayout(false);
            splitContainerPropertiesInputs.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)splitContainerPropertiesInputs).EndInit();
            splitContainerPropertiesInputs.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)gridProperties).EndInit();
            contextMenuStripGridProperties.ResumeLayout(false);
            groupInputPorts.ResumeLayout(false);
            groupInputPorts.PerformLayout();
            tableInputPorts.ResumeLayout(false);
            tableInputPorts.PerformLayout();
            groupOutputSignals.ResumeLayout(false);
            groupOutputSignals.PerformLayout();
            tableOutputSignals.ResumeLayout(false);
            tableOutputSignals.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)gridAttributes).EndInit();
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
        private SplitContainer splitContainerMain;
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
        private DataGridView gridAttributes;
        private SplitContainer splitContainerPropertiesAttributes;
        private GroupBox groupInputPorts;
        private TableLayoutPanel tableInputPorts;
        private Label label1;
        private ComboBox comboBox1;
        private Button button1;
        private SplitContainer splitContainerPropertiesInputs;
        private GroupBox groupOutputSignals;
        private TableLayoutPanel tableOutputSignals;
        private Label label2;
        private Button button3;
        private Label label3;
        private Button button2;
        private Label lblNoInputPorts;
        private Label lblNoOutputSignals;
    }
}
