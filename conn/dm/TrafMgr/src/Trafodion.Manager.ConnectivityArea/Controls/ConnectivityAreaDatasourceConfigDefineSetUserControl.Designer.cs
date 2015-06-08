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
﻿namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaDatasourceConfigDefineSetUserControl
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
            this.Define_TrafodionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.definesAdd_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.definesModify_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.definesRemove_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.defines_TrafodionPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.Sets_TrafodionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.setsAdd_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.setsModify_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.setsRemove_TrafodionButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.sets_TrafodionPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.Define_TrafodionGroupBox.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.Sets_TrafodionGroupBox.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // Define_TrafodionGroupBox
            // 
            this.Define_TrafodionGroupBox.Controls.Add(this.tableLayoutPanel1);
            this.Define_TrafodionGroupBox.Controls.Add(this.defines_TrafodionPanel);
            this.Define_TrafodionGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.Define_TrafodionGroupBox.Location = new System.Drawing.Point(0, 0);
            this.Define_TrafodionGroupBox.Name = "Define_TrafodionGroupBox";
            this.Define_TrafodionGroupBox.Size = new System.Drawing.Size(490, 513);
            this.Define_TrafodionGroupBox.TabIndex = 3;
            this.Define_TrafodionGroupBox.TabStop = false;
            this.Define_TrafodionGroupBox.Text = "DEFINEs";
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 1;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Controls.Add(this.flowLayoutPanel1, 0, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(3, 16);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(484, 34);
            this.tableLayoutPanel1.TabIndex = 5;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.flowLayoutPanel1.Controls.Add(this.definesAdd_TrafodionButton);
            this.flowLayoutPanel1.Controls.Add(this.definesModify_TrafodionButton);
            this.flowLayoutPanel1.Controls.Add(this.definesRemove_TrafodionButton);
            this.flowLayoutPanel1.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(120, 3);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(244, 28);
            this.flowLayoutPanel1.TabIndex = 3;
            // 
            // definesAdd_TrafodionButton
            // 
            this.definesAdd_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.definesAdd_TrafodionButton.Location = new System.Drawing.Point(3, 3);
            this.definesAdd_TrafodionButton.Name = "definesAdd_TrafodionButton";
            this.definesAdd_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.definesAdd_TrafodionButton.TabIndex = 0;
            this.definesAdd_TrafodionButton.Text = "Add...";
            this.definesAdd_TrafodionButton.UseVisualStyleBackColor = true;
            this.definesAdd_TrafodionButton.Click += new System.EventHandler(this.definesAdd_TrafodionButton_Click);
            // 
            // definesModify_TrafodionButton
            // 
            this.definesModify_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.definesModify_TrafodionButton.Location = new System.Drawing.Point(84, 3);
            this.definesModify_TrafodionButton.Name = "definesModify_TrafodionButton";
            this.definesModify_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.definesModify_TrafodionButton.TabIndex = 1;
            this.definesModify_TrafodionButton.Text = "Modify...";
            this.definesModify_TrafodionButton.UseVisualStyleBackColor = true;
            this.definesModify_TrafodionButton.Click += new System.EventHandler(this.definesModify_TrafodionButton_Click);
            // 
            // definesRemove_TrafodionButton
            // 
            this.definesRemove_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.definesRemove_TrafodionButton.Location = new System.Drawing.Point(165, 3);
            this.definesRemove_TrafodionButton.Name = "definesRemove_TrafodionButton";
            this.definesRemove_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.definesRemove_TrafodionButton.TabIndex = 2;
            this.definesRemove_TrafodionButton.Text = "Remove";
            this.definesRemove_TrafodionButton.UseVisualStyleBackColor = true;
            this.definesRemove_TrafodionButton.Click += new System.EventHandler(this.definesRemove_TrafodionButton_Click);
            // 
            // defines_TrafodionPanel
            // 
            this.defines_TrafodionPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.defines_TrafodionPanel.Location = new System.Drawing.Point(3, 51);
            this.defines_TrafodionPanel.Name = "defines_TrafodionPanel";
            this.defines_TrafodionPanel.Size = new System.Drawing.Size(484, 460);
            this.defines_TrafodionPanel.TabIndex = 6;
            // 
            // Sets_TrafodionGroupBox
            // 
            this.Sets_TrafodionGroupBox.Controls.Add(this.tableLayoutPanel2);
            this.Sets_TrafodionGroupBox.Controls.Add(this.sets_TrafodionPanel);
            this.Sets_TrafodionGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.Sets_TrafodionGroupBox.Location = new System.Drawing.Point(0, 0);
            this.Sets_TrafodionGroupBox.Name = "Sets_TrafodionGroupBox";
            this.Sets_TrafodionGroupBox.Size = new System.Drawing.Size(545, 513);
            this.Sets_TrafodionGroupBox.TabIndex = 4;
            this.Sets_TrafodionGroupBox.TabStop = false;
            this.Sets_TrafodionGroupBox.Text = "SETs";
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.ColumnCount = 1;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.Controls.Add(this.flowLayoutPanel2, 0, 0);
            this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.tableLayoutPanel2.Location = new System.Drawing.Point(3, 16);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 1;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(539, 34);
            this.tableLayoutPanel2.TabIndex = 6;
            // 
            // flowLayoutPanel2
            // 
            this.flowLayoutPanel2.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.flowLayoutPanel2.Controls.Add(this.setsAdd_TrafodionButton);
            this.flowLayoutPanel2.Controls.Add(this.setsModify_TrafodionButton);
            this.flowLayoutPanel2.Controls.Add(this.setsRemove_TrafodionButton);
            this.flowLayoutPanel2.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
            this.flowLayoutPanel2.Location = new System.Drawing.Point(147, 3);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            this.flowLayoutPanel2.Size = new System.Drawing.Size(244, 28);
            this.flowLayoutPanel2.TabIndex = 3;
            // 
            // setsAdd_TrafodionButton
            // 
            this.setsAdd_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.setsAdd_TrafodionButton.Location = new System.Drawing.Point(3, 3);
            this.setsAdd_TrafodionButton.Name = "setsAdd_TrafodionButton";
            this.setsAdd_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.setsAdd_TrafodionButton.TabIndex = 0;
            this.setsAdd_TrafodionButton.Text = "Add...";
            this.setsAdd_TrafodionButton.UseVisualStyleBackColor = true;
            this.setsAdd_TrafodionButton.Click += new System.EventHandler(this.setsAdd_TrafodionButton_Click);
            // 
            // setsModify_TrafodionButton
            // 
            this.setsModify_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.setsModify_TrafodionButton.Location = new System.Drawing.Point(84, 3);
            this.setsModify_TrafodionButton.Name = "setsModify_TrafodionButton";
            this.setsModify_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.setsModify_TrafodionButton.TabIndex = 1;
            this.setsModify_TrafodionButton.Text = "Modify...";
            this.setsModify_TrafodionButton.UseVisualStyleBackColor = true;
            this.setsModify_TrafodionButton.Click += new System.EventHandler(this.setsModify_TrafodionButton_Click);
            // 
            // setsRemove_TrafodionButton
            // 
            this.setsRemove_TrafodionButton.Anchor = System.Windows.Forms.AnchorStyles.None;
            this.setsRemove_TrafodionButton.Location = new System.Drawing.Point(165, 3);
            this.setsRemove_TrafodionButton.Name = "setsRemove_TrafodionButton";
            this.setsRemove_TrafodionButton.Size = new System.Drawing.Size(75, 23);
            this.setsRemove_TrafodionButton.TabIndex = 2;
            this.setsRemove_TrafodionButton.Text = "Remove";
            this.setsRemove_TrafodionButton.UseVisualStyleBackColor = true;
            this.setsRemove_TrafodionButton.Click += new System.EventHandler(this.setsRemove_TrafodionButton_Click);
            // 
            // sets_TrafodionPanel
            // 
            this.sets_TrafodionPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.sets_TrafodionPanel.Location = new System.Drawing.Point(3, 51);
            this.sets_TrafodionPanel.Name = "sets_TrafodionPanel";
            this.sets_TrafodionPanel.Size = new System.Drawing.Size(539, 461);
            this.sets_TrafodionPanel.TabIndex = 5;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.Controls.Add(this.splitContainer1);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(1039, 513);
            this.TrafodionPanel1.TabIndex = 5;
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 0);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.Define_TrafodionGroupBox);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.Sets_TrafodionGroupBox);
            this.splitContainer1.Size = new System.Drawing.Size(1039, 513);
            this.splitContainer1.SplitterDistance = 490;
            this.splitContainer1.TabIndex = 5;
            // 
            // ConnectivityAreaDatasourceConfigDefineSetUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaDatasourceConfigDefineSetUserControl";
            this.Size = new System.Drawing.Size(1039, 513);
            this.Define_TrafodionGroupBox.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.Sets_TrafodionGroupBox.ResumeLayout(false);
            this.tableLayoutPanel2.ResumeLayout(false);
            this.flowLayoutPanel2.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox Define_TrafodionGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox Sets_TrafodionGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel sets_TrafodionPanel;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton definesAdd_TrafodionButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton definesModify_TrafodionButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton definesRemove_TrafodionButton;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionButton setsAdd_TrafodionButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton setsModify_TrafodionButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton setsRemove_TrafodionButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel defines_TrafodionPanel;
    }
}
