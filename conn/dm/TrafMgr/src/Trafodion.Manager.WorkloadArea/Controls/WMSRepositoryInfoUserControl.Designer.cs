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
    partial class WMSRepositoryInfoUserControl
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
            MyDispose(disposing);
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
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theQueryIdTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.label1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._thePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this._theQueryIdTextBox);
            this.panel1.Controls.Add(this.label1);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(769, 54);
            this.panel1.TabIndex = 2;
            // 
            // _theQueryIdTextBox
            // 
            this._theQueryIdTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theQueryIdTextBox.Location = new System.Drawing.Point(62, 17);
            this._theQueryIdTextBox.Name = "_theQueryIdTextBox";
            this._theQueryIdTextBox.ReadOnly = true;
            this._theQueryIdTextBox.Size = new System.Drawing.Size(557, 21);
            this._theQueryIdTextBox.TabIndex = 1;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.label1.Location = new System.Drawing.Point(4, 21);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(55, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Query ID:";
            // 
            // _thePanel
            // 
            this._thePanel.AutoScroll = true;
            this._thePanel.AutoSize = true;
            this._thePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._thePanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this._thePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thePanel.Location = new System.Drawing.Point(0, 54);
            this._thePanel.Name = "_thePanel";
            this._thePanel.Size = new System.Drawing.Size(769, 282);
            this._thePanel.TabIndex = 3;
            // 
            // WMSRepositoryInfoUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._thePanel);
            this.Controls.Add(this.panel1);
            this.Name = "WMSRepositoryInfoUserControl";
            this.Size = new System.Drawing.Size(769, 336);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theQueryIdTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel label1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _thePanel;
    }
}
