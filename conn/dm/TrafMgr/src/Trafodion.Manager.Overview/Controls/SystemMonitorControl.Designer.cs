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
namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class SystemMonitorControl
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
            this.graphContainer_tableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
            this._systemMonitorToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.contextMenu_oneGuiContextMenuStrip = new Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip(this.components);
            this.configuration_ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.SysMon_oneGuiSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this.oneGuiTabControl1 = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this.tabPage1 = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.tabPage2 = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.tabPage3 = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.config_oneGuiPanel = new Trafodion.Manager.OverviewArea.Controls.SystemMonitorConfigurationControl();
            this.cloneToWindowToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.contextMenu_oneGuiContextMenuStrip.SuspendLayout();
            this.SysMon_oneGuiSplitContainer.Panel1.SuspendLayout();
            this.SysMon_oneGuiSplitContainer.SuspendLayout();
            this.oneGuiTabControl1.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.SuspendLayout();
            // 
            // graphContainer_tableLayoutPanel
            // 
            this.graphContainer_tableLayoutPanel.BackColor = System.Drawing.SystemColors.ControlDarkDark;
            this.graphContainer_tableLayoutPanel.CellBorderStyle = System.Windows.Forms.TableLayoutPanelCellBorderStyle.InsetDouble;
            this.graphContainer_tableLayoutPanel.ColumnCount = 1;
            this.graphContainer_tableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.graphContainer_tableLayoutPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.graphContainer_tableLayoutPanel.Location = new System.Drawing.Point(3, 3);
            this.graphContainer_tableLayoutPanel.Name = "graphContainer_tableLayoutPanel";
            this.graphContainer_tableLayoutPanel.RowCount = 1;
            this.graphContainer_tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.graphContainer_tableLayoutPanel.Size = new System.Drawing.Size(486, 594);
            this.graphContainer_tableLayoutPanel.TabIndex = 5;
            // 
            // _systemMonitorToolTip
            // 
            this._systemMonitorToolTip.IsBalloon = true;
            // 
            // contextMenu_oneGuiContextMenuStrip
            // 
            this.contextMenu_oneGuiContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.configuration_ToolStripMenuItem,
            this.cloneToWindowToolStripMenuItem});
            this.contextMenu_oneGuiContextMenuStrip.Name = "contextMenu_oneGuiContextMenuStrip";
            this.contextMenu_oneGuiContextMenuStrip.Size = new System.Drawing.Size(215, 70);
            // 
            // configuration_ToolStripMenuItem
            // 
            this.configuration_ToolStripMenuItem.Name = "configuration_ToolStripMenuItem";
            this.configuration_ToolStripMenuItem.Size = new System.Drawing.Size(214, 22);
            this.configuration_ToolStripMenuItem.Text = "Configure System Monitor";
            this.configuration_ToolStripMenuItem.Click += new System.EventHandler(this.configuration_ToolStripMenuItem_Click);
            // 
            // SysMon_oneGuiSplitContainer
            // 
            this.SysMon_oneGuiSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.SysMon_oneGuiSplitContainer.Location = new System.Drawing.Point(0, 0);
            this.SysMon_oneGuiSplitContainer.Name = "SysMon_oneGuiSplitContainer";
            this.SysMon_oneGuiSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // SysMon_oneGuiSplitContainer.Panel1
            // 
            this.SysMon_oneGuiSplitContainer.Panel1.Controls.Add(this.oneGuiTabControl1);
            this.SysMon_oneGuiSplitContainer.Size = new System.Drawing.Size(500, 748);
            this.SysMon_oneGuiSplitContainer.SplitterDistance = 630;
            this.SysMon_oneGuiSplitContainer.SplitterWidth = 9;
            this.SysMon_oneGuiSplitContainer.TabIndex = 0;
            // 
            // oneGuiTabControl1
            // 
            this.oneGuiTabControl1.Controls.Add(this.tabPage1);
            this.oneGuiTabControl1.Controls.Add(this.tabPage2);
            this.oneGuiTabControl1.Controls.Add(this.tabPage3);
            this.oneGuiTabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiTabControl1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiTabControl1.HotTrack = true;
            this.oneGuiTabControl1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiTabControl1.Multiline = true;
            this.oneGuiTabControl1.Name = "oneGuiTabControl1";
            this.oneGuiTabControl1.Padding = new System.Drawing.Point(10, 5);
            this.oneGuiTabControl1.SelectedIndex = 0;
            this.oneGuiTabControl1.Size = new System.Drawing.Size(500, 630);
            this.oneGuiTabControl1.TabIndex = 0;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.graphContainer_tableLayoutPanel);
            this.tabPage1.Location = new System.Drawing.Point(4, 26);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(492, 600);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Bar Graph";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // tabPage2
            // 
            this.tabPage2.Location = new System.Drawing.Point(4, 26);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(492, 600);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Timeline";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // tabPage3
            // 
            this.tabPage3.Location = new System.Drawing.Point(4, 26);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Size = new System.Drawing.Size(492, 600);
            this.tabPage3.TabIndex = 2;
            this.tabPage3.Text = "Hybrid";
            this.tabPage3.UseVisualStyleBackColor = true;
            // 
            // config_oneGuiPanel
            // 
            this.config_oneGuiPanel.ActiveConfigDefinition = null;
            this.config_oneGuiPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.config_oneGuiPanel.ConnectionDefn = null;
            this.config_oneGuiPanel.Location = new System.Drawing.Point(0, 0);
            this.config_oneGuiPanel.Name = "config_oneGuiPanel";
            this.config_oneGuiPanel.Size = new System.Drawing.Size(500, 748);
            this.config_oneGuiPanel.TabIndex = 0;
            // 
            // cloneToWindowToolStripMenuItem
            // 
            this.cloneToWindowToolStripMenuItem.Name = "cloneToWindowToolStripMenuItem";
            this.cloneToWindowToolStripMenuItem.Size = new System.Drawing.Size(214, 22);
            this.cloneToWindowToolStripMenuItem.Text = "Clone To Window";
            this.cloneToWindowToolStripMenuItem.Click += new System.EventHandler(this.cloneToWindowToolStripMenuItem_Click);
            // 
            // SystemMonitorControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ContextMenuStrip = this.contextMenu_oneGuiContextMenuStrip;
            this.Controls.Add(this.SysMon_oneGuiSplitContainer);
            this.Controls.Add(this.config_oneGuiPanel);
            this.Name = "SystemMonitorControl";
            this.Size = new System.Drawing.Size(500, 748);
            this.Enter += new System.EventHandler(this.SystemMonitorControl_Enter);
            this.contextMenu_oneGuiContextMenuStrip.ResumeLayout(false);
            this.SysMon_oneGuiSplitContainer.Panel1.ResumeLayout(false);
            this.SysMon_oneGuiSplitContainer.ResumeLayout(false);
            this.oneGuiTabControl1.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TableLayoutPanel graphContainer_tableLayoutPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _systemMonitorToolTip;
        private TrafodionContextMenuStrip contextMenu_oneGuiContextMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem configuration_ToolStripMenuItem;
        private SystemMonitorConfigurationControl config_oneGuiPanel;
        private TrafodionSplitContainer SysMon_oneGuiSplitContainer;
        private TrafodionTabControl oneGuiTabControl1;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage tabPage1;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage tabPage2;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage tabPage3;
        private System.Windows.Forms.ToolStripMenuItem cloneToWindowToolStripMenuItem;
    }
}
