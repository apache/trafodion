// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//
namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class SecurityPoliciesSystemUserControl
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
            this.components = new System.ComponentModel.Container();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._optForLocalAccessCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._driverCompatibilityCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._enforceCertificateExpiryCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._autoDownloadCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.oneGuiPanel1.SuspendLayout();
            this.oneGuiGroupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this.oneGuiGroupBox1);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(674, 350);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // oneGuiGroupBox1
            // 
            this.oneGuiGroupBox1.Controls.Add(this._optForLocalAccessCheckBox);
            this.oneGuiGroupBox1.Controls.Add(this._driverCompatibilityCheckBox);
            this.oneGuiGroupBox1.Controls.Add(this._enforceCertificateExpiryCheckBox);
            this.oneGuiGroupBox1.Controls.Add(this._autoDownloadCheckBox);
            this.oneGuiGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiGroupBox1.Name = "oneGuiGroupBox1";
            this.oneGuiGroupBox1.Size = new System.Drawing.Size(610, 164);
            this.oneGuiGroupBox1.TabIndex = 1;
            this.oneGuiGroupBox1.TabStop = false;
            this.oneGuiGroupBox1.Text = "Certificate && Connection Policy";
            // 
            // _optForLocalAccessCheckBox
            // 
            this._optForLocalAccessCheckBox.AutoSize = true;
            this._optForLocalAccessCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._optForLocalAccessCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._optForLocalAccessCheckBox.Location = new System.Drawing.Point(37, 125);
            this._optForLocalAccessCheckBox.Name = "_optForLocalAccessCheckBox";
            this._optForLocalAccessCheckBox.Size = new System.Drawing.Size(159, 18);
            this._optForLocalAccessCheckBox.TabIndex = 5;
            this._optForLocalAccessCheckBox.Text = "Optimized for Local Access";
            this._optForLocalAccessCheckBox.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._optForLocalAccessCheckBox.UseVisualStyleBackColor = true;
            // 
            // _driverCompatibilityCheckBox
            // 
            this._driverCompatibilityCheckBox.AutoSize = true;
            this._driverCompatibilityCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._driverCompatibilityCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._driverCompatibilityCheckBox.Location = new System.Drawing.Point(37, 95);
            this._driverCompatibilityCheckBox.Name = "_driverCompatibilityCheckBox";
            this._driverCompatibilityCheckBox.Size = new System.Drawing.Size(203, 18);
            this._driverCompatibilityCheckBox.TabIndex = 4;
            this._driverCompatibilityCheckBox.Text = "Allow Down-rev  Drivers to Connect";
            this._driverCompatibilityCheckBox.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._driverCompatibilityCheckBox.UseVisualStyleBackColor = true;
            // 
            // _enforceCertificateExpiryCheckBox
            // 
            this._enforceCertificateExpiryCheckBox.AutoSize = true;
            this._enforceCertificateExpiryCheckBox.Checked = true;
            this._enforceCertificateExpiryCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
            this._enforceCertificateExpiryCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._enforceCertificateExpiryCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._enforceCertificateExpiryCheckBox.Location = new System.Drawing.Point(37, 65);
            this._enforceCertificateExpiryCheckBox.Name = "_enforceCertificateExpiryCheckBox";
            this._enforceCertificateExpiryCheckBox.Size = new System.Drawing.Size(155, 18);
            this._enforceCertificateExpiryCheckBox.TabIndex = 2;
            this._enforceCertificateExpiryCheckBox.Text = "Enforce Certificate Expiry";
            this._enforceCertificateExpiryCheckBox.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._enforceCertificateExpiryCheckBox.UseVisualStyleBackColor = true;
            // 
            // _autoDownloadCheckBox
            // 
            this._autoDownloadCheckBox.AutoSize = true;
            this._autoDownloadCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._autoDownloadCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._autoDownloadCheckBox.Location = new System.Drawing.Point(37, 35);
            this._autoDownloadCheckBox.Name = "_autoDownloadCheckBox";
            this._autoDownloadCheckBox.Size = new System.Drawing.Size(158, 18);
            this._autoDownloadCheckBox.TabIndex = 1;
            this._autoDownloadCheckBox.Text = "Auto Download Certificate";
            this._autoDownloadCheckBox.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._autoDownloadCheckBox.UseVisualStyleBackColor = true;
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // SecurityPoliciesSystemUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "SecurityPoliciesSystemUserControl";
            this.Size = new System.Drawing.Size(674, 350);
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiGroupBox1.ResumeLayout(false);
            this.oneGuiGroupBox1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _enforceCertificateExpiryCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _autoDownloadCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _driverCompatibilityCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _optForLocalAccessCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _toolTip;
    }
}
