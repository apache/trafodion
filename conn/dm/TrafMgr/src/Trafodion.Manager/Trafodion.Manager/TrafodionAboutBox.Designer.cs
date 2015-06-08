// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager
{
    partial class TrafodionAboutBox
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
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
            this.components = new System.ComponentModel.Container();
            this.tableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            this.labelProductName = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.labelCopyright = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.labelCompanyName = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.VProcLabel = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionRichTextBox1 = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.TrafodionPictureBox1 = new Trafodion.Manager.Framework.Controls.TrafodionPictureBox();
            this.tableLayoutPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.TrafodionPictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // tableLayoutPanel
            // 
            this.tableLayoutPanel.ColumnCount = 2;
            this.tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 21.25748F));
            this.tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 78.74252F));
            this.tableLayoutPanel.Controls.Add(this.labelProductName, 1, 3);
            this.tableLayoutPanel.Controls.Add(this.labelCopyright, 1, 5);
            this.tableLayoutPanel.Controls.Add(this.labelCompanyName, 1, 6);
            this.tableLayoutPanel.Controls.Add(this.okButton, 1, 7);
            this.tableLayoutPanel.Controls.Add(this.VProcLabel, 1, 4);
            this.tableLayoutPanel.Controls.Add(this.TrafodionRichTextBox1, 1, 0);
            this.tableLayoutPanel.Controls.Add(this.TrafodionPictureBox1, 0, 1);
            this.tableLayoutPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel.Location = new System.Drawing.Point(9, 9);
            this.tableLayoutPanel.Name = "tableLayoutPanel";
            this.tableLayoutPanel.RowCount = 7;
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 10F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 8F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 23F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 32F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 26F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 25F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 21F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 108F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel.Size = new System.Drawing.Size(668, 179);
            this.tableLayoutPanel.TabIndex = 0;
            this.tableLayoutPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.tableLayoutPanel_Paint);
            // 
            // labelProductName
            // 
            this.labelProductName.Dock = System.Windows.Forms.DockStyle.Fill;
            this.labelProductName.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelProductName.Location = new System.Drawing.Point(147, 41);
            this.labelProductName.Margin = new System.Windows.Forms.Padding(6, 0, 3, 0);
            this.labelProductName.MaximumSize = new System.Drawing.Size(0, 17);
            this.labelProductName.Name = "labelProductName";
            this.labelProductName.Size = new System.Drawing.Size(518, 17);
            this.labelProductName.TabIndex = 19;
            this.labelProductName.Text = "Product Name";
            this.labelProductName.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // labelCopyright
            // 
            this.labelCopyright.Dock = System.Windows.Forms.DockStyle.Fill;
            this.labelCopyright.Font = new System.Drawing.Font("Tahoma", 8F);
            this.labelCopyright.Location = new System.Drawing.Point(147, 99);
            this.labelCopyright.Margin = new System.Windows.Forms.Padding(6, 0, 3, 0);
            this.labelCopyright.MaximumSize = new System.Drawing.Size(0, 17);
            this.labelCopyright.Name = "labelCopyright";
            this.labelCopyright.Size = new System.Drawing.Size(518, 17);
            this.labelCopyright.TabIndex = 21;
            this.labelCopyright.Text = "Copyright";
            this.labelCopyright.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // labelCompanyName
            // 
            this.labelCompanyName.Dock = System.Windows.Forms.DockStyle.Fill;
            this.labelCompanyName.Font = new System.Drawing.Font("Tahoma", 8F);
            this.labelCompanyName.Location = new System.Drawing.Point(147, 124);
            this.labelCompanyName.Margin = new System.Windows.Forms.Padding(6, 0, 3, 0);
            this.labelCompanyName.MaximumSize = new System.Drawing.Size(0, 17);
            this.labelCompanyName.Name = "labelCompanyName";
            this.labelCompanyName.Size = new System.Drawing.Size(518, 17);
            this.labelCompanyName.TabIndex = 22;
            this.labelCompanyName.Text = "Company Name";
            this.labelCompanyName.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // okButton
            // 
            this.okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.okButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.okButton.Location = new System.Drawing.Point(590, 148);
            this.okButton.Name = "okButton";
            this.okButton.Size = new System.Drawing.Size(75, 27);
            this.okButton.TabIndex = 24;
            this.okButton.Text = "&OK";
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // VProcLabel
            // 
            this.VProcLabel.BackColor = System.Drawing.Color.White;
            this.VProcLabel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.VProcLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this.VProcLabel.Location = new System.Drawing.Point(147, 73);
            this.VProcLabel.Margin = new System.Windows.Forms.Padding(6, 0, 3, 0);
            this.VProcLabel.Name = "VProcLabel";
            this.VProcLabel.ReadOnly = true;
            this.VProcLabel.Size = new System.Drawing.Size(518, 20);
            this.VProcLabel.TabIndex = 29;
            this.VProcLabel.Text = "VProcLabel";
            // 
            // TrafodionRichTextBox1
            // 
            this.TrafodionRichTextBox1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionRichTextBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.TrafodionRichTextBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionRichTextBox1.Font = new System.Drawing.Font("Viner Hand ITC", 18F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.TrafodionRichTextBox1.Location = new System.Drawing.Point(144, 3);
            this.TrafodionRichTextBox1.Name = "TrafodionRichTextBox1";
            this.TrafodionRichTextBox1.ReadOnly = true;
            this.tableLayoutPanel.SetRowSpan(this.TrafodionRichTextBox1, 3);
            this.TrafodionRichTextBox1.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.None;
            this.TrafodionRichTextBox1.Size = new System.Drawing.Size(521, 35);
            this.TrafodionRichTextBox1.TabIndex = 30;
            this.TrafodionRichTextBox1.Text = "Trafodion";
            // 
            // TrafodionPictureBox1
            // 
            this.TrafodionPictureBox1.Image = global::Trafodion.Manager.Properties.Resources.Trafodion_Logo;
            this.TrafodionPictureBox1.Location = new System.Drawing.Point(3, 13);
            this.TrafodionPictureBox1.Name = "TrafodionPictureBox1";
            this.tableLayoutPanel.SetRowSpan(this.TrafodionPictureBox1, 7);
            this.TrafodionPictureBox1.Size = new System.Drawing.Size(135, 131);
            this.TrafodionPictureBox1.TabIndex = 31;
            this.TrafodionPictureBox1.TabStop = false;
            // 
            // TrafodionAboutBox
            // 
            this.AcceptButton = this.okButton;
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(686, 197);
            this.Controls.Add(this.tableLayoutPanel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "TrafodionAboutBox";
            this.Padding = new System.Windows.Forms.Padding(9);
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Trafodion Database Manager (Trafodion) - About";
            this.tableLayoutPanel.ResumeLayout(false);
            this.tableLayoutPanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.TrafodionPictureBox1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private TableLayoutPanel tableLayoutPanel;
        private TrafodionLabel labelProductName;
        private TrafodionButton okButton;
        private TrafodionTextBox VProcLabel;
        private TrafodionLabel labelCopyright;
        private TrafodionLabel labelCompanyName;
        private TrafodionRichTextBox TrafodionRichTextBox1;
        private TrafodionPictureBox TrafodionPictureBox1;
    }
}
