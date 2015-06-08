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
﻿namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class WMSQueryDetailUserControl
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
            MyDispose(disposing);
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WMSQueryDetailUserControl));
            this._widgetCanvas = new Trafodion.Manager.Framework.WidgetCanvas();
            this._theQueryIDPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._refreshButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator6 = new System.Windows.Forms.ToolStripSeparator();
            this._cancelButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this._getSqlTextButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this._getSqlPlanButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this._getReposButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this._getSessionStatsButton = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
            this._helpButton = new System.Windows.Forms.ToolStripButton();
            this._getPerTableStats = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator7 = new System.Windows.Forms.ToolStripSeparator();
            this._getWarningButton = new System.Windows.Forms.ToolStripButton();
            this._theWarnLevelTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theWarnIndPictureBox = new Trafodion.Manager.Framework.Controls.TrafodionPictureBox();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSubStateTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theQueryIdTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.label58 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.label57 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theStateTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theWarnIndImageList = new System.Windows.Forms.ImageList(this.components);
            this._getHistoryToolStripButton = new System.Windows.Forms.ToolStripButton();
            this._theQueryIDPanel.SuspendLayout();
            this._theToolStrip.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theWarnIndPictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // _widgetCanvas
            // 
            this._widgetCanvas.ActiveWidget = null;
            this._widgetCanvas.AllowDelete = true;
            this._widgetCanvas.AllowDrop = true;
            this._widgetCanvas.BackColor = System.Drawing.Color.Azure;
            this._widgetCanvas.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this._widgetCanvas.Dock = System.Windows.Forms.DockStyle.Fill;
            this._widgetCanvas.LayoutManager = null;
            this._widgetCanvas.Location = new System.Drawing.Point(0, 63);
            this._widgetCanvas.LockBackColor = System.Drawing.SystemColors.Control;
            this._widgetCanvas.Name = "_widgetCanvas";
            this._widgetCanvas.Size = new System.Drawing.Size(1126, 383);
            this._widgetCanvas.TabIndex = 2;
            this._widgetCanvas.ThePersistenceKey = null;
            this._widgetCanvas.UnlockBackColor = System.Drawing.Color.Azure;
            this._widgetCanvas.ViewName = null;
            this._widgetCanvas.ViewNum = 0;
            this._widgetCanvas.ViewText = null;
            this._widgetCanvas.WidgetsModel = ((System.Collections.Hashtable)(resources.GetObject("_widgetCanvas.WidgetsModel")));
            // 
            // _theQueryIDPanel
            // 
            this._theQueryIDPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theQueryIDPanel.Controls.Add(this._theToolStrip);
            this._theQueryIDPanel.Controls.Add(this._theWarnLevelTextBox);
            this._theQueryIDPanel.Controls.Add(this._theWarnIndPictureBox);
            this._theQueryIDPanel.Controls.Add(this.TrafodionLabel2);
            this._theQueryIDPanel.Controls.Add(this.label1);
            this._theQueryIDPanel.Controls.Add(this._theSubStateTextBox);
            this._theQueryIDPanel.Controls.Add(this._theQueryIdTextBox);
            this._theQueryIDPanel.Controls.Add(this.label58);
            this._theQueryIDPanel.Controls.Add(this.label57);
            this._theQueryIDPanel.Controls.Add(this._theStateTextBox);
            this._theQueryIDPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theQueryIDPanel.Location = new System.Drawing.Point(0, 0);
            this._theQueryIDPanel.Name = "_theQueryIDPanel";
            this._theQueryIDPanel.Size = new System.Drawing.Size(1126, 63);
            this._theQueryIDPanel.TabIndex = 1;
            // 
            // _theToolStrip
            // 
            this._theToolStrip.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theToolStrip.AutoSize = false;
            this._theToolStrip.BackColor = System.Drawing.SystemColors.ControlLight;
            this._theToolStrip.Dock = System.Windows.Forms.DockStyle.None;
            this._theToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._refreshButton,
            this.toolStripSeparator6,
            this._cancelButton,
            this.toolStripSeparator1,
            this._getSqlTextButton,
            this.toolStripSeparator3,
            this._getSqlPlanButton,
            this.toolStripSeparator2,
            this._getReposButton,
            this.toolStripSeparator4,
            this._getSessionStatsButton,
            this.toolStripSeparator5,
            this._helpButton,
            this._getPerTableStats,
            this.toolStripSeparator7,
            this._getWarningButton,
            this._getHistoryToolStripButton});
            this._theToolStrip.Location = new System.Drawing.Point(332, 34);
            this._theToolStrip.Name = "_theToolStrip";
            this._theToolStrip.Size = new System.Drawing.Size(784, 25);
            this._theToolStrip.TabIndex = 26;
            // 
            // _refreshButton
            // 
            this._refreshButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._refreshButton.Image = ((System.Drawing.Image)(resources.GetObject("_refreshButton.Image")));
            this._refreshButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._refreshButton.Name = "_refreshButton";
            this._refreshButton.Size = new System.Drawing.Size(23, 22);
            this._refreshButton.Text = "Refresh";
            // 
            // toolStripSeparator6
            // 
            this.toolStripSeparator6.Name = "toolStripSeparator6";
            this.toolStripSeparator6.Size = new System.Drawing.Size(6, 25);
            // 
            // _cancelButton
            // 
            this._cancelButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.cancelImage;
            this._cancelButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(63, 22);
            this._cancelButton.Text = "Cancel";
            this._cancelButton.ToolTipText = "Cancel this query";
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            // 
            // _getSqlTextButton
            // 
            this._getSqlTextButton.Image = ((System.Drawing.Image)(resources.GetObject("_getSqlTextButton.Image")));
            this._getSqlTextButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._getSqlTextButton.Name = "_getSqlTextButton";
            this._getSqlTextButton.Size = new System.Drawing.Size(71, 22);
            this._getSqlTextButton.Text = "Full Text";
            this._getSqlTextButton.ToolTipText = "Get full query text";
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(6, 25);
            // 
            // _getSqlPlanButton
            // 
            this._getSqlPlanButton.Image = ((System.Drawing.Image)(resources.GetObject("_getSqlPlanButton.Image")));
            this._getSqlPlanButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._getSqlPlanButton.Name = "_getSqlPlanButton";
            this._getSqlPlanButton.Size = new System.Drawing.Size(74, 22);
            this._getSqlPlanButton.Text = "SQL Plan";
            this._getSqlPlanButton.ToolTipText = "Get SQL plan ";
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
            // 
            // _getReposButton
            // 
            this._getReposButton.Image = ((System.Drawing.Image)(resources.GetObject("_getReposButton.Image")));
            this._getReposButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._getReposButton.Name = "_getReposButton";
            this._getReposButton.Size = new System.Drawing.Size(78, 22);
            this._getReposButton.Text = "Repo Info";
            this._getReposButton.ToolTipText = "Get detailed query stats from Repository";
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(6, 25);
            // 
            // _getSessionStatsButton
            // 
            this._getSessionStatsButton.Image = ((System.Drawing.Image)(resources.GetObject("_getSessionStatsButton.Image")));
            this._getSessionStatsButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._getSessionStatsButton.Name = "_getSessionStatsButton";
            this._getSessionStatsButton.Size = new System.Drawing.Size(94, 22);
            this._getSessionStatsButton.Text = "Session Stats";
            this._getSessionStatsButton.ToolTipText = "Get session stats";
            this._getSessionStatsButton.Click += new System.EventHandler(this.GetSessionStatsButton_Click);
            // 
            // toolStripSeparator5
            // 
            this.toolStripSeparator5.Name = "toolStripSeparator5";
            this.toolStripSeparator5.Size = new System.Drawing.Size(6, 25);
            // 
            // _helpButton
            // 
            this._helpButton.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._helpButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._helpButton.Image = ((System.Drawing.Image)(resources.GetObject("_helpButton.Image")));
            this._helpButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(23, 22);
            this._helpButton.Text = "Help";
            this._helpButton.Click += new System.EventHandler(this.GetHelpButton_Click);
            // 
            // _getPerTableStats
            // 
            this._getPerTableStats.Image = ((System.Drawing.Image)(resources.GetObject("_getPerTableStats.Image")));
            this._getPerTableStats.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._getPerTableStats.Name = "_getPerTableStats";
            this._getPerTableStats.Size = new System.Drawing.Size(106, 22);
            this._getPerTableStats.Text = "Per-Table Stats";
            this._getPerTableStats.ToolTipText = "Get table statistics";
            this._getPerTableStats.Click += new System.EventHandler(this.GetTableStatsButton_Click);
            // 
            // toolStripSeparator7
            // 
            this.toolStripSeparator7.Name = "toolStripSeparator7";
            this.toolStripSeparator7.Size = new System.Drawing.Size(6, 25);
            // 
            // _getWarningButton
            // 
            this._getWarningButton.Image = ((System.Drawing.Image)(resources.GetObject("_getWarningButton.Image")));
            this._getWarningButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._getWarningButton.Name = "_getWarningButton";
            this._getWarningButton.Size = new System.Drawing.Size(72, 22);
            this._getWarningButton.Text = "Warning";
            this._getWarningButton.ToolTipText = "Get query warning";
            this._getWarningButton.Click += new System.EventHandler(this.GetWarningButton_Click);
            // 
            // _theWarnLevelTextBox
            // 
            this._theWarnLevelTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theWarnLevelTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theWarnLevelTextBox.Location = new System.Drawing.Point(991, 9);
            this._theWarnLevelTextBox.Name = "_theWarnLevelTextBox";
            this._theWarnLevelTextBox.ReadOnly = true;
            this._theWarnLevelTextBox.Size = new System.Drawing.Size(82, 21);
            this._theWarnLevelTextBox.TabIndex = 23;
            // 
            // _theWarnIndPictureBox
            // 
            this._theWarnIndPictureBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theWarnIndPictureBox.Image = ((System.Drawing.Image)(resources.GetObject("_theWarnIndPictureBox.Image")));
            this._theWarnIndPictureBox.Location = new System.Drawing.Point(1088, 9);
            this._theWarnIndPictureBox.Name = "_theWarnIndPictureBox";
            this._theWarnIndPictureBox.Size = new System.Drawing.Size(28, 21);
            this._theWarnIndPictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
            this._theWarnIndPictureBox.TabIndex = 25;
            this._theWarnIndPictureBox.TabStop = false;
            this._theWarnIndPictureBox.MouseHover += new System.EventHandler(this._theWarnIndPictureBox_MouseHover);
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(919, 13);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(61, 13);
            this.TrafodionLabel2.TabIndex = 24;
            this.TrafodionLabel2.Text = "Warn Level";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label1.Location = new System.Drawing.Point(9, 11);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(51, 13);
            this.label1.TabIndex = 6;
            this.label1.Text = "Query ID";
            // 
            // _theSubStateTextBox
            // 
            this._theSubStateTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSubStateTextBox.Location = new System.Drawing.Point(216, 36);
            this._theSubStateTextBox.Name = "_theSubStateTextBox";
            this._theSubStateTextBox.ReadOnly = true;
            this._theSubStateTextBox.Size = new System.Drawing.Size(104, 21);
            this._theSubStateTextBox.TabIndex = 11;
            // 
            // _theQueryIdTextBox
            // 
            this._theQueryIdTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theQueryIdTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theQueryIdTextBox.Location = new System.Drawing.Point(67, 9);
            this._theQueryIdTextBox.Name = "_theQueryIdTextBox";
            this._theQueryIdTextBox.ReadOnly = true;
            this._theQueryIdTextBox.Size = new System.Drawing.Size(846, 21);
            this._theQueryIdTextBox.TabIndex = 7;
            // 
            // label58
            // 
            this.label58.AutoSize = true;
            this.label58.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label58.Location = new System.Drawing.Point(158, 40);
            this.label58.Name = "label58";
            this.label58.Size = new System.Drawing.Size(50, 13);
            this.label58.TabIndex = 10;
            this.label58.Text = "Substate";
            // 
            // label57
            // 
            this.label57.AutoSize = true;
            this.label57.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label57.Location = new System.Drawing.Point(9, 40);
            this.label57.Name = "label57";
            this.label57.Size = new System.Drawing.Size(33, 13);
            this.label57.TabIndex = 8;
            this.label57.Text = "State";
            // 
            // _theStateTextBox
            // 
            this._theStateTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theStateTextBox.Location = new System.Drawing.Point(67, 36);
            this._theStateTextBox.Name = "_theStateTextBox";
            this._theStateTextBox.ReadOnly = true;
            this._theStateTextBox.Size = new System.Drawing.Size(82, 21);
            this._theStateTextBox.TabIndex = 9;
            // 
            // _theToolTip
            // 
            this._theToolTip.IsBalloon = true;
            // 
            // _theWarnIndImageList
            // 
            this._theWarnIndImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("_theWarnIndImageList.ImageStream")));
            this._theWarnIndImageList.TransparentColor = System.Drawing.Color.Transparent;
            this._theWarnIndImageList.Images.SetKeyName(0, "ledgreen2.png");
            this._theWarnIndImageList.Images.SetKeyName(1, "ledyellow2.png");
            this._theWarnIndImageList.Images.SetKeyName(2, "ledorange2.png");
            this._theWarnIndImageList.Images.SetKeyName(3, "ledred2.png");
            // 
            // _getHistoryToolStripButton
            // 
            this._getHistoryToolStripButton.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.History;
            this._getHistoryToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._getHistoryToolStripButton.Name = "_getHistoryToolStripButton";
            this._getHistoryToolStripButton.Size = new System.Drawing.Size(65, 22);
            this._getHistoryToolStripButton.Text = "History";
            this._getHistoryToolStripButton.ToolTipText = "Look up past occurrences of this query";
            this._getHistoryToolStripButton.Click += new System.EventHandler(this._getHistoryToolStripButton_Click);
            // 
            // WMSQueryDetailUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._widgetCanvas);
            this.Controls.Add(this._theQueryIDPanel);
            this.Name = "WMSQueryDetailUserControl";
            this.Size = new System.Drawing.Size(1126, 446);
            this._theQueryIDPanel.ResumeLayout(false);
            this._theQueryIDPanel.PerformLayout();
            this._theToolStrip.ResumeLayout(false);
            this._theToolStrip.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._theWarnIndPictureBox)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theQueryIDPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theSubStateTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theQueryIdTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label58;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label57;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theStateTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theWarnLevelTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPictureBox _theWarnIndPictureBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip _theToolStrip;
        private System.Windows.Forms.ToolStripButton _cancelButton;
        private System.Windows.Forms.ToolStripButton _refreshButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripButton _getSqlTextButton;
        private System.Windows.Forms.ToolStripButton _getSqlPlanButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ToolStripButton _getReposButton;
        private System.Windows.Forms.ToolStripButton _getSessionStatsButton;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator5;
        private System.Windows.Forms.ToolStripButton _helpButton;
        private System.Windows.Forms.ToolStripButton _getPerTableStats;
        private Trafodion.Manager.Framework.WidgetCanvas _widgetCanvas;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator6;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator7;
        private System.Windows.Forms.ToolStripButton _getWarningButton;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _theToolTip;
        private System.Windows.Forms.ImageList _theWarnIndImageList;
        private System.Windows.Forms.ToolStripButton _getHistoryToolStripButton;
    }
}
