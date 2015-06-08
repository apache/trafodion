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
namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class MonitoringUserControl
    {
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            MyDispose(disposing);
            base.Dispose(disposing); 
        }

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theTopPanelUpperLabel = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.splitContainer_oneGuiSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._systemMonitorControl = new Trafodion.Manager.OverviewArea.Controls.SystemMonitorControl();
            this._monitoringTopTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this.theTopPanel.SuspendLayout();
            this.splitContainer_oneGuiSplitContainer.Panel1.SuspendLayout();
            this.splitContainer_oneGuiSplitContainer.Panel2.SuspendLayout();
            this.splitContainer_oneGuiSplitContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.theTopPanel.Controls.Add(this.theTopPanelUpperLabel);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theTopPanel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(775, 33);
            this.theTopPanel.TabIndex = 8;
            // 
            // theTopPanelUpperLabel
            // 
            this.theTopPanelUpperLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.theTopPanelUpperLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelUpperLabel.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.theTopPanelUpperLabel.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelUpperLabel.Location = new System.Drawing.Point(5, 7);
            this.theTopPanelUpperLabel.Name = "theTopPanelUpperLabel";
            this.theTopPanelUpperLabel.ReadOnly = true;
            this.theTopPanelUpperLabel.Size = new System.Drawing.Size(763, 15);
            this.theTopPanelUpperLabel.TabIndex = 1;
            this.theTopPanelUpperLabel.Text = "Overall System Summary";
            // 
            // splitContainer_oneGuiSplitContainer
            // 
            this.splitContainer_oneGuiSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer_oneGuiSplitContainer.Location = new System.Drawing.Point(0, 33);
            this.splitContainer_oneGuiSplitContainer.Name = "splitContainer_oneGuiSplitContainer";
            // 
            // splitContainer_oneGuiSplitContainer.Panel1
            // 
            this.splitContainer_oneGuiSplitContainer.Panel1.Controls.Add(this._systemMonitorControl);
            // 
            // splitContainer_oneGuiSplitContainer.Panel2
            // 
            this.splitContainer_oneGuiSplitContainer.Panel2.Controls.Add(this._monitoringTopTabControl);
            this.splitContainer_oneGuiSplitContainer.Size = new System.Drawing.Size(775, 408);
            this.splitContainer_oneGuiSplitContainer.SplitterDistance = 211;
            this.splitContainer_oneGuiSplitContainer.SplitterWidth = 9;
            this.splitContainer_oneGuiSplitContainer.TabIndex = 9;
            // 
            // _systemMonitorControl
            // 
            this._systemMonitorControl.ConnectionDefn = null;
            this._systemMonitorControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._systemMonitorControl.Location = new System.Drawing.Point(0, 0);
            this._systemMonitorControl.Name = "_systemMonitorControl";
            this._systemMonitorControl.Size = new System.Drawing.Size(211, 408);
            this._systemMonitorControl.TabIndex = 0;
            // 
            // _monitoringTopTabControl
            // 
            this._monitoringTopTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._monitoringTopTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._monitoringTopTabControl.HotTrack = true;
            this._monitoringTopTabControl.Location = new System.Drawing.Point(0, 0);
            this._monitoringTopTabControl.Multiline = true;
            this._monitoringTopTabControl.Name = "_monitoringTopTabControl";
            this._monitoringTopTabControl.Padding = new System.Drawing.Point(10, 5);
            this._monitoringTopTabControl.SelectedIndex = 0;
            this._monitoringTopTabControl.Size = new System.Drawing.Size(555, 408);
            this._monitoringTopTabControl.TabIndex = 0;
            // 
            // MonitoringUserControl
            // 
            this.Controls.Add(this.splitContainer_oneGuiSplitContainer);
            this.Controls.Add(this.theTopPanel);
            this.Name = "MonitoringUserControl";
            this.Size = new System.Drawing.Size(775, 441);
            this.theTopPanel.ResumeLayout(false);
            this.theTopPanel.PerformLayout();
            this.splitContainer_oneGuiSplitContainer.Panel1.ResumeLayout(false);
            this.splitContainer_oneGuiSplitContainer.Panel2.ResumeLayout(false);
            this.splitContainer_oneGuiSplitContainer.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer splitContainer_oneGuiSplitContainer;
        private SystemMonitorControl _systemMonitorControl;
        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _monitoringTopTabControl;
        private System.ComponentModel.IContainer components;
    }
}