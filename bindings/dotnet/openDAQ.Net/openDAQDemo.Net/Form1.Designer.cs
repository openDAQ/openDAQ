namespace openDAQDemo.Net
{
    partial class Form1
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
            btnScan = new Button();
            cmbDevices = new ComboBox();
            btnConnect = new Button();
            btnDisconnect = new Button();
            treeView1 = new TreeView();
            listBox1 = new ListBox();
            SuspendLayout();
            // 
            // btnScan
            // 
            btnScan.Location = new Point(12, 13);
            btnScan.Name = "btnScan";
            btnScan.Size = new Size(75, 23);
            btnScan.TabIndex = 0;
            btnScan.Text = "Scan";
            btnScan.UseVisualStyleBackColor = true;
            btnScan.Click += btnScan_Click;
            // 
            // cmbDevices
            // 
            cmbDevices.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            cmbDevices.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbDevices.FormattingEnabled = true;
            cmbDevices.Location = new Point(93, 13);
            cmbDevices.Name = "cmbDevices";
            cmbDevices.Size = new Size(679, 23);
            cmbDevices.TabIndex = 1;
            cmbDevices.SelectedIndexChanged += cmbDevices_SelectedIndexChanged;
            // 
            // btnConnect
            // 
            btnConnect.Location = new Point(12, 42);
            btnConnect.Name = "btnConnect";
            btnConnect.Size = new Size(75, 23);
            btnConnect.TabIndex = 0;
            btnConnect.Text = "Connect";
            btnConnect.UseVisualStyleBackColor = true;
            btnConnect.Click += btnConnect_Click;
            // 
            // btnDisconnect
            // 
            btnDisconnect.Location = new Point(93, 42);
            btnDisconnect.Name = "btnDisconnect";
            btnDisconnect.Size = new Size(75, 23);
            btnDisconnect.TabIndex = 0;
            btnDisconnect.Text = "Disconnect";
            btnDisconnect.UseVisualStyleBackColor = true;
            btnDisconnect.Click += btnDisconnect_Click;
            // 
            // treeView1
            // 
            treeView1.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left;
            treeView1.Location = new Point(12, 71);
            treeView1.Name = "treeView1";
            treeView1.Size = new Size(176, 278);
            treeView1.TabIndex = 2;
            treeView1.AfterSelect += treeView1_AfterSelect;
            // 
            // listBox1
            // 
            listBox1.Font = new Font("Consolas", 9F, FontStyle.Regular, GraphicsUnit.Point);
            listBox1.FormattingEnabled = true;
            listBox1.IntegralHeight = false;
            listBox1.ItemHeight = 14;
            listBox1.Location = new Point(194, 71);
            listBox1.Name = "listBox1";
            listBox1.Size = new Size(578, 278);
            listBox1.TabIndex = 3;
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(784, 361);
            Controls.Add(listBox1);
            Controls.Add(treeView1);
            Controls.Add(cmbDevices);
            Controls.Add(btnDisconnect);
            Controls.Add(btnConnect);
            Controls.Add(btnScan);
            MinimumSize = new Size(800, 400);
            Name = "Form1";
            StartPosition = FormStartPosition.CenterScreen;
            Text = "openDAQ Demo .NET";
            FormClosing += Form1_FormClosing;
            Load += Form1_Load;
            ResumeLayout(false);
        }

        #endregion

        private Button btnScan;
        private ComboBox cmbDevices;
        private Button btnConnect;
        private Button btnDisconnect;
        private TreeView treeView1;
        private ListBox listBox1;
    }
}