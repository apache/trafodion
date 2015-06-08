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
﻿namespace Trafodion.Manager.MetricMiner.Controls
{
    partial class TabbedMetricMinerControl
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TabbedMetricMinerControl));
            this._theNavAndReportsSplitPanel = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theReportNavigatorTabPanel = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this._theReportExplorerTab = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theMMTreeView = new Trafodion.Manager.MetricMiner.Controls.Tree.MetricMinerTreeViewUserControl();
            this._theImageList = new System.Windows.Forms.ImageList(this.components);
            this._theReportsTab = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this.tabPage1 = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this.metricMinerReportTabContent1 = new Trafodion.Manager.MetricMiner.Controls.MetricMinerReportTabContent();
            this.tabPage2 = new Trafodion.Manager.Framework.Controls.TrafodionTabPage();
            this._theMetricMinerNavigator = new Trafodion.Manager.MetricMiner.Controls.MetricMinerNavigator();
            this._theNavAndReportsSplitPanel.Panel1.SuspendLayout();
            this._theNavAndReportsSplitPanel.Panel2.SuspendLayout();
            this._theNavAndReportsSplitPanel.SuspendLayout();
            this._theReportNavigatorTabPanel.SuspendLayout();
            this._theReportExplorerTab.SuspendLayout();
            this._theReportsTab.SuspendLayout();
            this.tabPage1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theNavAndReportsSplitPanel
            // 
            this._theNavAndReportsSplitPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theNavAndReportsSplitPanel.Location = new System.Drawing.Point(0, 24);
            this._theNavAndReportsSplitPanel.Name = "_theNavAndReportsSplitPanel";
            // 
            // _theNavAndReportsSplitPanel.Panel1
            // 
            this._theNavAndReportsSplitPanel.Panel1.Controls.Add(this._theReportNavigatorTabPanel);
            // 
            // _theNavAndReportsSplitPanel.Panel2
            // 
            this._theNavAndReportsSplitPanel.Panel2.Controls.Add(this._theReportsTab);
            this._theNavAndReportsSplitPanel.Size = new System.Drawing.Size(1140, 560);
            this._theNavAndReportsSplitPanel.SplitterDistance = 242;
            this._theNavAndReportsSplitPanel.SplitterWidth = 9;
            this._theNavAndReportsSplitPanel.TabIndex = 1;
            // 
            // _theReportNavigatorTabPanel
            // 
            this._theReportNavigatorTabPanel.Controls.Add(this._theReportExplorerTab);
            this._theReportNavigatorTabPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theReportNavigatorTabPanel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theReportNavigatorTabPanel.ImageList = this._theImageList;
            this._theReportNavigatorTabPanel.Location = new System.Drawing.Point(0, 0);
            this._theReportNavigatorTabPanel.Multiline = true;
            this._theReportNavigatorTabPanel.Name = "_theReportNavigatorTabPanel";
            this._theReportNavigatorTabPanel.Padding = new System.Drawing.Point(10, 5);
            this._theReportNavigatorTabPanel.SelectedIndex = 0;
            this._theReportNavigatorTabPanel.Size = new System.Drawing.Size(242, 560);
            this._theReportNavigatorTabPanel.TabIndex = 0;
            // 
            // _theReportExplorerTab
            // 
            this._theReportExplorerTab.Controls.Add(this._theMMTreeView);
            this._theReportExplorerTab.ImageIndex = 0;
            this._theReportExplorerTab.Location = new System.Drawing.Point(4, 27);
            this._theReportExplorerTab.Name = "_theReportExplorerTab";
            this._theReportExplorerTab.Padding = new System.Windows.Forms.Padding(3);
            this._theReportExplorerTab.Size = new System.Drawing.Size(234, 529);
            this._theReportExplorerTab.TabIndex = 0;
            this._theReportExplorerTab.Text = "Report Explorer";
            this._theReportExplorerTab.UseVisualStyleBackColor = true;
            // 
            // _theMMTreeView
            // 
            this._theMMTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMMTreeView.Location = new System.Drawing.Point(3, 3);
            this._theMMTreeView.Name = "_theMMTreeView";
            this._theMMTreeView.Size = new System.Drawing.Size(228, 523);
            this._theMMTreeView.TabIndex = 0;
            // 
            // _theImageList
            // 
            this._theImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("_theImageList.ImageStream")));
            this._theImageList.TransparentColor = System.Drawing.Color.Transparent;
            this._theImageList.Images.SetKeyName(0, "view_tree.png");
            this._theImageList.Images.SetKeyName(1, "view_detailed.png");
            // 
            // _theReportsTab
            // 
            this._theReportsTab.Controls.Add(this.tabPage1);
            this._theReportsTab.Controls.Add(this.tabPage2);
            this._theReportsTab.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theReportsTab.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theReportsTab.Location = new System.Drawing.Point(0, 0);
            this._theReportsTab.Multiline = true;
            this._theReportsTab.Name = "_theReportsTab";
            this._theReportsTab.Padding = new System.Drawing.Point(10, 5);
            this._theReportsTab.SelectedIndex = 0;
            this._theReportsTab.Size = new System.Drawing.Size(889, 560);
            this._theReportsTab.TabIndex = 0;
            this._theReportsTab.SelectedIndexChanged += new System.EventHandler(this._theReportsTab_SelectedIndexChanged);
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.metricMinerReportTabContent1);
            this.tabPage1.Location = new System.Drawing.Point(4, 26);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(881, 530);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Report 1";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // metricMinerReportTabContent1
            // 
            this.metricMinerReportTabContent1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.metricMinerReportTabContent1.Location = new System.Drawing.Point(3, 3);
            this.metricMinerReportTabContent1.Name = "metricMinerReportTabContent1";
            this.metricMinerReportTabContent1.Size = new System.Drawing.Size(875, 524);
            this.metricMinerReportTabContent1.TabIndex = 0;
            // 
            // tabPage2
            // 
            this.tabPage2.Location = new System.Drawing.Point(4, 26);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(881, 530);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Report 2";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // _theMetricMinerNavigator
            // 
            this._theMetricMinerNavigator.Dock = System.Windows.Forms.DockStyle.Top;
            this._theMetricMinerNavigator.Location = new System.Drawing.Point(0, 0);
            this._theMetricMinerNavigator.Name = "_theMetricMinerNavigator";
            this._theMetricMinerNavigator.Size = new System.Drawing.Size(1140, 24);
            this._theMetricMinerNavigator.TabIndex = 0;
            // 
            // TabbedMetricMinerControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theNavAndReportsSplitPanel);
            this.Controls.Add(this._theMetricMinerNavigator);
            this.Name = "TabbedMetricMinerControl";
            this.Size = new System.Drawing.Size(1140, 584);
            this._theNavAndReportsSplitPanel.Panel1.ResumeLayout(false);
            this._theNavAndReportsSplitPanel.Panel2.ResumeLayout(false);
            this._theNavAndReportsSplitPanel.ResumeLayout(false);
            this._theReportNavigatorTabPanel.ResumeLayout(false);
            this._theReportExplorerTab.ResumeLayout(false);
            this._theReportsTab.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private MetricMinerNavigator _theMetricMinerNavigator;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _theNavAndReportsSplitPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _theReportNavigatorTabPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage _theReportExplorerTab;
        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _theReportsTab;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage tabPage1;
        private Trafodion.Manager.Framework.Controls.TrafodionTabPage tabPage2;
        private MetricMinerReportTabContent metricMinerReportTabContent1;
        private Trafodion.Manager.MetricMiner.Controls.Tree.MetricMinerTreeViewUserControl _theMMTreeView;
        private System.Windows.Forms.ImageList _theImageList;

    }
}
