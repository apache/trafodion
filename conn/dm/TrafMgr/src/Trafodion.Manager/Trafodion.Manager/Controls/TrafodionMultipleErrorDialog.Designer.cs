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
﻿using System.Drawing;
namespace Trafodion.Manager.Framework.Controls
{
    partial class TrafodionMultipleErrorDialog
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionMultipleErrorDialog));
            this.pictureBox1 = new Trafodion.Manager.Framework.Controls.TrafodionPictureBox();
            this.splitContainer1 = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.OkButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.showDetailsButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.errorHeader_TrafodionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.errorMainText_TrafodionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.exceptions_TrafodionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // pictureBox1
            // 
            this.pictureBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.pictureBox1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
            this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
            this.pictureBox1.Location = new System.Drawing.Point(7, 58);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(33, 34);
            this.pictureBox1.TabIndex = 5;
            this.pictureBox1.TabStop = false;
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.AutoScroll = true;
            this.splitContainer1.Panel1.Controls.Add(this.tableLayoutPanel1);
            this.splitContainer1.Panel1MinSize = 105;
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.panel1);
            this.splitContainer1.Panel2Collapsed = true;
            this.splitContainer1.Panel2MinSize = 0;
            this.splitContainer1.Size = new System.Drawing.Size(454, 194);
            this.splitContainer1.SplitterDistance = 194;
            this.splitContainer1.SplitterWidth = 9;
            this.splitContainer1.TabIndex = 6;
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.AutoSize = true;
            this.tableLayoutPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Controls.Add(this.TrafodionPanel1, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.tableLayoutPanel2, 0, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 2;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 80.72916F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 19.27083F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(454, 194);
            this.tableLayoutPanel1.TabIndex = 6;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.OkButton);
            this.TrafodionPanel1.Controls.Add(this.showDetailsButton);
            this.TrafodionPanel1.Location = new System.Drawing.Point(127, 160);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(200, 30);
            this.TrafodionPanel1.TabIndex = 1;
            // 
            // OkButton
            // 
            this.OkButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.OkButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.OkButton.Location = new System.Drawing.Point(115, 3);
            this.OkButton.Name = "OkButton";
            this.OkButton.Size = new System.Drawing.Size(76, 23);
            this.OkButton.TabIndex = 3;
            this.OkButton.Text = "OK";
            this.OkButton.UseVisualStyleBackColor = true;
            this.OkButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // showDetailsButton
            // 
            this.showDetailsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.showDetailsButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.showDetailsButton.Location = new System.Drawing.Point(3, 3);
            this.showDetailsButton.Name = "showDetailsButton";
            this.showDetailsButton.Size = new System.Drawing.Size(106, 23);
            this.showDetailsButton.TabIndex = 4;
            this.showDetailsButton.Text = "Show Details";
            this.showDetailsButton.UseVisualStyleBackColor = true;
            this.showDetailsButton.Click += new System.EventHandler(this.showDetails_Click);
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.AutoScroll = true;
            this.tableLayoutPanel2.AutoSize = true;
            this.tableLayoutPanel2.ColumnCount = 2;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 10.49869F));
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 89.50131F));
            this.tableLayoutPanel2.Controls.Add(this.pictureBox1, 0, 0);
            this.tableLayoutPanel2.Controls.Add(this.TrafodionPanel2, 1, 0);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(3, 3);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 1;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(448, 150);
            this.tableLayoutPanel2.TabIndex = 0;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.TrafodionPanel2.Controls.Add(this.errorHeader_TrafodionLabel);
            this.TrafodionPanel2.Controls.Add(this.errorMainText_TrafodionTextBox);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel2.Location = new System.Drawing.Point(50, 3);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(395, 144);
            this.TrafodionPanel2.TabIndex = 6;
            // 
            // errorHeader_TrafodionLabel
            // 
            this.errorHeader_TrafodionLabel.AutoSize = true;
            this.errorHeader_TrafodionLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this.errorHeader_TrafodionLabel.Location = new System.Drawing.Point(3, 3);
            this.errorHeader_TrafodionLabel.Name = "errorHeader_TrafodionLabel";
            this.errorHeader_TrafodionLabel.Size = new System.Drawing.Size(178, 13);
            this.errorHeader_TrafodionLabel.TabIndex = 1;
            this.errorHeader_TrafodionLabel.Text = "The following errors were returned:";
            // 
            // errorMainText_TrafodionTextBox
            // 
            this.errorMainText_TrafodionTextBox.BackColor = System.Drawing.SystemColors.Control;
            this.errorMainText_TrafodionTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.errorMainText_TrafodionTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.errorMainText_TrafodionTextBox.Location = new System.Drawing.Point(1, 19);
            this.errorMainText_TrafodionTextBox.Multiline = true;
            this.errorMainText_TrafodionTextBox.Name = "errorMainText_TrafodionTextBox";
            this.errorMainText_TrafodionTextBox.ReadOnly = true;
            this.errorMainText_TrafodionTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.errorMainText_TrafodionTextBox.Size = new System.Drawing.Size(393, 124);
            this.errorMainText_TrafodionTextBox.TabIndex = 0;
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this.exceptions_TrafodionTextBox);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Padding = new System.Windows.Forms.Padding(5, 10, 5, 5);
            this.panel1.Size = new System.Drawing.Size(150, 25);
            this.panel1.TabIndex = 6;
            // 
            // exceptions_TrafodionTextBox
            // 
            this.exceptions_TrafodionTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.exceptions_TrafodionTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.exceptions_TrafodionTextBox.Location = new System.Drawing.Point(5, 10);
            this.exceptions_TrafodionTextBox.Multiline = true;
            this.exceptions_TrafodionTextBox.Name = "exceptions_TrafodionTextBox";
            this.exceptions_TrafodionTextBox.ReadOnly = true;
            this.exceptions_TrafodionTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.exceptions_TrafodionTextBox.Size = new System.Drawing.Size(140, 10);
            this.exceptions_TrafodionTextBox.TabIndex = 0;
            // 
            // TrafodionMultipleErrorDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ClientSize = new System.Drawing.Size(454, 194);
            this.Controls.Add(this.splitContainer1);
            this.Name = "TrafodionMultipleErrorDialog";
            this.Text = "Trafodion Database Manager - Error";
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this.tableLayoutPanel2.ResumeLayout(false);
            this.TrafodionPanel2.ResumeLayout(false);
            this.TrafodionPanel2.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionButton OkButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton showDetailsButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPictureBox pictureBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer splitContainer1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox exceptions_TrafodionTextBox;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private TrafodionPanel TrafodionPanel1;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        private TrafodionPanel TrafodionPanel2;
        private TrafodionLabel errorHeader_TrafodionLabel;
        private TrafodionTextBox errorMainText_TrafodionTextBox;
    }
}
