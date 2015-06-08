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
    partial class TrafodionRequiredFieldLabel
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
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theRequiredLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theMainPanel.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this.TrafodionPanel1);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(198, 18);
            this._theMainPanel.TabIndex = 0;
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.flowLayoutPanel1);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(198, 18);
            this.TrafodionPanel1.TabIndex = 0;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.TrafodionPanel2);
            this.flowLayoutPanel1.Controls.Add(this._theLabel);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel1.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(198, 18);
            this.flowLayoutPanel1.TabIndex = 1;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this._theRequiredLabel);
            this.TrafodionPanel2.Location = new System.Drawing.Point(188, 0);
            this.TrafodionPanel2.Margin = new System.Windows.Forms.Padding(0);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(10, 18);
            this.TrafodionPanel2.TabIndex = 6;
            // 
            // _theRequiredLabel
            // 
            this._theRequiredLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theRequiredLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRequiredLabel.ForeColor = System.Drawing.Color.Red;
            this._theRequiredLabel.Location = new System.Drawing.Point(1, 4);
            this._theRequiredLabel.Name = "_theRequiredLabel";
            this._theRequiredLabel.Size = new System.Drawing.Size(8, 13);
            this._theRequiredLabel.TabIndex = 5;
            this._theRequiredLabel.Text = "*";
            this._theRequiredLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // _theLabel
            // 
            this._theLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this._theLabel.AutoSize = true;
            this._theLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLabel.Location = new System.Drawing.Point(188, 2);
            this._theLabel.Margin = new System.Windows.Forms.Padding(0);
            this._theLabel.Name = "_theLabel";
            this._theLabel.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this._theLabel.Size = new System.Drawing.Size(0, 13);
            this._theLabel.TabIndex = 4;
            this._theLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // TrafodionRequiredFieldLabel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theMainPanel);
            this.Name = "TrafodionRequiredFieldLabel";
            this.Size = new System.Drawing.Size(198, 18);
            this._theMainPanel.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.TrafodionPanel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theRequiredLabel;
    }
}
