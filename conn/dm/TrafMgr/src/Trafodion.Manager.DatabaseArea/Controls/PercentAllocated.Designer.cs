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
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class PercentAllocated
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
            this._percentAllocatedProgress = new Trafodion.Manager.Framework.Controls.TrafodionProgressBar();
            this._percentAllocatedLbl = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.SuspendLayout();
            // 
            // _percentAllocatedProgress
            // 
            this._percentAllocatedProgress.Dock = System.Windows.Forms.DockStyle.Left;
            this._percentAllocatedProgress.Location = new System.Drawing.Point(0, 0);
            this._percentAllocatedProgress.Name = "_percentAllocatedProgress";
            this._percentAllocatedProgress.Size = new System.Drawing.Size(146, 18);
            this._percentAllocatedProgress.TabIndex = 0;
            // 
            // _percentAllocatedLbl
            // 
            this._percentAllocatedLbl.AutoSize = true;
            this._percentAllocatedLbl.Dock = System.Windows.Forms.DockStyle.Right;
            this._percentAllocatedLbl.Font = new System.Drawing.Font("Tahoma", 8F);
            this._percentAllocatedLbl.Location = new System.Drawing.Point(152, 0);
            this._percentAllocatedLbl.Name = "_percentAllocatedLbl";
            this._percentAllocatedLbl.Size = new System.Drawing.Size(35, 16);
            this._percentAllocatedLbl.TabIndex = 1;
            this._percentAllocatedLbl.Text = "NNN";
            this._percentAllocatedLbl.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            // 
            // PercentAllocated
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._percentAllocatedLbl);
            this.Controls.Add(this._percentAllocatedProgress);
            this.Name = "PercentAllocated";
            this.Size = new System.Drawing.Size(187, 18);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionProgressBar _percentAllocatedProgress;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _percentAllocatedLbl;

    }
}
