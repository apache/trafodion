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
    partial class PCFBrowserDialog
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._bannerBannerControl = new Trafodion.Manager.Framework.Controls.TrafodionBannerControl();
            this.SuspendLayout();
            // 
            // _bannerBannerControl
            // 
            this._bannerBannerControl.ConnectionDefinition = null;
            this._bannerBannerControl.Dock = System.Windows.Forms.DockStyle.Top;
            this._bannerBannerControl.Location = new System.Drawing.Point(0, 0);
            this._bannerBannerControl.Name = "_bannerBannerControl";
            this._bannerBannerControl.ShowDescription = true;
            this._bannerBannerControl.Size = new System.Drawing.Size(910, 51);
            this._bannerBannerControl.TabIndex = 0;
            // 
            // PCFBrowserDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ClientSize = new System.Drawing.Size(910, 585);
            this.Controls.Add(this._bannerBannerControl);
            this.Name = "PCFBrowserDialog";
            this.Text = "HP Database Manager - Procedure Code File Browser";
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionBannerControl _bannerBannerControl;

    }
}