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
    partial class ConfigureMappingUserControl
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
            this.TrafodionGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theAssociationReason = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theParameterGridPanel = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theReportNamesGrid = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theCallingReportLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionGroupBox2.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionGroupBox2
            // 
            this.TrafodionGroupBox2.Controls.Add(this.TrafodionGroupBox1);
            this.TrafodionGroupBox2.Controls.Add(this._theParameterGridPanel);
            this.TrafodionGroupBox2.Controls.Add(this.TrafodionPanel2);
            this.TrafodionGroupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox2.Location = new System.Drawing.Point(0, 0);
            this.TrafodionGroupBox2.Name = "TrafodionGroupBox2";
            this.TrafodionGroupBox2.Size = new System.Drawing.Size(485, 411);
            this.TrafodionGroupBox2.TabIndex = 2;
            this.TrafodionGroupBox2.TabStop = false;
            this.TrafodionGroupBox2.Text = "Linking Reports";
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._theAssociationReason);
            this.TrafodionGroupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(3, 152);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(479, 108);
            this.TrafodionGroupBox1.TabIndex = 2;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Link Reason (Describes the association between the reports)";
            // 
            // _theAssociationReason
            // 
            this._theAssociationReason.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAssociationReason.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAssociationReason.Location = new System.Drawing.Point(3, 17);
            this._theAssociationReason.Multiline = true;
            this._theAssociationReason.Name = "_theAssociationReason";
            this._theAssociationReason.Size = new System.Drawing.Size(473, 88);
            this._theAssociationReason.TabIndex = 0;
            // 
            // _theParameterGridPanel
            // 
            this._theParameterGridPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theParameterGridPanel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theParameterGridPanel.Location = new System.Drawing.Point(3, 260);
            this._theParameterGridPanel.Name = "_theParameterGridPanel";
            this._theParameterGridPanel.Size = new System.Drawing.Size(479, 148);
            this._theParameterGridPanel.TabIndex = 1;
            this._theParameterGridPanel.TabStop = false;
            this._theParameterGridPanel.Text = "Map Columns from calling report to Parameters of called report";
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this._theReportNamesGrid);
            this.TrafodionPanel2.Controls.Add(this._theCallingReportLabel);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel2);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel1);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel2.Location = new System.Drawing.Point(3, 17);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(479, 135);
            this.TrafodionPanel2.TabIndex = 0;
            // 
            // _theReportNamesGrid
            // 
            this._theReportNamesGrid.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theReportNamesGrid.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theReportNamesGrid.Location = new System.Drawing.Point(0, 43);
            this._theReportNamesGrid.Name = "_theReportNamesGrid";
            this._theReportNamesGrid.Size = new System.Drawing.Size(479, 92);
            this._theReportNamesGrid.TabIndex = 4;
            // 
            // _theCallingReportLabel
            // 
            this._theCallingReportLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theCallingReportLabel.AutoSize = true;
            this._theCallingReportLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._theCallingReportLabel.Location = new System.Drawing.Point(94, 2);
            this._theCallingReportLabel.Name = "_theCallingReportLabel";
            this._theCallingReportLabel.Size = new System.Drawing.Size(0, 13);
            this._theCallingReportLabel.TabIndex = 3;
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(3, 2);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(89, 13);
            this.TrafodionLabel2.TabIndex = 2;
            this.TrafodionLabel2.Text = "Drill from Report:";
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(5, 18);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(102, 13);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "Drill down to Report";
            // 
            // ConfigureMappingUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionGroupBox2);
            this.Name = "ConfigureMappingUserControl";
            this.Size = new System.Drawing.Size(485, 411);
            this.TrafodionGroupBox2.ResumeLayout(false);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.TrafodionPanel2.ResumeLayout(false);
            this.TrafodionPanel2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theParameterGridPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theCallingReportLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theAssociationReason;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theReportNamesGrid;
    }
}
