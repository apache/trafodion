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
    partial class WMSSummaryUserControl
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WMSSummaryUserControl));
            this.flowLayoutPanel = new System.Windows.Forms.FlowLayoutPanel();
            this._workloadCountersUserControl = new Trafodion.Manager.WorkloadArea.Controls.WorkloadCountersUserControl();
            this._servicesCountersUserControl = new Trafodion.Manager.WorkloadArea.Controls.ServicesCountersUserControl();
            this._platformCountersUserControl = new Trafodion.Manager.WorkloadArea.Controls.PlatformCountersUserControl();
            this.flowLayoutPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // flowLayoutPanel
            // 
            this.flowLayoutPanel.AutoScroll = true;
            this.flowLayoutPanel.Controls.Add(this._workloadCountersUserControl);
            this.flowLayoutPanel.Controls.Add(this._servicesCountersUserControl);
            this.flowLayoutPanel.Controls.Add(this._platformCountersUserControl);
            this.flowLayoutPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.flowLayoutPanel.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel.Name = "flowLayoutPanel";
            this.flowLayoutPanel.Size = new System.Drawing.Size(741, 206);
            this.flowLayoutPanel.TabIndex = 0;
            // 
            // _workloadCountersUserControl
            // 
            this._workloadCountersUserControl.Location = new System.Drawing.Point(3, 3);
            this._workloadCountersUserControl.Name = "_workloadCountersUserControl";
            this._workloadCountersUserControl.Size = new System.Drawing.Size(230, 200);
            this._workloadCountersUserControl.TabIndex = 0;
            // 
            // _servicesCountersUserControl
            // 
            this._servicesCountersUserControl.Location = new System.Drawing.Point(239, 3);
            this._servicesCountersUserControl.Name = "_servicesCountersUserControl";
            this._servicesCountersUserControl.Size = new System.Drawing.Size(230, 200);
            this._servicesCountersUserControl.TabIndex = 1;
            // 
            // _platformCountersUserControl
            // 
            this._platformCountersUserControl.Location = new System.Drawing.Point(475, 3);
            this._platformCountersUserControl.Name = "_platformCountersUserControl";
            this._platformCountersUserControl.Size = new System.Drawing.Size(230, 200);
            this._platformCountersUserControl.TabIndex = 2;
            // 
            // WMSSummaryUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this.flowLayoutPanel);
            this.Name = "WMSSummaryUserControl";
            this.Size = new System.Drawing.Size(741, 206);
            this.flowLayoutPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel;
        private WorkloadCountersUserControl _workloadCountersUserControl;
        private ServicesCountersUserControl _servicesCountersUserControl;
        private PlatformCountersUserControl _platformCountersUserControl;


    }
}
