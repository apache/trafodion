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
    partial class TrafodionProgressUserControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._progressBar = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this._progressTextLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionPanel1.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // _progressBar
            // 
            this._progressBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._progressBar.Location = new System.Drawing.Point(8, 6);
            this._progressBar.Name = "_progressBar";
            this._progressBar.Size = new System.Drawing.Size(167, 18);
            this._progressBar.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this._progressBar.TabIndex = 2;
            // 
            // _progressTextLabel
            // 
            this._progressTextLabel.AutoSize = true;
            this._progressTextLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._progressTextLabel.Location = new System.Drawing.Point(5, 16);
            this._progressTextLabel.Name = "_progressTextLabel";
            this._progressTextLabel.Size = new System.Drawing.Size(123, 13);
            this._progressTextLabel.TabIndex = 3;
            this._progressTextLabel.Text = "Operation in progress...";
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._progressTextLabel);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(193, 57);
            this.TrafodionPanel1.TabIndex = 4;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this._progressBar);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel2.Location = new System.Drawing.Point(0, 57);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(193, 38);
            this.TrafodionPanel2.TabIndex = 5;
            // 
            // TrafodionProgressUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.TrafodionPanel2);
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "TrafodionProgressUserControl";
            this.Size = new System.Drawing.Size(193, 119);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.TrafodionPanel2.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionProgressBar _progressBar;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _progressTextLabel;
        private TrafodionPanel TrafodionPanel1;
        private TrafodionPanel TrafodionPanel2;
    }
}
