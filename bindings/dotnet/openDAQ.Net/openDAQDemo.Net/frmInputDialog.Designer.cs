namespace openDAQDemoNet
{
    partial class frmInputDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmInputDialog));
            lblCaption = new Label();
            comboBoxValue = new ComboBox();
            lblMinMax = new Label();
            btnCancel = new Button();
            btnOK = new Button();
            SuspendLayout();
            // 
            // lblCaption
            // 
            lblCaption.AutoSize = true;
            lblCaption.Location = new Point(12, 9);
            lblCaption.Name = "lblCaption";
            lblCaption.Size = new Size(62, 15);
            lblCaption.TabIndex = 0;
            lblCaption.Text = "lblCaption";
            // 
            // comboBoxValue
            // 
            comboBoxValue.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            comboBoxValue.FormattingEnabled = true;
            comboBoxValue.Location = new Point(12, 27);
            comboBoxValue.Name = "comboBoxValue";
            comboBoxValue.Size = new Size(260, 23);
            comboBoxValue.TabIndex = 1;
            // 
            // lblMinMax
            // 
            lblMinMax.AutoSize = true;
            lblMinMax.Location = new Point(12, 53);
            lblMinMax.Name = "lblMinMax";
            lblMinMax.Size = new Size(64, 15);
            lblMinMax.TabIndex = 2;
            lblMinMax.Text = "lblMinMax";
            // 
            // btnCancel
            // 
            btnCancel.DialogResult = DialogResult.Cancel;
            btnCancel.Location = new Point(197, 71);
            btnCancel.Name = "btnCancel";
            btnCancel.Size = new Size(75, 23);
            btnCancel.TabIndex = 3;
            btnCancel.Text = "Cancel";
            btnCancel.UseVisualStyleBackColor = true;
            // 
            // btnOK
            // 
            btnOK.DialogResult = DialogResult.OK;
            btnOK.Location = new Point(116, 71);
            btnOK.Name = "btnOK";
            btnOK.Size = new Size(75, 23);
            btnOK.TabIndex = 3;
            btnOK.Text = "OK";
            btnOK.UseVisualStyleBackColor = true;
            // 
            // frmInputDialog
            // 
            AcceptButton = btnOK;
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            CancelButton = btnCancel;
            ClientSize = new Size(284, 101);
            Controls.Add(btnOK);
            Controls.Add(btnCancel);
            Controls.Add(lblMinMax);
            Controls.Add(comboBoxValue);
            Controls.Add(lblCaption);
            FormBorderStyle = FormBorderStyle.SizableToolWindow;
            Icon = (Icon)resources.GetObject("$this.Icon");
            MaximizeBox = false;
            MinimizeBox = false;
            MinimumSize = new Size(300, 140);
            Name = "frmInputDialog";
            StartPosition = FormStartPosition.CenterParent;
            Text = "frmInputDialog";
            FormClosing += frmInputDialog_FormClosing;
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private Label lblCaption;
        private ComboBox comboBoxValue;
        private Label lblMinMax;
        private Button btnCancel;
        private Button btnOK;
    }
}
