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
﻿namespace Trafodion.Manager.Main
{
    partial class OptionsPanel
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
            this.theSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.theOptionsTree = new Trafodion.Manager.Framework.Controls.TrafodionTreeView();
            this.theTablePanel = new System.Windows.Forms.TableLayoutPanel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theApplyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theOkButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theOptionsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theSplitContainer.Panel1.SuspendLayout();
            this.theSplitContainer.Panel2.SuspendLayout();
            this.theSplitContainer.SuspendLayout();
            this.theTablePanel.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // theSplitContainer
            // 
            this.theSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.theSplitContainer.Name = "theSplitContainer";
            // 
            // theSplitContainer.Panel1
            // 
            this.theSplitContainer.Panel1.Controls.Add(this.theOptionsTree);
            // 
            // theSplitContainer.Panel2
            // 
            this.theSplitContainer.Panel2.AutoScroll = true;
            this.theSplitContainer.Panel2.Controls.Add(this.theTablePanel);
            this.theSplitContainer.Size = new System.Drawing.Size(680, 387);
            this.theSplitContainer.SplitterDistance = 226;
            this.theSplitContainer.SplitterWidth = 9;
            this.theSplitContainer.TabIndex = 0;
            // 
            // theOptionsTree
            // 
            this.theOptionsTree.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.theOptionsTree.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theOptionsTree.Location = new System.Drawing.Point(0, 0);
            this.theOptionsTree.Name = "theOptionsTree";
            this.theOptionsTree.Size = new System.Drawing.Size(226, 387);
            this.theOptionsTree.TabIndex = 0;
            this.theOptionsTree.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.theOptionsTree_AfterSelect);
            // 
            // theTablePanel
            // 
            this.theTablePanel.ColumnCount = 1;
            this.theTablePanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.theTablePanel.Controls.Add(this.flowLayoutPanel1, 0, 1);
            this.theTablePanel.Controls.Add(this.theOptionsPanel, 0, 0);
            this.theTablePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theTablePanel.Location = new System.Drawing.Point(0, 0);
            this.theTablePanel.Name = "theTablePanel";
            this.theTablePanel.RowCount = 2;
            this.theTablePanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 90.43928F));
            this.theTablePanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 9.560723F));
            this.theTablePanel.Size = new System.Drawing.Size(445, 387);
            this.theTablePanel.TabIndex = 1;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.theCancelButton);
            this.flowLayoutPanel1.Controls.Add(this.theApplyButton);
            this.flowLayoutPanel1.Controls.Add(this.theOkButton);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(3, 353);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
            this.flowLayoutPanel1.Size = new System.Drawing.Size(439, 31);
            this.flowLayoutPanel1.TabIndex = 1;
            // 
            // theCancelButton
            // 
            this.theCancelButton.Location = new System.Drawing.Point(361, 3);
            this.theCancelButton.Name = "theCancelButton";
            this.theCancelButton.Size = new System.Drawing.Size(75, 23);
            this.theCancelButton.TabIndex = 0;
            this.theCancelButton.Text = "&Cancel";
            this.theCancelButton.UseVisualStyleBackColor = true;
            // 
            // theApplyButton
            // 
            this.theApplyButton.Location = new System.Drawing.Point(280, 3);
            this.theApplyButton.Name = "theApplyButton";
            this.theApplyButton.Size = new System.Drawing.Size(75, 23);
            this.theApplyButton.TabIndex = 1;
            this.theApplyButton.Text = "&Apply";
            this.theApplyButton.UseVisualStyleBackColor = true;
            // 
            // theOkButton
            // 
            this.theOkButton.Location = new System.Drawing.Point(199, 3);
            this.theOkButton.Name = "theOkButton";
            this.theOkButton.Size = new System.Drawing.Size(75, 23);
            this.theOkButton.TabIndex = 2;
            this.theOkButton.Text = "&OK";
            this.theOkButton.UseVisualStyleBackColor = true;
            // 
            // theOptionsPanel
            // 
            this.theOptionsPanel.AutoScroll = true;
            this.theOptionsPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theOptionsPanel.Location = new System.Drawing.Point(3, 3);
            this.theOptionsPanel.Name = "theOptionsPanel";
            this.theOptionsPanel.Size = new System.Drawing.Size(439, 344);
            this.theOptionsPanel.TabIndex = 0;
            // 
            // OptionsPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.theSplitContainer);
            this.Name = "OptionsPanel";
            this.Size = new System.Drawing.Size(680, 387);
            this.theSplitContainer.Panel1.ResumeLayout(false);
            this.theSplitContainer.Panel2.ResumeLayout(false);
            this.theSplitContainer.ResumeLayout(false);
            this.theTablePanel.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer theSplitContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionTreeView theOptionsTree;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theOptionsPanel;
        private System.Windows.Forms.TableLayoutPanel theTablePanel;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionButton theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton theApplyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton theOkButton;

    }
}
