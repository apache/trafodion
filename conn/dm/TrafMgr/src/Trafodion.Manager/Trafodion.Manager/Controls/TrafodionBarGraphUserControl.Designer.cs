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
    partial class TrafodionBarGraphUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionBarGraphUserControl));
            this.graph_TrafodionRealTimeBarGraph = new Trafodion.Manager.Framework.Controls.TrafodionRealTimeBarGraph();
            this.statusStrip7 = new System.Windows.Forms.StatusStrip();
            this.graphLabel_toolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
            this.statusStrip7.SuspendLayout();
            this.SuspendLayout();
            // 
            // graph_TrafodionRealTimeBarGraph
            // 
            this.graph_TrafodionRealTimeBarGraph.Aggregate = false;
            this.graph_TrafodionRealTimeBarGraph.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.graph_TrafodionRealTimeBarGraph.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.graph_TrafodionRealTimeBarGraph.BackColor = System.Drawing.SystemColors.Control;
            this.graph_TrafodionRealTimeBarGraph.ChartRawMaxValue = 1;
            this.graph_TrafodionRealTimeBarGraph.DownCPUs = ((System.Collections.ArrayList)(resources.GetObject("graph_TrafodionRealTimeBarGraph.DownCPUs")));
            this.graph_TrafodionRealTimeBarGraph.FancyTooltips = false;
            this.graph_TrafodionRealTimeBarGraph.GraphBrushes = ((System.Collections.ArrayList)(resources.GetObject("graph_TrafodionRealTimeBarGraph.GraphBrushes")));
            this.graph_TrafodionRealTimeBarGraph.GraphValues = null;
            this.graph_TrafodionRealTimeBarGraph.IsDrilled = false;
            this.graph_TrafodionRealTimeBarGraph.IsIntervalBased = false;
            this.graph_TrafodionRealTimeBarGraph.Location = new System.Drawing.Point(39, 0);
            this.graph_TrafodionRealTimeBarGraph.Margin = new System.Windows.Forms.Padding(0);
            this.graph_TrafodionRealTimeBarGraph.MetricLabel = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            this.graph_TrafodionRealTimeBarGraph.MetricUnitSuffix = global::Trafodion.Manager.Properties.Resources.NCICustomPrompt;
            this.graph_TrafodionRealTimeBarGraph.Name = "graph_TrafodionRealTimeBarGraph";
            this.graph_TrafodionRealTimeBarGraph.SegmentSepCol = System.Drawing.Color.Black;
            this.graph_TrafodionRealTimeBarGraph.ShowcaseSegment = -1;
            this.graph_TrafodionRealTimeBarGraph.ShowSegmentSeperator = true;
            this.graph_TrafodionRealTimeBarGraph.Size = new System.Drawing.Size(111, 150);
            this.graph_TrafodionRealTimeBarGraph.StanDeviate = false;
            this.graph_TrafodionRealTimeBarGraph.StanThreshArray = new System.Collections.Hashtable[] {
        null,
        null,
        null};
            this.graph_TrafodionRealTimeBarGraph.TabIndex = 0;
            this.graph_TrafodionRealTimeBarGraph.ThresholdExceededIndicator = true;
            this.graph_TrafodionRealTimeBarGraph.ToolTipFontSize = 10;
            // 
            // statusStrip7
            // 
            this.statusStrip7.AutoSize = false;
            this.statusStrip7.BackColor = System.Drawing.SystemColors.ControlDarkDark;
            this.statusStrip7.Dock = System.Windows.Forms.DockStyle.Left;
            this.statusStrip7.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.graphLabel_toolStripStatusLabel});
            this.statusStrip7.Location = new System.Drawing.Point(0, 0);
            this.statusStrip7.Name = "statusStrip7";
            this.statusStrip7.Size = new System.Drawing.Size(39, 150);
            this.statusStrip7.SizingGrip = false;
            this.statusStrip7.TabIndex = 11;
            this.statusStrip7.Text = "statusStrip7";
            // 
            // graphLabel_toolStripStatusLabel
            // 
            this.graphLabel_toolStripStatusLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.graphLabel_toolStripStatusLabel.ForeColor = System.Drawing.SystemColors.ActiveCaptionText;
            this.graphLabel_toolStripStatusLabel.Margin = new System.Windows.Forms.Padding(0);
            this.graphLabel_toolStripStatusLabel.Name = "graphLabel_toolStripStatusLabel";
            this.graphLabel_toolStripStatusLabel.Size = new System.Drawing.Size(37, 106);
            this.graphLabel_toolStripStatusLabel.Spring = true;
            this.graphLabel_toolStripStatusLabel.Text = "CPU Queue Length";
            this.graphLabel_toolStripStatusLabel.TextDirection = System.Windows.Forms.ToolStripTextDirection.Vertical270;
            // 
            // TrafodionBarGraphUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.ControlDarkDark;
            this.Controls.Add(this.statusStrip7);
            this.Controls.Add(this.graph_TrafodionRealTimeBarGraph);
            this.Name = "TrafodionBarGraphUserControl";
            this.statusStrip7.ResumeLayout(false);
            this.statusStrip7.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionRealTimeBarGraph graph_TrafodionRealTimeBarGraph;
        private System.Windows.Forms.StatusStrip statusStrip7;
        private System.Windows.Forms.ToolStripStatusLabel graphLabel_toolStripStatusLabel;
    }
}
