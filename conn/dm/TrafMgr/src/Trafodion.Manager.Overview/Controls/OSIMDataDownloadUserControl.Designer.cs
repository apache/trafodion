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
﻿using System.Windows.Forms;
namespace Trafodion.Manager.OverviewArea
{
    partial class OSIMDataDownloadUserControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.downloadButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.selectAllCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionStatusStrip = new Trafodion.Manager.Framework.Controls.TrafodionStatusStrip();
            this.toolStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
            this.toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.backgroundWorker = new System.ComponentModel.BackgroundWorker();
            this.TrafodionCloseButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.refreshButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionStatusStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // downloadButton
            // 
            this.downloadButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.downloadButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.downloadButton.Location = new System.Drawing.Point(624, 527);
            this.downloadButton.Name = "downloadButton";
            this.downloadButton.Size = new System.Drawing.Size(75, 21);
            this.downloadButton.TabIndex = 1;
            this.downloadButton.Text = "&Download";
            this.downloadButton.UseVisualStyleBackColor = true;
            this.downloadButton.Click += new System.EventHandler(this.downloadButton_Click);
            // 
            // selectAllCheckBox
            // 
            this.selectAllCheckBox.AutoSize = true;
            this.selectAllCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.selectAllCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.selectAllCheckBox.Location = new System.Drawing.Point(23, 18);
            this.selectAllCheckBox.Name = "selectAllCheckBox";
            this.selectAllCheckBox.Size = new System.Drawing.Size(75, 18);
            this.selectAllCheckBox.TabIndex = 2;
            this.selectAllCheckBox.Text = "Select All";
            this.selectAllCheckBox.UseVisualStyleBackColor = true;
            this.selectAllCheckBox.Click += new System.EventHandler(this.selectAllCheckBox_Click);
            // 
            // helpButton
            // 
            this.helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.helpButton.Location = new System.Drawing.Point(812, 527);
            this.helpButton.Name = "helpButton";
            this.helpButton.Size = new System.Drawing.Size(75, 21);
            this.helpButton.TabIndex = 3;
            this.helpButton.Text = "&Help";
            this.helpButton.UseVisualStyleBackColor = true;
            this.helpButton.Click += new System.EventHandler(this.helpButton_Click);
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionGroupBox1.Controls.Add(this.TrafodionPanel1);
            this.TrafodionGroupBox1.Controls.Add(this.selectAllCheckBox);
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(0, 3);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(896, 512);
            this.TrafodionGroupBox1.TabIndex = 4;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "OSIM Data Files";
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.TrafodionPanel1.Location = new System.Drawing.Point(6, 40);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(886, 466);
            this.TrafodionPanel1.TabIndex = 1;
            // 
            // TrafodionStatusStrip
            // 
            this.TrafodionStatusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripProgressBar,
            this.toolStripStatusLabel});
            this.TrafodionStatusStrip.Location = new System.Drawing.Point(0, 548);
            this.TrafodionStatusStrip.Name = "TrafodionStatusStrip";
            this.TrafodionStatusStrip.Size = new System.Drawing.Size(898, 22);
            this.TrafodionStatusStrip.TabIndex = 5;
            this.TrafodionStatusStrip.Text = "TrafodionStatusStrip";
            // 
            // toolStripProgressBar
            // 
            this.toolStripProgressBar.Maximum = 400;
            this.toolStripProgressBar.Name = "toolStripProgressBar";
            this.toolStripProgressBar.Size = new System.Drawing.Size(440, 16);
            this.toolStripProgressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            // 
            // toolStripStatusLabel
            // 
            this.toolStripStatusLabel.Name = "toolStripStatusLabel";
            this.toolStripStatusLabel.Size = new System.Drawing.Size(118, 17);
            this.toolStripStatusLabel.Text = "toolStripStatusLabel1";
            // 
            // TrafodionCloseButton
            // 
            this.TrafodionCloseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionCloseButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionCloseButton.Location = new System.Drawing.Point(718, 527);
            this.TrafodionCloseButton.Name = "TrafodionCloseButton";
            this.TrafodionCloseButton.Size = new System.Drawing.Size(75, 21);
            this.TrafodionCloseButton.TabIndex = 3;
            this.TrafodionCloseButton.Text = "&Close";
            this.TrafodionCloseButton.UseVisualStyleBackColor = true;
            this.TrafodionCloseButton.Click += new System.EventHandler(this.TrafodionCloseButton_Click);
            // 
            // refreshButton
            // 
            this.refreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.refreshButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.refreshButton.Location = new System.Drawing.Point(3, 527);
            this.refreshButton.Name = "refreshButton";
            this.refreshButton.Size = new System.Drawing.Size(75, 21);
            this.refreshButton.TabIndex = 6;
            this.refreshButton.Text = "&Refresh";
            this.refreshButton.UseVisualStyleBackColor = true;
            this.refreshButton.Click += new System.EventHandler(this.refreshButton_Click);
            // 
            // OSIMDataDownloadUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.refreshButton);
            this.Controls.Add(this.TrafodionStatusStrip);
            this.Controls.Add(this.TrafodionGroupBox1);
            this.Controls.Add(this.TrafodionCloseButton);
            this.Controls.Add(this.helpButton);
            this.Controls.Add(this.downloadButton);
            this.Name = "OSIMDataDownloadUserControl";
            this.Size = new System.Drawing.Size(898, 570);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.TrafodionStatusStrip.ResumeLayout(false);
            this.TrafodionStatusStrip.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionButton downloadButton;
        private Framework.Controls.TrafodionCheckBox selectAllCheckBox;
        private Framework.Controls.TrafodionButton helpButton;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Framework.Controls.TrafodionStatusStrip TrafodionStatusStrip;
        private ToolStripProgressBar toolStripProgressBar;
        private ToolStripStatusLabel toolStripStatusLabel;
        private System.ComponentModel.BackgroundWorker backgroundWorker;
        private Framework.Controls.TrafodionButton TrafodionCloseButton;
        private Framework.Controls.TrafodionButton refreshButton;

    }
}