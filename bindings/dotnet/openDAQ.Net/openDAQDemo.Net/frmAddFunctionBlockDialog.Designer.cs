namespace openDAQDemoNet
{
    partial class frmAddFunctionBlockDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmAddFunctionBlockDialog));
            splitContainer1 = new SplitContainer();
            treeParentDevices = new TreeView();
            textBox1 = new TextBox();
            gridFunctionBlocks = new DataGridView();
            contextMenuStripFunctionBlocks = new ContextMenuStrip(components);
            contextMenuItemChildDevicesAddDevice = new ToolStripMenuItem();
            toolStripSeparator1 = new ToolStripSeparator();
            contextMenuItemChildDevicesRefresh = new ToolStripMenuItem();
            ((System.ComponentModel.ISupportInitialize)splitContainer1).BeginInit();
            splitContainer1.Panel1.SuspendLayout();
            splitContainer1.Panel2.SuspendLayout();
            splitContainer1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)gridFunctionBlocks).BeginInit();
            contextMenuStripFunctionBlocks.SuspendLayout();
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
            splitContainer1.Panel2.Controls.Add(gridFunctionBlocks);
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
            // gridFunctionBlocks
            // 
            gridFunctionBlocks.AllowUserToAddRows = false;
            gridFunctionBlocks.AllowUserToDeleteRows = false;
            gridFunctionBlocks.BackgroundColor = SystemColors.Window;
            gridFunctionBlocks.ContextMenuStrip = contextMenuStripFunctionBlocks;
            gridFunctionBlocks.Dock = DockStyle.Fill;
            gridFunctionBlocks.Location = new Point(0, 0);
            gridFunctionBlocks.Name = "gridFunctionBlocks";
            gridFunctionBlocks.ReadOnly = true;
            gridFunctionBlocks.RowTemplate.Height = 25;
            gridFunctionBlocks.Size = new Size(653, 461);
            gridFunctionBlocks.TabIndex = 0;
            gridFunctionBlocks.CellContextMenuStripNeeded += gridFunctionBlocks_CellContextMenuStripNeeded;
            gridFunctionBlocks.CellDoubleClick += gridFunctionBlocks_CellDoubleClick;
            // 
            // contextMenuStripFunctionBlocks
            // 
            contextMenuStripFunctionBlocks.Items.AddRange(new ToolStripItem[] { contextMenuItemChildDevicesAddDevice, toolStripSeparator1, contextMenuItemChildDevicesRefresh });
            contextMenuStripFunctionBlocks.Name = "contextMenuStrip1";
            contextMenuStripFunctionBlocks.Size = new Size(266, 76);
            contextMenuStripFunctionBlocks.Opening += contextMenuStripChildDevices_Opening;
            // 
            // contextMenuItemChildDevicesAddDevice
            // 
            contextMenuItemChildDevicesAddDevice.Name = "contextMenuItemChildDevicesAddDevice";
            contextMenuItemChildDevicesAddDevice.ShortcutKeyDisplayString = "<double-click>";
            contextMenuItemChildDevicesAddDevice.Size = new Size(265, 22);
            contextMenuItemChildDevicesAddDevice.Text = "Add function block";
            contextMenuItemChildDevicesAddDevice.Click += contextMenuItemChildDevicesAddDevice_Click;
            // 
            // toolStripSeparator1
            // 
            toolStripSeparator1.Name = "toolStripSeparator1";
            toolStripSeparator1.Size = new Size(262, 6);
            // 
            // contextMenuItemChildDevicesRefresh
            // 
            contextMenuItemChildDevicesRefresh.Image = Properties.Resources.refresh16;
            contextMenuItemChildDevicesRefresh.Name = "contextMenuItemChildDevicesRefresh";
            contextMenuItemChildDevicesRefresh.Size = new Size(265, 22);
            contextMenuItemChildDevicesRefresh.Text = "Refresh list";
            contextMenuItemChildDevicesRefresh.Click += contextMenuItemChildDevicesRefresh_Click;
            // 
            // frmAddFunctionBlockDialog
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(984, 461);
            Controls.Add(splitContainer1);
            Icon = (Icon)resources.GetObject("$this.Icon");
            MinimumSize = new Size(1000, 500);
            Name = "frmAddFunctionBlockDialog";
            StartPosition = FormStartPosition.CenterParent;
            Text = "Add function block";
            Shown += frmAddFunctionBlockDialog_Shown;
            splitContainer1.Panel1.ResumeLayout(false);
            splitContainer1.Panel1.PerformLayout();
            splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)splitContainer1).EndInit();
            splitContainer1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)gridFunctionBlocks).EndInit();
            contextMenuStripFunctionBlocks.ResumeLayout(false);
            ResumeLayout(false);
        }

        #endregion

        private SplitContainer splitContainer1;
        private TextBox textBox1;
        private TreeView treeParentDevices;
        private DataGridView gridFunctionBlocks;
        private ContextMenuStrip contextMenuStripFunctionBlocks;
        private ToolStripMenuItem contextMenuItemChildDevicesRefresh;
        private ToolStripMenuItem contextMenuItemChildDevicesAddDevice;
        private ToolStripSeparator toolStripSeparator1;
    }
}
