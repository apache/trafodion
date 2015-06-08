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
    partial class AlertsInitializingUserControl
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
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.progressBar1 = new System.Windows.Forms.ProgressBar();
            this.lblDataProviderInfo = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.SuspendLayout();
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(85, 0);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(140, 13);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "Initializing Alerts control ....";
            this.TrafodionLabel1.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // progressBar1
            // 
            this.progressBar1.Location = new System.Drawing.Point(3, 17);
            this.progressBar1.Name = "progressBar1";
            this.progressBar1.Size = new System.Drawing.Size(298, 16);
            this.progressBar1.Style = System.Windows.Forms.ProgressBarStyle.Marquee;
            this.progressBar1.TabIndex = 1;
            // 
            // lblDataProviderInfo
            // 
            this.lblDataProviderInfo.AutoSize = true;
            this.lblDataProviderInfo.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.lblDataProviderInfo.Location = new System.Drawing.Point(8, 13);
            this.lblDataProviderInfo.Name = "lblDataProviderInfo";
            this.lblDataProviderInfo.Size = new System.Drawing.Size(0, 13);
            this.lblDataProviderInfo.TabIndex = 2;
            this.lblDataProviderInfo.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.lblDataProviderInfo.Visible = false;
            // 
            // AlertsInitializingUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.lblDataProviderInfo);
            this.Controls.Add(this.progressBar1);
            this.Controls.Add(this.TrafodionLabel1);
            this.Name = "AlertsInitializingUserControl";
            this.Size = new System.Drawing.Size(304, 40);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
        private System.Windows.Forms.ProgressBar progressBar1;
        private Framework.Controls.TrafodionLabel lblDataProviderInfo;
    }
}
