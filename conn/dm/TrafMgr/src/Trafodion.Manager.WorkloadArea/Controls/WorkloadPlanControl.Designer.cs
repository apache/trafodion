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
    partial class WorkloadPlanControl
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WorkloadPlanControl));
            this._statusStrip = new Trafodion.Manager.Framework.Controls.TrafodionStatusStrip();
            this._toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this._toolStripProgressBar = new System.Windows.Forms.ToolStripProgressBar();
            this._sqlTextBox = new Trafodion.Manager.DatabaseArea.Queries.Controls.SqlStatementTextBox();
            this._queryPlanPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._splitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._sqlTextGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._planGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._queryIDTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._queryIDLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theRefreshButton = new System.Windows.Forms.ToolStripButton();
            this._theHelpButton = new System.Windows.Forms.ToolStripButton();
            this.btnGenerateMaintainScript = new System.Windows.Forms.ToolStripButton();
            this._statusStrip.SuspendLayout();
            this._splitContainer.Panel1.SuspendLayout();
            this._splitContainer.Panel2.SuspendLayout();
            this._splitContainer.SuspendLayout();
            this._sqlTextGroupBox.SuspendLayout();
            this._planGroupBox.SuspendLayout();
            this._headerPanel.SuspendLayout();
            this.TrafodionToolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _statusStrip
            // 
            this._statusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._toolStripStatusLabel,
            this._toolStripProgressBar});
            this._statusStrip.Location = new System.Drawing.Point(0, 643);
            this._statusStrip.Name = "_statusStrip";
            this._statusStrip.Size = new System.Drawing.Size(1001, 22);
            this._statusStrip.TabIndex = 0;
            this._statusStrip.Text = "TrafodionStatusStrip1";
            // 
            // _toolStripStatusLabel
            // 
            this._toolStripStatusLabel.Name = "_toolStripStatusLabel";
            this._toolStripStatusLabel.Size = new System.Drawing.Size(111, 17);
            this._toolStripStatusLabel.Text = "Fetching SQL Text...";
            // 
            // _toolStripProgressBar
            // 
            this._toolStripProgressBar.Name = "_toolStripProgressBar";
            this._toolStripProgressBar.Size = new System.Drawing.Size(100, 16);
            this._toolStripProgressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            // 
            // _sqlTextBox
            // 
            this._sqlTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._sqlTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this._sqlTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._sqlTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sqlTextBox.Location = new System.Drawing.Point(10, 24);
            this._sqlTextBox.Margin = new System.Windows.Forms.Padding(10);
            this._sqlTextBox.Name = "_sqlTextBox";
            this._sqlTextBox.ReadOnly = true;
            this._sqlTextBox.Size = new System.Drawing.Size(975, 185);
            this._sqlTextBox.TabIndex = 1;
            this._sqlTextBox.Text = "";
            this._sqlTextBox.WordWrap = false;
            // 
            // _queryPlanPanel
            // 
            this._queryPlanPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._queryPlanPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._queryPlanPanel.Location = new System.Drawing.Point(10, 24);
            this._queryPlanPanel.Name = "_queryPlanPanel";
            this._queryPlanPanel.Padding = new System.Windows.Forms.Padding(5);
            this._queryPlanPanel.Size = new System.Drawing.Size(981, 314);
            this._queryPlanPanel.TabIndex = 2;
            // 
            // _splitContainer
            // 
            this._splitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._splitContainer.Location = new System.Drawing.Point(0, 61);
            this._splitContainer.Name = "_splitContainer";
            this._splitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _splitContainer.Panel1
            // 
            this._splitContainer.Panel1.Controls.Add(this._sqlTextGroupBox);
            this._splitContainer.Panel1.Padding = new System.Windows.Forms.Padding(3);
            // 
            // _splitContainer.Panel2
            // 
            this._splitContainer.Panel2.Controls.Add(this._planGroupBox);
            this._splitContainer.Size = new System.Drawing.Size(1001, 582);
            this._splitContainer.SplitterDistance = 225;
            this._splitContainer.SplitterWidth = 9;
            this._splitContainer.TabIndex = 0;
            // 
            // _sqlTextGroupBox
            // 
            this._sqlTextGroupBox.Controls.Add(this._sqlTextBox);
            this._sqlTextGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._sqlTextGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._sqlTextGroupBox.Location = new System.Drawing.Point(3, 3);
            this._sqlTextGroupBox.Name = "_sqlTextGroupBox";
            this._sqlTextGroupBox.Padding = new System.Windows.Forms.Padding(10);
            this._sqlTextGroupBox.Size = new System.Drawing.Size(995, 219);
            this._sqlTextGroupBox.TabIndex = 2;
            this._sqlTextGroupBox.TabStop = false;
            this._sqlTextGroupBox.Text = "Full SQL Text";
            // 
            // _planGroupBox
            // 
            this._planGroupBox.Controls.Add(this._queryPlanPanel);
            this._planGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._planGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._planGroupBox.Location = new System.Drawing.Point(0, 0);
            this._planGroupBox.Name = "_planGroupBox";
            this._planGroupBox.Padding = new System.Windows.Forms.Padding(10);
            this._planGroupBox.Size = new System.Drawing.Size(1001, 348);
            this._planGroupBox.TabIndex = 0;
            this._planGroupBox.TabStop = false;
            this._planGroupBox.Text = "Explain Plan";
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this._queryIDTextBox);
            this._headerPanel.Controls.Add(this._queryIDLabel);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(1001, 36);
            this._headerPanel.TabIndex = 2;
            // 
            // _queryIDTextBox
            // 
            this._queryIDTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._queryIDTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._queryIDTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._queryIDTextBox.Location = new System.Drawing.Point(68, 6);
            this._queryIDTextBox.Name = "_queryIDTextBox";
            this._queryIDTextBox.ReadOnly = true;
            this._queryIDTextBox.Size = new System.Drawing.Size(910, 21);
            this._queryIDTextBox.TabIndex = 1;
            // 
            // _queryIDLabel
            // 
            this._queryIDLabel.AutoSize = true;
            this._queryIDLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._queryIDLabel.Location = new System.Drawing.Point(10, 9);
            this._queryIDLabel.Name = "_queryIDLabel";
            this._queryIDLabel.Size = new System.Drawing.Size(57, 13);
            this._queryIDLabel.TabIndex = 0;
            this._queryIDLabel.Text = "Query ID";
            // 
            // TrafodionToolStrip1
            // 
            this.TrafodionToolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theRefreshButton,
            this._theHelpButton,
            this.btnGenerateMaintainScript});
            this.TrafodionToolStrip1.Location = new System.Drawing.Point(0, 36);
            this.TrafodionToolStrip1.Name = "TrafodionToolStrip1";
            this.TrafodionToolStrip1.Size = new System.Drawing.Size(1001, 25);
            this.TrafodionToolStrip1.TabIndex = 2;
            this.TrafodionToolStrip1.Text = "TrafodionToolStrip1";
            // 
            // _theRefreshButton
            // 
            this._theRefreshButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theRefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("_theRefreshButton.Image")));
            this._theRefreshButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theRefreshButton.Name = "_theRefreshButton";
            this._theRefreshButton.Size = new System.Drawing.Size(23, 22);
            this._theRefreshButton.Text = "Refresh SQL Text";
            this._theRefreshButton.ToolTipText = "Refresh SQL Text";
            this._theRefreshButton.Click += new System.EventHandler(this._theRefreshButton_Click);
            // 
            // _theHelpButton
            // 
            this._theHelpButton.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theHelpButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this._theHelpButton.Image = ((System.Drawing.Image)(resources.GetObject("_theHelpButton.Image")));
            this._theHelpButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            this._theHelpButton.Name = "_theHelpButton";
            this._theHelpButton.Size = new System.Drawing.Size(23, 22);
            this._theHelpButton.Text = "_theHelpButton";
            this._theHelpButton.ToolTipText = "Help";
            this._theHelpButton.Click += new System.EventHandler(this._theHelpButton_Click);
            // 
            // btnGenerateMaintainScript
            // 
            this.btnGenerateMaintainScript.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            this.btnGenerateMaintainScript.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources.MaintainScript;
            this.btnGenerateMaintainScript.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btnGenerateMaintainScript.Name = "btnGenerateMaintainScript";
            this.btnGenerateMaintainScript.Size = new System.Drawing.Size(23, 22);
            this.btnGenerateMaintainScript.Text = "Generate Maintain Script";
            this.btnGenerateMaintainScript.Click += new System.EventHandler(this.btnGenerateMaintainScript_Click);
            // 
            // WorkloadPlanControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._splitContainer);
            this.Controls.Add(this.TrafodionToolStrip1);
            this.Controls.Add(this._headerPanel);
            this.Controls.Add(this._statusStrip);
            this.Name = "WorkloadPlanControl";
            this.Size = new System.Drawing.Size(1001, 665);
            this._statusStrip.ResumeLayout(false);
            this._statusStrip.PerformLayout();
            this._splitContainer.Panel1.ResumeLayout(false);
            this._splitContainer.Panel2.ResumeLayout(false);
            this._splitContainer.ResumeLayout(false);
            this._sqlTextGroupBox.ResumeLayout(false);
            this._planGroupBox.ResumeLayout(false);
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionStatusStrip _statusStrip;
        private System.Windows.Forms.ToolStripProgressBar _toolStripProgressBar;
        private System.Windows.Forms.ToolStripStatusLabel _toolStripStatusLabel;
        private Trafodion.Manager.DatabaseArea.Queries.Controls.SqlStatementTextBox _sqlTextBox;
        private Framework.Controls.TrafodionPanel _queryPlanPanel;
        private Framework.Controls.TrafodionSplitContainer _splitContainer;
        private Framework.Controls.TrafodionPanel _headerPanel;
        private Framework.Controls.TrafodionTextBox _queryIDTextBox;
        private Framework.Controls.TrafodionLabel _queryIDLabel;
        private Framework.Controls.TrafodionGroupBox _sqlTextGroupBox;
        private Framework.Controls.TrafodionGroupBox _planGroupBox;
        private Framework.Controls.TrafodionToolStrip TrafodionToolStrip1;
        private System.Windows.Forms.ToolStripButton _theRefreshButton;
        private System.Windows.Forms.ToolStripButton _theHelpButton;
        private System.Windows.Forms.ToolStripButton btnGenerateMaintainScript;
    }
}
