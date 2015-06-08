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
﻿namespace Trafodion.Manager.Framework.Navigation
{
    partial class NavigationUserControl
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
            this.theFavoritesAllSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.theFavoritesTreeViewPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theFavoritesTreeViewUserControl = new Trafodion.Manager.Framework.Favorites.FavoritesTreeViewUserControl();
            this.theFavoritesLabelStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.toolStripLabel1 = new System.Windows.Forms.ToolStripLabel();
            this.theNavigationTreeViewPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theNavigationTreeViewUserControl = new Trafodion.Manager.Framework.Navigation.NavigationTreeViewUserControl();
            this.theNavigationLabelStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.toolStripLabel2 = new System.Windows.Forms.ToolStripLabel();
            this.theFilterButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.theFavoritesAllSplitContainer.Panel1.SuspendLayout();
            this.theFavoritesAllSplitContainer.Panel2.SuspendLayout();
            this.theFavoritesAllSplitContainer.SuspendLayout();
            this.theFavoritesTreeViewPanel.SuspendLayout();
            this.theFavoritesLabelStrip.SuspendLayout();
            this.theNavigationTreeViewPanel.SuspendLayout();
            this.theNavigationLabelStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // theFavoritesAllSplitContainer
            // 
            this.theFavoritesAllSplitContainer.BackColor = System.Drawing.Color.White;
            this.theFavoritesAllSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theFavoritesAllSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.theFavoritesAllSplitContainer.Name = "theFavoritesAllSplitContainer";
            this.theFavoritesAllSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // theFavoritesAllSplitContainer.Panel1
            // 
            this.theFavoritesAllSplitContainer.Panel1.Controls.Add(this.theFavoritesTreeViewPanel);
            // 
            // theFavoritesAllSplitContainer.Panel2
            // 
            this.theFavoritesAllSplitContainer.Panel2.Controls.Add(this.theNavigationTreeViewPanel);
            this.theFavoritesAllSplitContainer.Size = new System.Drawing.Size(280, 770);
            this.theFavoritesAllSplitContainer.SplitterDistance = 120;
            this.theFavoritesAllSplitContainer.SplitterWidth = 9;
            this.theFavoritesAllSplitContainer.TabIndex = 1;
            // 
            // theFavoritesTreeViewPanel
            // 
            this.theFavoritesTreeViewPanel.BackColor = System.Drawing.SystemColors.Control;
            this.theFavoritesTreeViewPanel.Controls.Add(this.theFavoritesTreeViewUserControl);
            this.theFavoritesTreeViewPanel.Controls.Add(this.theFavoritesLabelStrip);
            this.theFavoritesTreeViewPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theFavoritesTreeViewPanel.Location = new System.Drawing.Point(0, 0);
            this.theFavoritesTreeViewPanel.Name = "theFavoritesTreeViewPanel";
            this.theFavoritesTreeViewPanel.Size = new System.Drawing.Size(280, 120);
            this.theFavoritesTreeViewPanel.TabIndex = 0;
            // 
            // theFavoritesTreeViewUserControl
            // 
            this.theFavoritesTreeViewUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theFavoritesTreeViewUserControl.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theFavoritesTreeViewUserControl.Location = new System.Drawing.Point(0, 25);
            this.theFavoritesTreeViewUserControl.Name = "theFavoritesTreeViewUserControl";
            this.theFavoritesTreeViewUserControl.Size = new System.Drawing.Size(280, 95);
            this.theFavoritesTreeViewUserControl.TabIndex = 1;
            // 
            // theFavoritesLabelStrip
            // 
            this.theFavoritesLabelStrip.AllowMerge = false;
            this.theFavoritesLabelStrip.AutoSize = false;
            this.theFavoritesLabelStrip.CanOverflow = false;
            this.theFavoritesLabelStrip.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.theFavoritesLabelStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripLabel1});
            this.theFavoritesLabelStrip.Location = new System.Drawing.Point(0, 0);
            this.theFavoritesLabelStrip.Name = "theFavoritesLabelStrip";
            this.theFavoritesLabelStrip.RenderMode = System.Windows.Forms.ToolStripRenderMode.ManagerRenderMode;
            this.theFavoritesLabelStrip.Size = new System.Drawing.Size(280, 25);
            this.theFavoritesLabelStrip.TabIndex = 3;
            // 
            // toolStripLabel1
            // 
            this.toolStripLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.toolStripLabel1.Name = "toolStripLabel1";
            this.toolStripLabel1.Size = new System.Drawing.Size(80, 22);
            this.toolStripLabel1.Text = "My Favorites";
            // 
            // theNavigationTreeViewPanel
            // 
            this.theNavigationTreeViewPanel.BackColor = System.Drawing.SystemColors.Control;
            this.theNavigationTreeViewPanel.Controls.Add(this.theNavigationTreeViewUserControl);
            this.theNavigationTreeViewPanel.Controls.Add(this.theNavigationLabelStrip);
            this.theNavigationTreeViewPanel.Controls.Add(this.theFilterButton);
            this.theNavigationTreeViewPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theNavigationTreeViewPanel.Location = new System.Drawing.Point(0, 0);
            this.theNavigationTreeViewPanel.Name = "theNavigationTreeViewPanel";
            this.theNavigationTreeViewPanel.Size = new System.Drawing.Size(280, 641);
            this.theNavigationTreeViewPanel.TabIndex = 0;
            // 
            // theNavigationTreeViewUserControl
            // 
            this.theNavigationTreeViewUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this.theNavigationTreeViewUserControl.Location = new System.Drawing.Point(0, 25);
            this.theNavigationTreeViewUserControl.Name = "theNavigationTreeViewUserControl";
            this.theNavigationTreeViewUserControl.Size = new System.Drawing.Size(280, 593);
            this.theNavigationTreeViewUserControl.TabIndex = 4;
            // 
            // theNavigationLabelStrip
            // 
            this.theNavigationLabelStrip.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.theNavigationLabelStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripLabel2});
            this.theNavigationLabelStrip.Location = new System.Drawing.Point(0, 0);
            this.theNavigationLabelStrip.Name = "theNavigationLabelStrip";
            this.theNavigationLabelStrip.Size = new System.Drawing.Size(280, 25);
            this.theNavigationLabelStrip.TabIndex = 2;
            this.theNavigationLabelStrip.Text = "TrafodionToolStrip1";
            // 
            // toolStripLabel2
            // 
            this.toolStripLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.toolStripLabel2.Name = "toolStripLabel2";
            this.toolStripLabel2.Size = new System.Drawing.Size(21, 22);
            this.toolStripLabel2.Text = "All";
            // 
            // theFilterButton
            // 
            this.theFilterButton.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(117)))), ((int)(((byte)(145)))), ((int)(((byte)(172)))));
            this.theFilterButton.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.theFilterButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.theFilterButton.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theFilterButton.ForeColor = System.Drawing.Color.White;
            this.theFilterButton.Location = new System.Drawing.Point(0, 618);
            this.theFilterButton.Name = "theFilterButton";
            this.theFilterButton.Size = new System.Drawing.Size(280, 23);
            this.theFilterButton.TabIndex = 3;
            this.theFilterButton.Text = "Filter By ...";
            this.theFilterButton.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.theFilterButton.UseVisualStyleBackColor = false;
            this.theFilterButton.Visible = false;
            // 
            // NavigationUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.White;
            this.Controls.Add(this.theFavoritesAllSplitContainer);
            this.Name = "NavigationUserControl";
            this.Size = new System.Drawing.Size(280, 770);
            this.theFavoritesAllSplitContainer.Panel1.ResumeLayout(false);
            this.theFavoritesAllSplitContainer.Panel2.ResumeLayout(false);
            this.theFavoritesAllSplitContainer.ResumeLayout(false);
            this.theFavoritesTreeViewPanel.ResumeLayout(false);
            this.theFavoritesLabelStrip.ResumeLayout(false);
            this.theFavoritesLabelStrip.PerformLayout();
            this.theNavigationTreeViewPanel.ResumeLayout(false);
            this.theNavigationTreeViewPanel.PerformLayout();
            this.theNavigationLabelStrip.ResumeLayout(false);
            this.theNavigationLabelStrip.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer theFavoritesAllSplitContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theFavoritesTreeViewPanel;
        private Trafodion.Manager.Framework.Favorites.FavoritesTreeViewUserControl theFavoritesTreeViewUserControl;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip theFavoritesLabelStrip;
        private System.Windows.Forms.ToolStripLabel toolStripLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theNavigationTreeViewPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip theNavigationLabelStrip;
        private System.Windows.Forms.ToolStripLabel toolStripLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionButton theFilterButton;
        private NavigationTreeViewUserControl theNavigationTreeViewUserControl;
    }
}
