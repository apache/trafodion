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
    partial class TriageSpaceUserControl
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
            this._theRootPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTopSplitCotainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theGraphAndGridSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theTriageChartContainer = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theFilterAndGridSplitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._theFilterContainer = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionToolStrip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this.toolStripButton1 = new System.Windows.Forms.ToolStripButton();
            this._theTriageGridContainer = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theStatementCountersContainer = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theTriageChartControl = new Trafodion.Manager.WorkloadArea.Controls.TriageChartControl();
            this.workloadCountersUserControl1 = new Trafodion.Manager.WorkloadArea.Controls.WorkloadCountersUserControl();
            this._theRootPanel.SuspendLayout();
            this._theTopSplitCotainer.Panel1.SuspendLayout();
            this._theTopSplitCotainer.Panel2.SuspendLayout();
            this._theTopSplitCotainer.SuspendLayout();
            this._theGraphAndGridSplitContainer.Panel1.SuspendLayout();
            this._theGraphAndGridSplitContainer.Panel2.SuspendLayout();
            this._theGraphAndGridSplitContainer.SuspendLayout();
            this._theTriageChartContainer.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this._theFilterAndGridSplitContainer.Panel1.SuspendLayout();
            this._theFilterAndGridSplitContainer.Panel2.SuspendLayout();
            this._theFilterAndGridSplitContainer.SuspendLayout();
            this.TrafodionToolStrip1.SuspendLayout();
            this._theStatementCountersContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theRootPanel
            // 
            this._theRootPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theRootPanel.Controls.Add(this._theTopSplitCotainer);
            this._theRootPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theRootPanel.Location = new System.Drawing.Point(0, 0);
            this._theRootPanel.Name = "_theRootPanel";
            this._theRootPanel.Size = new System.Drawing.Size(873, 582);
            this._theRootPanel.TabIndex = 0;
            // 
            // _theTopSplitCotainer
            // 
            this._theTopSplitCotainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTopSplitCotainer.Location = new System.Drawing.Point(0, 0);
            this._theTopSplitCotainer.Name = "_theTopSplitCotainer";
            this._theTopSplitCotainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _theTopSplitCotainer.Panel1
            // 
            this._theTopSplitCotainer.Panel1.Controls.Add(this._theGraphAndGridSplitContainer);
            // 
            // _theTopSplitCotainer.Panel2
            // 
            this._theTopSplitCotainer.Panel2.Controls.Add(this._theStatementCountersContainer);
            this._theTopSplitCotainer.Panel2Collapsed = true;
            this._theTopSplitCotainer.Size = new System.Drawing.Size(873, 582);
            this._theTopSplitCotainer.SplitterDistance = 392;
            this._theTopSplitCotainer.SplitterWidth = 9;
            this._theTopSplitCotainer.TabIndex = 0;
            // 
            // _theGraphAndGridSplitContainer
            // 
            this._theGraphAndGridSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theGraphAndGridSplitContainer.Location = new System.Drawing.Point(0, 0);
            this._theGraphAndGridSplitContainer.Name = "_theGraphAndGridSplitContainer";
            this._theGraphAndGridSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _theGraphAndGridSplitContainer.Panel1
            // 
            this._theGraphAndGridSplitContainer.Panel1.Controls.Add(this._theTriageChartContainer);
            // 
            // _theGraphAndGridSplitContainer.Panel2
            // 
            this._theGraphAndGridSplitContainer.Panel2.Controls.Add(this.TrafodionPanel1);
            this._theGraphAndGridSplitContainer.Size = new System.Drawing.Size(873, 582);
            this._theGraphAndGridSplitContainer.SplitterDistance = 221;
            this._theGraphAndGridSplitContainer.SplitterWidth = 9;
            this._theGraphAndGridSplitContainer.TabIndex = 0;
            // 
            // _theTriageChartContainer
            // 
            this._theTriageChartContainer.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theTriageChartContainer.Controls.Add(this._theTriageChartControl);
            this._theTriageChartContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTriageChartContainer.Location = new System.Drawing.Point(0, 0);
            this._theTriageChartContainer.Name = "_theTriageChartContainer";
            this._theTriageChartContainer.Size = new System.Drawing.Size(873, 221);
            this._theTriageChartContainer.TabIndex = 0;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._theFilterAndGridSplitContainer);
            this.TrafodionPanel1.Controls.Add(this.TrafodionToolStrip1);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(873, 352);
            this.TrafodionPanel1.TabIndex = 1;
            // 
            // _theFilterAndGridSplitContainer
            // 
            this._theFilterAndGridSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theFilterAndGridSplitContainer.Location = new System.Drawing.Point(24, 0);
            this._theFilterAndGridSplitContainer.Name = "_theFilterAndGridSplitContainer";
            // 
            // _theFilterAndGridSplitContainer.Panel1
            // 
            this._theFilterAndGridSplitContainer.Panel1.Controls.Add(this._theFilterContainer);
            this._theFilterAndGridSplitContainer.Panel1MinSize = 278;
            // 
            // _theFilterAndGridSplitContainer.Panel2
            // 
            this._theFilterAndGridSplitContainer.Panel2.Controls.Add(this._theTriageGridContainer);
            this._theFilterAndGridSplitContainer.Size = new System.Drawing.Size(849, 352);
            this._theFilterAndGridSplitContainer.SplitterDistance = 282;
            this._theFilterAndGridSplitContainer.SplitterWidth = 9;
            this._theFilterAndGridSplitContainer.TabIndex = 0;
            // 
            // _theFilterContainer
            // 
            this._theFilterContainer.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theFilterContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theFilterContainer.Location = new System.Drawing.Point(0, 0);
            this._theFilterContainer.Name = "_theFilterContainer";
            this._theFilterContainer.Size = new System.Drawing.Size(282, 352);
            this._theFilterContainer.TabIndex = 0;
            // 
            // TrafodionToolStrip1
            // 
            this.TrafodionToolStrip1.Dock = System.Windows.Forms.DockStyle.Left;
            this.TrafodionToolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButton1});
            this.TrafodionToolStrip1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionToolStrip1.Name = "TrafodionToolStrip1";
            this.TrafodionToolStrip1.Size = new System.Drawing.Size(24, 352);
            this.TrafodionToolStrip1.TabIndex = 1;
            this.TrafodionToolStrip1.Text = "TrafodionToolStrip1";
            // 
            // toolStripButton1
            // 
            this.toolStripButton1.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.toolStripButton1.Image = global::Trafodion.Manager.WorkloadArea.Properties.Resources._2leftarrow;
            this.toolStripButton1.ImageAlign = System.Drawing.ContentAlignment.TopCenter;
            this.toolStripButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripButton1.Name = "toolStripButton1";
            this.toolStripButton1.Size = new System.Drawing.Size(21, 113);
            this.toolStripButton1.Text = "Workload Filter";
            this.toolStripButton1.TextAlign = System.Drawing.ContentAlignment.BottomCenter;
            this.toolStripButton1.TextDirection = System.Windows.Forms.ToolStripTextDirection.Vertical270;
            this.toolStripButton1.TextImageRelation = System.Windows.Forms.TextImageRelation.TextAboveImage;
            this.toolStripButton1.Click += new System.EventHandler(this.toolStripButton1_Click);
            // 
            // _theTriageGridContainer
            // 
            this._theTriageGridContainer.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theTriageGridContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTriageGridContainer.Location = new System.Drawing.Point(0, 0);
            this._theTriageGridContainer.Name = "_theTriageGridContainer";
            this._theTriageGridContainer.Size = new System.Drawing.Size(558, 352);
            this._theTriageGridContainer.TabIndex = 0;
            // 
            // _theStatementCountersContainer
            // 
            this._theStatementCountersContainer.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theStatementCountersContainer.Controls.Add(this.workloadCountersUserControl1);
            this._theStatementCountersContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theStatementCountersContainer.Location = new System.Drawing.Point(0, 0);
            this._theStatementCountersContainer.Name = "_theStatementCountersContainer";
            this._theStatementCountersContainer.Size = new System.Drawing.Size(150, 41);
            this._theStatementCountersContainer.TabIndex = 0;
            // 
            // _theTriageChartControl
            // 
            this._theTriageChartControl.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this._theTriageChartControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTriageChartControl.Location = new System.Drawing.Point(0, 0);
            this._theTriageChartControl.Name = "_theTriageChartControl";
            this._theTriageChartControl.Size = new System.Drawing.Size(873, 221);
            this._theTriageChartControl.TabIndex = 0;
            // 
            // workloadCountersUserControl1
            // 
            this.workloadCountersUserControl1.Location = new System.Drawing.Point(3, 9);
            this.workloadCountersUserControl1.Name = "workloadCountersUserControl1";
            this.workloadCountersUserControl1.Size = new System.Drawing.Size(212, 169);
            this.workloadCountersUserControl1.TabIndex = 0;
            // 
            // TriageSpaceUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theRootPanel);
            this.Name = "TriageSpaceUserControl";
            this.Size = new System.Drawing.Size(873, 582);
            this._theRootPanel.ResumeLayout(false);
            this._theTopSplitCotainer.Panel1.ResumeLayout(false);
            this._theTopSplitCotainer.Panel2.ResumeLayout(false);
            this._theTopSplitCotainer.ResumeLayout(false);
            this._theGraphAndGridSplitContainer.Panel1.ResumeLayout(false);
            this._theGraphAndGridSplitContainer.Panel2.ResumeLayout(false);
            this._theGraphAndGridSplitContainer.ResumeLayout(false);
            this._theTriageChartContainer.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this._theFilterAndGridSplitContainer.Panel1.ResumeLayout(false);
            this._theFilterAndGridSplitContainer.Panel2.ResumeLayout(false);
            this._theFilterAndGridSplitContainer.ResumeLayout(false);
            this.TrafodionToolStrip1.ResumeLayout(false);
            this.TrafodionToolStrip1.PerformLayout();
            this._theStatementCountersContainer.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theRootPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _theTopSplitCotainer;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _theGraphAndGridSplitContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionSplitContainer _theFilterAndGridSplitContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theTriageChartContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theFilterContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theTriageGridContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theStatementCountersContainer;
        private TriageChartControl _theTriageChartControl;
        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip TrafodionToolStrip1;
        private System.Windows.Forms.ToolStripButton toolStripButton1;
        private WorkloadCountersUserControl workloadCountersUserControl1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;

    }
}
