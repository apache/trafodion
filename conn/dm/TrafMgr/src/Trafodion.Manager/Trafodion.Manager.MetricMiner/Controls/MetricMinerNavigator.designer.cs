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
    partial class MetricMinerNavigator
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
            this._theToolStrip = new Trafodion.Manager.Framework.Controls.TrafodionToolStrip();
            this._theQueriesCombo = new System.Windows.Forms.ToolStripComboBox();
            this._theBreadCrumbPanel = new System.Windows.Forms.FlowLayoutPanel();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theToolStrip.SuspendLayout();
            this._theBreadCrumbPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theToolStrip
            // 
            this._theToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this._theQueriesCombo});
            this._theToolStrip.Location = new System.Drawing.Point(0, 0);
            this._theToolStrip.Name = "_theToolStrip";
            this._theToolStrip.Size = new System.Drawing.Size(744, 25);
            this._theToolStrip.TabIndex = 0;
            this._theToolStrip.Text = "TrafodionToolStrip1";
            // 
            // _theQueriesCombo
            // 
            this._theQueriesCombo.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this._theQueriesCombo.Name = "_theQueriesCombo";
            this._theQueriesCombo.Size = new System.Drawing.Size(400, 25);
            this._theQueriesCombo.SelectedIndexChanged += new System.EventHandler(this._theQueriesCombo_SelectedIndexChanged);
            // 
            // _theBreadCrumbPanel
            // 
            this._theBreadCrumbPanel.Controls.Add(this.TrafodionLabel1);
            this._theBreadCrumbPanel.Controls.Add(this.TrafodionLabel2);
            this._theBreadCrumbPanel.Controls.Add(this.TrafodionLabel3);
            this._theBreadCrumbPanel.Location = new System.Drawing.Point(0, 25);
            this._theBreadCrumbPanel.Name = "_theBreadCrumbPanel";
            this._theBreadCrumbPanel.Size = new System.Drawing.Size(682, 37);
            this._theBreadCrumbPanel.TabIndex = 1;
            this._theBreadCrumbPanel.Visible = false;
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 10F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Underline))));
            this.TrafodionLabel1.ForeColor = System.Drawing.Color.RoyalBlue;
            this.TrafodionLabel1.Location = new System.Drawing.Point(3, 0);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(69, 17);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "Report 1";
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 10F);
            this.TrafodionLabel2.ForeColor = System.Drawing.SystemColors.ControlText;
            this.TrafodionLabel2.Location = new System.Drawing.Point(78, 0);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(18, 17);
            this.TrafodionLabel2.TabIndex = 1;
            this.TrafodionLabel2.Text = ">";
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 10F, ((System.Drawing.FontStyle)((System.Drawing.FontStyle.Bold | System.Drawing.FontStyle.Underline))));
            this.TrafodionLabel3.ForeColor = System.Drawing.Color.RoyalBlue;
            this.TrafodionLabel3.Location = new System.Drawing.Point(102, 0);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(69, 17);
            this.TrafodionLabel3.TabIndex = 2;
            this.TrafodionLabel3.Text = "Report 2";
            // 
            // MetricMinerNavigator
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theBreadCrumbPanel);
            this.Controls.Add(this._theToolStrip);
            this.Name = "MetricMinerNavigator";
            this.Size = new System.Drawing.Size(744, 25);
            this._theToolStrip.ResumeLayout(false);
            this._theToolStrip.PerformLayout();
            this._theBreadCrumbPanel.ResumeLayout(false);
            this._theBreadCrumbPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionToolStrip _theToolStrip;
        private System.Windows.Forms.ToolStripComboBox _theQueriesCombo;
        private System.Windows.Forms.FlowLayoutPanel _theBreadCrumbPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel3;
    }
}
