namespace openDAQDemoNet
{
    partial class frmEditComponent
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
            gridAttributes = new DataGridView();
            contextMenuStripGridAttributes = new ContextMenuStrip(components);
            conetxtMenuItemGridAttributesEdit = new ToolStripMenuItem();
            ((System.ComponentModel.ISupportInitialize)gridAttributes).BeginInit();
            contextMenuStripGridAttributes.SuspendLayout();
            SuspendLayout();
            // 
            // gridAttributes
            // 
            gridAttributes.AllowUserToAddRows = false;
            gridAttributes.AllowUserToDeleteRows = false;
            gridAttributes.BackgroundColor = SystemColors.Window;
            gridAttributes.ContextMenuStrip = contextMenuStripGridAttributes;
            gridAttributes.Dock = DockStyle.Fill;
            gridAttributes.Location = new Point(0, 0);
            gridAttributes.Name = "gridAttributes";
            gridAttributes.ReadOnly = true;
            gridAttributes.RowTemplate.Height = 25;
            gridAttributes.Size = new Size(684, 561);
            gridAttributes.TabIndex = 6;
            gridAttributes.CellContextMenuStripNeeded += gridAttributes_CellContextMenuStripNeeded;
            gridAttributes.CellDoubleClick += gridAttributes_CellDoubleClick;
            // 
            // contextMenuStripGridAttributes
            // 
            contextMenuStripGridAttributes.Items.AddRange(new ToolStripItem[] { conetxtMenuItemGridAttributesEdit });
            contextMenuStripGridAttributes.Name = "contextMenuStrip1";
            contextMenuStripGridAttributes.Size = new Size(193, 26);
            contextMenuStripGridAttributes.Opening += contextMenuStripGridAttributes_Opening;
            // 
            // conetxtMenuItemGridAttributesEdit
            // 
            conetxtMenuItemGridAttributesEdit.Name = "conetxtMenuItemGridAttributesEdit";
            conetxtMenuItemGridAttributesEdit.ShortcutKeyDisplayString = "<double-click>";
            conetxtMenuItemGridAttributesEdit.Size = new Size(192, 22);
            conetxtMenuItemGridAttributesEdit.Text = "Edit...";
            conetxtMenuItemGridAttributesEdit.Click += conetxtMenuItemGridAttributesEdit_Click;
            // 
            // frmEditComponent
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(684, 561);
            Controls.Add(gridAttributes);
            FormBorderStyle = FormBorderStyle.SizableToolWindow;
            Name = "frmEditComponent";
            StartPosition = FormStartPosition.CenterParent;
            Text = "Attributes";
            FormClosing += frmEditComponent_FormClosing;
            Shown += frmEditComponent_Shown;
            ((System.ComponentModel.ISupportInitialize)gridAttributes).EndInit();
            contextMenuStripGridAttributes.ResumeLayout(false);
            ResumeLayout(false);
        }

        #endregion

        private DataGridView gridAttributes;
        private ContextMenuStrip contextMenuStripGridAttributes;
        private ToolStripMenuItem conetxtMenuItemGridAttributesEdit;
    }
}