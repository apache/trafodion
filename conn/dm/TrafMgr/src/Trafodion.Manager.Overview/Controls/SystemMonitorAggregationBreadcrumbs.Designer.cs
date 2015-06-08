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
﻿namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class SystemMonitorAggregationBreadcrumbs
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
            this.full_linkLabel = new System.Windows.Forms.LinkLabel();
            this.arrows_label = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.segment_label = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.segNum_label = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.tableLayoutPanel1.SuspendLayout();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // full_linkLabel
            // 
            this.full_linkLabel.AutoEllipsis = true;
            this.full_linkLabel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.full_linkLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.full_linkLabel.Location = new System.Drawing.Point(3, 0);
            this.full_linkLabel.Name = "full_linkLabel";
            this.full_linkLabel.Size = new System.Drawing.Size(101, 14);
            this.full_linkLabel.TabIndex = 0;
            this.full_linkLabel.TabStop = true;
            this.full_linkLabel.Text = "Full System View";
            // 
            // arrows_label
            // 
            this.arrows_label.Dock = System.Windows.Forms.DockStyle.Fill;
            this.arrows_label.Location = new System.Drawing.Point(110, 0);
            this.arrows_label.Name = "arrows_label";
            this.arrows_label.Size = new System.Drawing.Size(19, 14);
            this.arrows_label.TabIndex = 1;
            this.arrows_label.Text = ">>";
            // 
            // segment_label
            // 
            this.segment_label.AutoEllipsis = true;
            this.segment_label.Dock = System.Windows.Forms.DockStyle.Fill;
            this.segment_label.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.segment_label.Location = new System.Drawing.Point(135, 0);
            this.segment_label.Name = "segment_label";
            this.segment_label.Size = new System.Drawing.Size(55, 14);
            this.segment_label.TabIndex = 2;
            this.segment_label.Text = "Segment";
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.AutoSize = true;
            this.tableLayoutPanel1.ColumnCount = 4;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 63.72145F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 25F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 36.27855F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 22F));
            this.tableLayoutPanel1.Controls.Add(this.full_linkLabel, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.arrows_label, 1, 0);
            this.tableLayoutPanel1.Controls.Add(this.segment_label, 2, 0);
            this.tableLayoutPanel1.Controls.Add(this.segNum_label, 3, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(2, 2);
            this.tableLayoutPanel1.MaximumSize = new System.Drawing.Size(216, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(216, 14);
            this.tableLayoutPanel1.TabIndex = 3;
            // 
            // segNum_label
            // 
            this.segNum_label.AutoSize = true;
            this.segNum_label.Dock = System.Windows.Forms.DockStyle.Left;
            this.segNum_label.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.segNum_label.Location = new System.Drawing.Point(193, 0);
            this.segNum_label.Margin = new System.Windows.Forms.Padding(0, 0, 3, 0);
            this.segNum_label.Name = "segNum_label";
            this.segNum_label.Size = new System.Drawing.Size(14, 14);
            this.segNum_label.TabIndex = 3;
            this.segNum_label.Text = "3";
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this.tableLayoutPanel1);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel1.Location = new System.Drawing.Point(1, 1);
            this.panel1.Name = "panel1";
            this.panel1.Padding = new System.Windows.Forms.Padding(2);
            this.panel1.Size = new System.Drawing.Size(318, 18);
            this.panel1.TabIndex = 4;
            // 
            // SystemMonitorAggregationBreadcrumbs
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.panel1);
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "SystemMonitorAggregationBreadcrumbs";
            this.Padding = new System.Windows.Forms.Padding(1);
            this.Size = new System.Drawing.Size(320, 20);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.LinkLabel full_linkLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel arrows_label;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel segment_label;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel segNum_label;
    }
}
