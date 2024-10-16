namespace openDAQDemoNet
{
    partial class frmFunctionDialog
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
            groupArguments = new GroupBox();
            tableArguments = new TableLayoutPanel();
            label2 = new Label();
            label1 = new Label();
            comboBox1 = new ComboBox();
            lblNoArguments = new Label();
            pnlBottom = new Panel();
            lblResult = new Label();
            txtResult = new TextBox();
            btnClose = new Button();
            btnExecute = new Button();
            pnlTop = new Panel();
            lblDescription = new Label();
            txtDescription = new TextBox();
            button1 = new Button();
            button2 = new Button();
            groupArguments.SuspendLayout();
            tableArguments.SuspendLayout();
            pnlBottom.SuspendLayout();
            pnlTop.SuspendLayout();
            SuspendLayout();
            // 
            // groupArguments
            // 
            groupArguments.Controls.Add(tableArguments);
            groupArguments.Controls.Add(lblNoArguments);
            groupArguments.Dock = DockStyle.Fill;
            groupArguments.Location = new Point(4, 35);
            groupArguments.Name = "groupArguments";
            groupArguments.Size = new Size(376, 187);
            groupArguments.TabIndex = 7;
            groupArguments.TabStop = false;
            groupArguments.Text = "Arguments";
            // 
            // tableArguments
            // 
            tableArguments.AutoSize = true;
            tableArguments.ColumnCount = 3;
            tableArguments.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 75F));
            tableArguments.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100F));
            tableArguments.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 50F));
            tableArguments.Controls.Add(label2, 2, 0);
            tableArguments.Controls.Add(label1, 0, 0);
            tableArguments.Controls.Add(comboBox1, 1, 0);
            tableArguments.Dock = DockStyle.Top;
            tableArguments.Location = new Point(3, 19);
            tableArguments.Name = "tableArguments";
            tableArguments.RowCount = 1;
            tableArguments.RowStyles.Add(new RowStyle(SizeType.Absolute, 40F));
            tableArguments.Size = new Size(370, 40);
            tableArguments.TabIndex = 0;
            // 
            // label2
            // 
            label2.Anchor = AnchorStyles.Left | AnchorStyles.Right;
            label2.AutoSize = true;
            label2.Location = new Point(323, 12);
            label2.Name = "label2";
            label2.Size = new Size(44, 15);
            label2.TabIndex = 2;
            label2.Text = "type";
            label2.TextAlign = ContentAlignment.MiddleCenter;
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
            comboBox1.Size = new Size(239, 23);
            comboBox1.TabIndex = 1;
            // 
            // lblNoArguments
            // 
            lblNoArguments.AutoSize = true;
            lblNoArguments.Location = new Point(92, 4);
            lblNoArguments.Name = "lblNoArguments";
            lblNoArguments.Size = new Size(83, 15);
            lblNoArguments.TabIndex = 1;
            lblNoArguments.Text = "No arguments";
            lblNoArguments.TextAlign = ContentAlignment.MiddleCenter;
            // 
            // pnlBottom
            // 
            pnlBottom.Controls.Add(lblResult);
            pnlBottom.Controls.Add(txtResult);
            pnlBottom.Controls.Add(btnClose);
            pnlBottom.Controls.Add(btnExecute);
            pnlBottom.Dock = DockStyle.Bottom;
            pnlBottom.Location = new Point(4, 222);
            pnlBottom.Name = "pnlBottom";
            pnlBottom.Size = new Size(376, 35);
            pnlBottom.TabIndex = 9;
            // 
            // lblResult
            // 
            lblResult.AutoSize = true;
            lblResult.Location = new Point(3, 9);
            lblResult.Name = "lblResult";
            lblResult.Size = new Size(39, 15);
            lblResult.TabIndex = 3;
            lblResult.Text = "Result";
            lblResult.TextAlign = ContentAlignment.MiddleLeft;
            // 
            // txtResult
            // 
            txtResult.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            txtResult.Location = new Point(48, 6);
            txtResult.Name = "txtResult";
            txtResult.PlaceholderText = "Execute to get result ->";
            txtResult.ReadOnly = true;
            txtResult.Size = new Size(130, 23);
            txtResult.TabIndex = 2;
            txtResult.TextAlign = HorizontalAlignment.Center;
            // 
            // btnClose
            // 
            btnClose.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            btnClose.Location = new Point(289, 6);
            btnClose.Name = "btnClose";
            btnClose.Size = new Size(75, 23);
            btnClose.TabIndex = 1;
            btnClose.Text = "Close";
            btnClose.UseVisualStyleBackColor = true;
            btnClose.Click += btnClose_Click;
            // 
            // btnExecute
            // 
            btnExecute.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            btnExecute.Location = new Point(208, 6);
            btnExecute.Name = "btnExecute";
            btnExecute.Size = new Size(75, 23);
            btnExecute.TabIndex = 0;
            btnExecute.Text = "Execute";
            btnExecute.UseVisualStyleBackColor = true;
            btnExecute.Click += btnExecute_Click;
            // 
            // pnlTop
            // 
            pnlTop.Controls.Add(lblDescription);
            pnlTop.Controls.Add(txtDescription);
            pnlTop.Controls.Add(button1);
            pnlTop.Controls.Add(button2);
            pnlTop.Dock = DockStyle.Top;
            pnlTop.Location = new Point(4, 4);
            pnlTop.Name = "pnlTop";
            pnlTop.Padding = new Padding(4);
            pnlTop.Size = new Size(376, 31);
            pnlTop.TabIndex = 10;
            // 
            // lblDescription
            // 
            lblDescription.AutoSize = true;
            lblDescription.Location = new Point(6, 7);
            lblDescription.Name = "lblDescription";
            lblDescription.Size = new Size(67, 15);
            lblDescription.TabIndex = 3;
            lblDescription.Text = "Description";
            lblDescription.TextAlign = ContentAlignment.MiddleLeft;
            // 
            // txtDescription
            // 
            txtDescription.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            txtDescription.Location = new Point(79, 4);
            txtDescription.Name = "txtDescription";
            txtDescription.PlaceholderText = "No property description";
            txtDescription.ReadOnly = true;
            txtDescription.Size = new Size(293, 23);
            txtDescription.TabIndex = 2;
            // 
            // button1
            // 
            button1.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            button1.Location = new Point(461, 10);
            button1.Name = "button1";
            button1.Size = new Size(75, 23);
            button1.TabIndex = 1;
            button1.Text = "Close";
            button1.UseVisualStyleBackColor = true;
            // 
            // button2
            // 
            button2.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            button2.Location = new Point(380, 10);
            button2.Name = "button2";
            button2.Size = new Size(75, 23);
            button2.TabIndex = 0;
            button2.Text = "Execute";
            button2.UseVisualStyleBackColor = true;
            // 
            // frmFunctionDialog
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            CancelButton = btnClose;
            ClientSize = new Size(384, 261);
            Controls.Add(groupArguments);
            Controls.Add(pnlBottom);
            Controls.Add(pnlTop);
            Name = "frmFunctionDialog";
            Padding = new Padding(4);
            StartPosition = FormStartPosition.CenterParent;
            Text = "Execute function";
            groupArguments.ResumeLayout(false);
            groupArguments.PerformLayout();
            tableArguments.ResumeLayout(false);
            tableArguments.PerformLayout();
            pnlBottom.ResumeLayout(false);
            pnlBottom.PerformLayout();
            pnlTop.ResumeLayout(false);
            pnlTop.PerformLayout();
            ResumeLayout(false);
        }

        #endregion

        private GroupBox groupArguments;
        private TableLayoutPanel tableArguments;
        private Label label1;
        private ComboBox comboBox1;
        private Label lblNoArguments;
        private Panel pnlBottom;
        private Button btnClose;
        private Button btnExecute;
        private Label lblResult;
        private TextBox txtResult;
        private Label label2;
        private Panel pnlTop;
        private Label lblDescription;
        private TextBox txtDescription;
        private Button button1;
        private Button button2;
    }
}
