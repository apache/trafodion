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
﻿namespace Trafodion.Manager.Framework.Controls
{
    partial class TrafodionHybridGraphUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionHybridGraphUserControl));
            this.statusStrip7 = new System.Windows.Forms.StatusStrip();
            this.graphLabel_toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.splitContainerMain_splitContainer = new System.Windows.Forms.SplitContainer();
            this.barGraph_TrafodionRealTimeBarGraph = new Trafodion.Manager.Framework.Controls.TrafodionRealTimeBarGraph();
            this.lineGraph_TrafodionRealTimeLineGraph = new Trafodion.Manager.Framework.Controls.TrafodionRealTimeLineGraph();
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.statusStrip7.SuspendLayout();
            this.splitContainerMain_splitContainer.Panel1.SuspendLayout();
            this.splitContainerMain_splitContainer.Panel2.SuspendLayout();
            this.splitContainerMain_splitContainer.SuspendLayout();
            this.SuspendLayout();
            // 
            // statusStrip7
            // 
            this.statusStrip7.AutoSize = false;
            this.statusStrip7.BackColor = System.Drawing.SystemColors.ControlDarkDark;
            this.statusStrip7.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.graphLabel_toolStripStatusLabel});
            this.statusStrip7.Location = new System.Drawing.Point(0, 322);
            this.statusStrip7.Name = "statusStrip7";
            this.statusStrip7.Size = new System.Drawing.Size(491, 15);
            this.statusStrip7.SizingGrip = false;
            this.statusStrip7.TabIndex = 11;
            this.statusStrip7.Text = "statusStrip7";
            // 
            // graphLabel_toolStripStatusLabel
            // 
            this.graphLabel_toolStripStatusLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.graphLabel_toolStripStatusLabel.ForeColor = System.Drawing.Color.White;
            this.graphLabel_toolStripStatusLabel.Margin = new System.Windows.Forms.Padding(0);
            this.graphLabel_toolStripStatusLabel.Name = "graphLabel_toolStripStatusLabel";
            this.graphLabel_toolStripStatusLabel.Size = new System.Drawing.Size(445, 15);
            this.graphLabel_toolStripStatusLabel.Spring = true;
            this.graphLabel_toolStripStatusLabel.Text = "CPU Queue Length";
            this.graphLabel_toolStripStatusLabel.TextDirection = System.Windows.Forms.ToolStripTextDirection.Horizontal;
            this.graphLabel_toolStripStatusLabel.Click += new System.EventHandler(this.graphLabel_toolStripStatusLabel_Click);
            // 
            // splitContainerMain_splitContainer
            // 
            this.splitContainerMain_splitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainerMain_splitContainer.Location = new System.Drawing.Point(0, 0);
            this.splitContainerMain_splitContainer.Name = "splitContainerMain_splitContainer";
            this.splitContainerMain_splitContainer.Panel1MinSize = 2;
            // 
            // splitContainerMain_splitContainer.Panel1
            // 
            this.splitContainerMain_splitContainer.Panel1.Controls.Add(this.barGraph_TrafodionRealTimeBarGraph);
            // 
            // splitContainerMain_splitContainer.Panel2
            // 
            this.splitContainerMain_splitContainer.Panel2.BackColor = System.Drawing.Color.Silver;
            this.splitContainerMain_splitContainer.Panel2.Controls.Add(this.lineGraph_TrafodionRealTimeLineGraph);
            this.splitContainerMain_splitContainer.Panel2.Padding = new System.Windows.Forms.Padding(3, 3, 5, 2);
            this.splitContainerMain_splitContainer.Size = new System.Drawing.Size(491, 322);
            this.splitContainerMain_splitContainer.SplitterDistance = 259;
            this.splitContainerMain_splitContainer.TabIndex = 12;
            // 
            // barGraph_TrafodionRealTimeBarGraph
            // 
            this.barGraph_TrafodionRealTimeBarGraph.Aggregate = false;
            this.barGraph_TrafodionRealTimeBarGraph.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.barGraph_TrafodionRealTimeBarGraph.BackColor = System.Drawing.SystemColors.Control;
            this.barGraph_TrafodionRealTimeBarGraph.ChartRawMaxValue = 1;
            this.barGraph_TrafodionRealTimeBarGraph.Dock = System.Windows.Forms.DockStyle.Fill;
            this.barGraph_TrafodionRealTimeBarGraph.DownCPUs = ((System.Collections.ArrayList)(resources.GetObject("barGraph_TrafodionRealTimeBarGraph.DownCPUs")));
            this.barGraph_TrafodionRealTimeBarGraph.FancyTooltips = false;
            this.barGraph_TrafodionRealTimeBarGraph.GraphBrushes = ((System.Collections.ArrayList)(resources.GetObject("barGraph_TrafodionRealTimeBarGraph.GraphBrushes")));
            this.barGraph_TrafodionRealTimeBarGraph.GraphValues = null;
            this.barGraph_TrafodionRealTimeBarGraph.IsDrilled = false;
            this.barGraph_TrafodionRealTimeBarGraph.IsIntervalBased = false;
            this.barGraph_TrafodionRealTimeBarGraph.Location = new System.Drawing.Point(0, 0);
            this.barGraph_TrafodionRealTimeBarGraph.Margin = new System.Windows.Forms.Padding(0);
            this.barGraph_TrafodionRealTimeBarGraph.MetricLabel = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            this.barGraph_TrafodionRealTimeBarGraph.MetricType = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            this.barGraph_TrafodionRealTimeBarGraph.MetricUnitSuffix = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            this.barGraph_TrafodionRealTimeBarGraph.Name = "barGraph_TrafodionRealTimeBarGraph";
            this.barGraph_TrafodionRealTimeBarGraph.SegmentSepCol = System.Drawing.Color.Black;
            this.barGraph_TrafodionRealTimeBarGraph.ShowcaseSegment = -1;
            this.barGraph_TrafodionRealTimeBarGraph.ShowSegmentSeperator = true;
            this.barGraph_TrafodionRealTimeBarGraph.Size = new System.Drawing.Size(259, 322);
            this.barGraph_TrafodionRealTimeBarGraph.StanDeviate = false;
            this.barGraph_TrafodionRealTimeBarGraph.StanThreshArray = new System.Collections.Hashtable[] {
        null,
        null,
        null};
            this.barGraph_TrafodionRealTimeBarGraph.TabIndex = 0;
            this.barGraph_TrafodionRealTimeBarGraph.ThresholdExceededIndicator = true;
            this.barGraph_TrafodionRealTimeBarGraph.ToolTipFontSize = 10;
            this.barGraph_TrafodionRealTimeBarGraph.TooltipLabel = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            // 
            // lineGraph_TrafodionRealTimeLineGraph
            // 
            this.lineGraph_TrafodionRealTimeLineGraph.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.lineGraph_TrafodionRealTimeLineGraph.BackColor = System.Drawing.SystemColors.Control;
            this.lineGraph_TrafodionRealTimeLineGraph.Dock = System.Windows.Forms.DockStyle.Fill;
            this.lineGraph_TrafodionRealTimeLineGraph.GraphColors = ((System.Collections.ArrayList)(resources.GetObject("lineGraph_TrafodionRealTimeLineGraph.GraphColors")));
            this.lineGraph_TrafodionRealTimeLineGraph.GraphMaxRange = 50;
            this.lineGraph_TrafodionRealTimeLineGraph.GraphValues = null;
            this.lineGraph_TrafodionRealTimeLineGraph.Location = new System.Drawing.Point(3, 3);
            this.lineGraph_TrafodionRealTimeLineGraph.Margin = new System.Windows.Forms.Padding(0);
            this.lineGraph_TrafodionRealTimeLineGraph.Name = "lineGraph_TrafodionRealTimeLineGraph";
            this.lineGraph_TrafodionRealTimeLineGraph.Padding = new System.Windows.Forms.Padding(3);
            this.lineGraph_TrafodionRealTimeLineGraph.Size = new System.Drawing.Size(220, 317);
            this.lineGraph_TrafodionRealTimeLineGraph.TabIndex = 1;
            // 
            // toolTip1
            // 
            this.toolTip1.AutoPopDelay = 5000;
            this.toolTip1.InitialDelay = 0;
            this.toolTip1.IsBalloon = true;
            this.toolTip1.OwnerDraw = true;
            this.toolTip1.ReshowDelay = 100;
            this.toolTip1.UseAnimation = false;
            this.toolTip1.UseFading = false;
            // 
            // TrafodionHybridGraphUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlDarkDark;
            this.Controls.Add(this.splitContainerMain_splitContainer);
            this.Controls.Add(this.statusStrip7);
            this.Name = "TrafodionHybridGraphUserControl";
            this.Size = new System.Drawing.Size(491, 337);
            this.statusStrip7.ResumeLayout(false);
            this.statusStrip7.PerformLayout();
            this.splitContainerMain_splitContainer.Panel1.ResumeLayout(false);
            this.splitContainerMain_splitContainer.Panel2.ResumeLayout(false);
            this.splitContainerMain_splitContainer.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionRealTimeBarGraph barGraph_TrafodionRealTimeBarGraph;
        private System.Windows.Forms.StatusStrip statusStrip7;
        private System.Windows.Forms.ToolStripStatusLabel graphLabel_toolStripStatusLabel;
        public System.Windows.Forms.SplitContainer splitContainerMain_splitContainer;
        private TrafodionRealTimeLineGraph lineGraph_TrafodionRealTimeLineGraph;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip toolTip1;
    }
}
