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
    partial class TrafodionLineGraphUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TrafodionLineGraphUserControl));
            this.graph_TrafodionRealTimeLineGraph = new Trafodion.Manager.Framework.Controls.TrafodionRealTimeLineGraph();
            this.SuspendLayout();
            // 
            // graph_TrafodionRealTimeLineGraph
            // 
            this.graph_TrafodionRealTimeLineGraph.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.graph_TrafodionRealTimeLineGraph.BackColor = System.Drawing.SystemColors.Control;
            this.graph_TrafodionRealTimeLineGraph.Dock = System.Windows.Forms.DockStyle.Fill;
            this.graph_TrafodionRealTimeLineGraph.GraphColors = ((System.Collections.ArrayList)(resources.GetObject("graph_TrafodionRealTimeLineGraph.GraphColors")));
            this.graph_TrafodionRealTimeLineGraph.GraphMaxRange = 50;
            this.graph_TrafodionRealTimeLineGraph.GraphValues = null;
            this.graph_TrafodionRealTimeLineGraph.Location = new System.Drawing.Point(2, 2);
            this.graph_TrafodionRealTimeLineGraph.Margin = new System.Windows.Forms.Padding(0);
            this.graph_TrafodionRealTimeLineGraph.Name = "graph_TrafodionRealTimeLineGraph";
            this.graph_TrafodionRealTimeLineGraph.Padding = new System.Windows.Forms.Padding(3);
            this.graph_TrafodionRealTimeLineGraph.Size = new System.Drawing.Size(1053, 123);
            this.graph_TrafodionRealTimeLineGraph.TabIndex = 0;
            // 
            // TrafodionLineGraphUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.Silver;
            this.Controls.Add(this.graph_TrafodionRealTimeLineGraph);
            this.Name = "TrafodionLineGraphUserControl";
            this.Padding = new System.Windows.Forms.Padding(2);
            this.Size = new System.Drawing.Size(1057, 127);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionRealTimeLineGraph graph_TrafodionRealTimeLineGraph;
    }
}
