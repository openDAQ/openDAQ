namespace openDAQDemoNet
{
    partial class frmAddDeviceDialog
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
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
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmAddDeviceDialog));
            splitContainer1 = new SplitContainer();
            treeParentDevices = new TreeView();
            textBox1 = new TextBox();
            gridChildDevices = new DataGridView();
            contextMenuStripChildDevices = new ContextMenuStrip(components);
            contextMenuItemChildDevicesAddDevice = new ToolStripMenuItem();
            toolStripSeparator1 = new ToolStripSeparator();
            contextMenuItemChildDevicesRefresh = new ToolStripMenuItem();
            panel1 = new Panel();
            label1 = new Label();
            btnAdd = new Button();
            txtConnectionString = new TextBox();
            ((System.ComponentModel.ISupportInitialize)splitContainer1).BeginInit();
            splitContainer1.Panel1.SuspendLayout();
            splitContainer1.Panel2.SuspendLayout();
            splitContainer1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)gridChildDevices).BeginInit();
            contextMenuStripChildDevices.SuspendLayout();
            panel1.SuspendLayout();
            SuspendLayout();
            // 
            // splitContainer1
            // 
            splitContainer1.Dock = DockStyle.Fill;
            splitContainer1.Location = new Point(0, 0);
            splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            splitContainer1.Panel1.Controls.Add(treeParentDevices);
            splitContainer1.Panel1.Controls.Add(textBox1);
            // 
            // splitContainer1.Panel2
            // 
            splitContainer1.Panel2.Controls.Add(gridChildDevices);
            splitContainer1.Panel2.Controls.Add(panel1);
            splitContainer1.Size = new Size(984, 461);
            splitContainer1.SplitterDistance = 327;
            splitContainer1.TabIndex = 0;
            // 
            // treeParentDevices
            // 
            treeParentDevices.Dock = DockStyle.Fill;
            treeParentDevices.Location = new Point(0, 23);
            treeParentDevices.Name = "treeParentDevices";
            treeParentDevices.Size = new Size(327, 438);
            treeParentDevices.TabIndex = 0;
            // 
            // textBox1
            // 
            textBox1.Dock = DockStyle.Top;
            textBox1.Font = new Font("Segoe UI", 9F, FontStyle.Bold, GraphicsUnit.Point);
            textBox1.Location = new Point(0, 0);
            textBox1.Name = "textBox1";
            textBox1.ReadOnly = true;
            textBox1.Size = new Size(327, 23);
            textBox1.TabIndex = 1;
            textBox1.Text = "Parent device";
            textBox1.TextAlign = HorizontalAlignment.Center;
            // 
            // gridChildDevices
            // 
            gridChildDevices.AllowUserToAddRows = false;
            gridChildDevices.AllowUserToDeleteRows = false;
            gridChildDevices.BackgroundColor = SystemColors.Window;
            gridChildDevices.ContextMenuStrip = contextMenuStripChildDevices;
            gridChildDevices.Dock = DockStyle.Fill;
            gridChildDevices.Location = new Point(0, 0);
            gridChildDevices.Name = "gridChildDevices";
            gridChildDevices.ReadOnly = true;
            gridChildDevices.RowTemplate.Height = 25;
            gridChildDevices.Size = new Size(653, 432);
            gridChildDevices.TabIndex = 0;
            gridChildDevices.CellContextMenuStripNeeded += gridChildDevices_CellContextMenuStripNeeded;
            gridChildDevices.CellDoubleClick += gridChildDevices_CellDoubleClick;
            gridChildDevices.SelectionChanged += gridChildDevices_SelectionChanged;
            // 
            // contextMenuStripChildDevices
            // 
            contextMenuStripChildDevices.Items.AddRange(new ToolStripItem[] { contextMenuItemChildDevicesAddDevice, toolStripSeparator1, contextMenuItemChildDevicesRefresh });
            contextMenuStripChildDevices.Name = "contextMenuStrip1";
            contextMenuStripChildDevices.Size = new Size(223, 76);
            contextMenuStripChildDevices.Opening += contextMenuStripChildDevices_Opening;
            // 
            // contextMenuItemChildDevicesAddDevice
            // 
            contextMenuItemChildDevicesAddDevice.Name = "contextMenuItemChildDevicesAddDevice";
            contextMenuItemChildDevicesAddDevice.ShortcutKeyDisplayString = "<double-click>";
            contextMenuItemChildDevicesAddDevice.Size = new Size(222, 22);
            contextMenuItemChildDevicesAddDevice.Text = "Add device";
            contextMenuItemChildDevicesAddDevice.Click += contextMenuItemChildDevicesAddDevice_Click;
            // 
            // toolStripSeparator1
            // 
            toolStripSeparator1.Name = "toolStripSeparator1";
            toolStripSeparator1.Size = new Size(219, 6);
            // 
            // contextMenuItemChildDevicesRefresh
            // 
            contextMenuItemChildDevicesRefresh.Image = Properties.Resources.refresh16;
            contextMenuItemChildDevicesRefresh.Name = "contextMenuItemChildDevicesRefresh";
            contextMenuItemChildDevicesRefresh.Size = new Size(222, 22);
            contextMenuItemChildDevicesRefresh.Text = "Refresh list";
            contextMenuItemChildDevicesRefresh.Click += contextMenuItemChildDevicesRefresh_Click;
            // 
            // panel1
            // 
            panel1.Controls.Add(label1);
            panel1.Controls.Add(btnAdd);
            panel1.Controls.Add(txtConnectionString);
            panel1.Dock = DockStyle.Bottom;
            panel1.Location = new Point(0, 432);
            panel1.Name = "panel1";
            panel1.Size = new Size(653, 29);
            panel1.TabIndex = 2;
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new Point(3, 6);
            label1.Name = "label1";
            label1.Size = new Size(102, 15);
            label1.TabIndex = 4;
            label1.Text = "Connection string";
            // 
            // btnAdd
            // 
            btnAdd.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            btnAdd.Location = new Point(600, 3);
            btnAdd.Name = "btnAdd";
            btnAdd.Size = new Size(50, 23);
            btnAdd.TabIndex = 3;
            btnAdd.Text = "Add";
            btnAdd.UseVisualStyleBackColor = true;
            btnAdd.Click += btnAdd_Click;
            // 
            // txtConnectionString
            // 
            txtConnectionString.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            txtConnectionString.Location = new Point(111, 3);
            txtConnectionString.Name = "txtConnectionString";
            txtConnectionString.Size = new Size(486, 23);
            txtConnectionString.TabIndex = 2;
            // 
            // frmAddDeviceDialog
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(984, 461);
            Controls.Add(splitContainer1);
            Icon = (Icon)resources.GetObject("$this.Icon");
            MinimumSize = new Size(1000, 500);
            Name = "frmAddDeviceDialog";
            StartPosition = FormStartPosition.CenterParent;
            Text = "Add device";
            Shown += frmAddDeviceDialog_Shown;
            splitContainer1.Panel1.ResumeLayout(false);
            splitContainer1.Panel1.PerformLayout();
            splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)splitContainer1).EndInit();
            splitContainer1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)gridChildDevices).EndInit();
            contextMenuStripChildDevices.ResumeLayout(false);
            panel1.ResumeLayout(false);
            panel1.PerformLayout();
            ResumeLayout(false);
        }

        #endregion

        private SplitContainer splitContainer1;
        private TextBox textBox1;
        private TreeView treeParentDevices;
        private DataGridView gridChildDevices;
        private Panel panel1;
        private Button btnAdd;
        private TextBox txtConnectionString;
        private Label label1;
        private ContextMenuStrip contextMenuStripChildDevices;
        private ToolStripMenuItem contextMenuItemChildDevicesRefresh;
        private ToolStripMenuItem contextMenuItemChildDevicesAddDevice;
        private ToolStripSeparator toolStripSeparator1;
    }
}
