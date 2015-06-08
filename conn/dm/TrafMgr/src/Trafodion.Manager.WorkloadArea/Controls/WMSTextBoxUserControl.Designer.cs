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
    partial class WMSTextBoxUserControl
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
            this._theIDTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theIDTitle = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._thePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this._theIDTextBox);
            this.panel1.Controls.Add(this._theIDTitle);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(771, 54);
            this.panel1.TabIndex = 1;
            // 
            // _theIDTextBox
            // 
            this._theIDTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theIDTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theIDTextBox.Location = new System.Drawing.Point(83, 17);
            this._theIDTextBox.Name = "_theIDTextBox";
            this._theIDTextBox.ReadOnly = true;
            this._theIDTextBox.Size = new System.Drawing.Size(667, 21);
            this._theIDTextBox.TabIndex = 1;
            // 
            // _theIDTitle
            // 
            this._theIDTitle.AutoSize = true;
            this._theIDTitle.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theIDTitle.Location = new System.Drawing.Point(5, 21);
            this._theIDTitle.Name = "_theIDTitle";
            this._theIDTitle.Size = new System.Drawing.Size(74, 13);
            this._theIDTitle.TabIndex = 0;
            this._theIDTitle.Text = "Process Name";
            // 
            // _thePanel
            // 
            this._thePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._thePanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this._thePanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thePanel.Location = new System.Drawing.Point(0, 54);
            this._thePanel.Name = "_thePanel";
            this._thePanel.Size = new System.Drawing.Size(771, 295);
            this._thePanel.TabIndex = 2;
            // 
            // WMSTextBoxUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._thePanel);
            this.Controls.Add(this.panel1);
            this.Name = "WMSTextBoxUserControl";
            this.Size = new System.Drawing.Size(771, 349);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel panel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theIDTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theIDTitle;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _thePanel;

    }
}
