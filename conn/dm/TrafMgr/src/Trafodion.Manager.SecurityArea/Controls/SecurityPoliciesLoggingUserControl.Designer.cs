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
    partial class SecurityPoliciesLoggingUserControl
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
            this.oneGuiGroupBox4 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._logFileAgesInDaysSpinner = new Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown();
            this.oneGuiLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._logDatabaseLogonFailure = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._logPlatformLogonFailure = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._logDatabaseLogonOKRequired = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._logDatabaseLogonOK = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._logPlatformLogonOK = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._logChangePasswordRequired = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._logChangePassword = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._logUserManagementRequired = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._logUserManagement = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.oneGuiPanel1.SuspendLayout();
            this.oneGuiGroupBox4.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this._logFileAgesInDaysSpinner)).BeginInit();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this.oneGuiGroupBox4);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(674, 350);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // oneGuiGroupBox4
            // 
            this.oneGuiGroupBox4.Controls.Add(this._logFileAgesInDaysSpinner);
            this.oneGuiGroupBox4.Controls.Add(this.oneGuiLabel2);
            this.oneGuiGroupBox4.Controls.Add(this._logDatabaseLogonFailure);
            this.oneGuiGroupBox4.Controls.Add(this._logPlatformLogonFailure);
            this.oneGuiGroupBox4.Controls.Add(this._logDatabaseLogonOKRequired);
            this.oneGuiGroupBox4.Controls.Add(this._logDatabaseLogonOK);
            this.oneGuiGroupBox4.Controls.Add(this._logPlatformLogonOK);
            this.oneGuiGroupBox4.Controls.Add(this._logChangePasswordRequired);
            this.oneGuiGroupBox4.Controls.Add(this._logChangePassword);
            this.oneGuiGroupBox4.Controls.Add(this._logUserManagementRequired);
            this.oneGuiGroupBox4.Controls.Add(this._logUserManagement);
            this.oneGuiGroupBox4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox4.Location = new System.Drawing.Point(0, 0);
            this.oneGuiGroupBox4.Name = "oneGuiGroupBox4";
            this.oneGuiGroupBox4.Size = new System.Drawing.Size(605, 238);
            this.oneGuiGroupBox4.TabIndex = 2;
            this.oneGuiGroupBox4.TabStop = false;
            this.oneGuiGroupBox4.Text = "Security Logging Policy";
            // 
            // _logFileAgesInDaysSpinner
            // 
            this._logFileAgesInDaysSpinner.Location = new System.Drawing.Point(156, 193);
            this._logFileAgesInDaysSpinner.Maximum = new decimal(new int[] {
            1827,
            0,
            0,
            0});
            this._logFileAgesInDaysSpinner.Minimum = new decimal(new int[] {
            7,
            0,
            0,
            0});
            this._logFileAgesInDaysSpinner.Name = "_logFileAgesInDaysSpinner";
            this._logFileAgesInDaysSpinner.Size = new System.Drawing.Size(72, 21);
            this._logFileAgesInDaysSpinner.TabIndex = 11;
            this._logFileAgesInDaysSpinner.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this._logFileAgesInDaysSpinner.Value = new decimal(new int[] {
            365,
            0,
            0,
            0});
            // 
            // oneGuiLabel2
            // 
            this.oneGuiLabel2.AutoSize = true;
            this.oneGuiLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel2.Location = new System.Drawing.Point(34, 197);
            this.oneGuiLabel2.Name = "oneGuiLabel2";
            this.oneGuiLabel2.Size = new System.Drawing.Size(115, 13);
            this.oneGuiLabel2.TabIndex = 0;
            this.oneGuiLabel2.Text = "Log File Ages in (days)";
            // 
            // _logDatabaseLogonFailure
            // 
            this._logDatabaseLogonFailure.AutoSize = true;
            this._logDatabaseLogonFailure.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._logDatabaseLogonFailure.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logDatabaseLogonFailure.Location = new System.Drawing.Point(37, 139);
            this._logDatabaseLogonFailure.Name = "_logDatabaseLogonFailure";
            this._logDatabaseLogonFailure.Size = new System.Drawing.Size(162, 18);
            this._logDatabaseLogonFailure.TabIndex = 9;
            this._logDatabaseLogonFailure.Text = "Log Failed Database Logins";
            this._logDatabaseLogonFailure.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._logDatabaseLogonFailure.UseVisualStyleBackColor = true;
            // 
            // _logPlatformLogonFailure
            // 
            this._logPlatformLogonFailure.AutoSize = true;
            this._logPlatformLogonFailure.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._logPlatformLogonFailure.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logPlatformLogonFailure.Location = new System.Drawing.Point(37, 165);
            this._logPlatformLogonFailure.Name = "_logPlatformLogonFailure";
            this._logPlatformLogonFailure.Size = new System.Drawing.Size(156, 18);
            this._logPlatformLogonFailure.TabIndex = 10;
            this._logPlatformLogonFailure.Text = "Log Failed Platform Logins";
            this._logPlatformLogonFailure.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._logPlatformLogonFailure.UseVisualStyleBackColor = true;
            // 
            // _logDatabaseLogonOKRequired
            // 
            this._logDatabaseLogonOKRequired.AutoSize = true;
            this._logDatabaseLogonOKRequired.Enabled = false;
            this._logDatabaseLogonOKRequired.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._logDatabaseLogonOKRequired.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logDatabaseLogonOKRequired.Location = new System.Drawing.Point(287, 87);
            this._logDatabaseLogonOKRequired.Name = "_logDatabaseLogonOKRequired";
            this._logDatabaseLogonOKRequired.Size = new System.Drawing.Size(209, 18);
            this._logDatabaseLogonOKRequired.TabIndex = 6;
            this._logDatabaseLogonOKRequired.Text = "Abort Database Login if Logging Fails";
            this._logDatabaseLogonOKRequired.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._logDatabaseLogonOKRequired.UseVisualStyleBackColor = true;
            // 
            // _logDatabaseLogonOK
            // 
            this._logDatabaseLogonOK.AutoSize = true;
            this._logDatabaseLogonOK.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._logDatabaseLogonOK.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logDatabaseLogonOK.Location = new System.Drawing.Point(37, 87);
            this._logDatabaseLogonOK.Name = "_logDatabaseLogonOK";
            this._logDatabaseLogonOK.Size = new System.Drawing.Size(184, 18);
            this._logDatabaseLogonOK.TabIndex = 5;
            this._logDatabaseLogonOK.Text = "Log Successful Database Logins";
            this._logDatabaseLogonOK.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._logDatabaseLogonOK.UseVisualStyleBackColor = true;
            this._logDatabaseLogonOK.CheckedChanged += new System.EventHandler(this._logDatabaseLogonOK_CheckChanged);
            // 
            // _logPlatformLogonOK
            // 
            this._logPlatformLogonOK.AutoSize = true;
            this._logPlatformLogonOK.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._logPlatformLogonOK.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logPlatformLogonOK.Location = new System.Drawing.Point(37, 113);
            this._logPlatformLogonOK.Name = "_logPlatformLogonOK";
            this._logPlatformLogonOK.Size = new System.Drawing.Size(178, 18);
            this._logPlatformLogonOK.TabIndex = 7;
            this._logPlatformLogonOK.Text = "Log Successful Platform Logins";
            this._logPlatformLogonOK.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._logPlatformLogonOK.UseVisualStyleBackColor = true;
            // 
            // _logChangePasswordRequired
            // 
            this._logChangePasswordRequired.AutoSize = true;
            this._logChangePasswordRequired.Enabled = false;
            this._logChangePasswordRequired.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._logChangePasswordRequired.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logChangePasswordRequired.Location = new System.Drawing.Point(287, 61);
            this._logChangePasswordRequired.Name = "_logChangePasswordRequired";
            this._logChangePasswordRequired.Size = new System.Drawing.Size(221, 18);
            this._logChangePasswordRequired.TabIndex = 4;
            this._logChangePasswordRequired.Text = "Abort Change Password if Logging Fails";
            this._logChangePasswordRequired.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._logChangePasswordRequired.UseVisualStyleBackColor = true;
            // 
            // _logChangePassword
            // 
            this._logChangePassword.AutoSize = true;
            this._logChangePassword.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._logChangePassword.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logChangePassword.Location = new System.Drawing.Point(37, 61);
            this._logChangePassword.Name = "_logChangePassword";
            this._logChangePassword.Size = new System.Drawing.Size(143, 18);
            this._logChangePassword.TabIndex = 3;
            this._logChangePassword.Text = "Log Change Passwords";
            this._logChangePassword.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._logChangePassword.UseVisualStyleBackColor = true;
            this._logChangePassword.CheckedChanged += new System.EventHandler(this._logChangePassword_CheckChanged);
            // 
            // _logUserManagementRequired
            // 
            this._logUserManagementRequired.AutoSize = true;
            this._logUserManagementRequired.Enabled = false;
            this._logUserManagementRequired.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._logUserManagementRequired.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logUserManagementRequired.Location = new System.Drawing.Point(287, 35);
            this._logUserManagementRequired.Name = "_logUserManagementRequired";
            this._logUserManagementRequired.Size = new System.Drawing.Size(273, 18);
            this._logUserManagementRequired.TabIndex = 2;
            this._logUserManagementRequired.Text = "Abort User Management Operation if Logging Fails";
            this._logUserManagementRequired.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._logUserManagementRequired.UseVisualStyleBackColor = true;
            // 
            // _logUserManagement
            // 
            this._logUserManagement.AutoSize = true;
            this._logUserManagement.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._logUserManagement.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logUserManagement.Location = new System.Drawing.Point(37, 35);
            this._logUserManagement.Name = "_logUserManagement";
            this._logUserManagement.Size = new System.Drawing.Size(195, 18);
            this._logUserManagement.TabIndex = 1;
            this._logUserManagement.Text = "Log User Management Operations";
            this._logUserManagement.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._logUserManagement.UseVisualStyleBackColor = true;
            this._logUserManagement.CheckedChanged += new System.EventHandler(this._logUserManagement_CheckChanged);
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // SecurityPoliciesLoggingUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "SecurityPoliciesLoggingUserControl";
            this.Size = new System.Drawing.Size(674, 350);
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiGroupBox4.ResumeLayout(false);
            this.oneGuiGroupBox4.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this._logFileAgesInDaysSpinner)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox4;
        private Trafodion.Manager.Framework.Controls.TrafodionNumericUpDown _logFileAgesInDaysSpinner;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _logDatabaseLogonFailure;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _logPlatformLogonFailure;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _logDatabaseLogonOKRequired;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _logDatabaseLogonOK;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _logPlatformLogonOK;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _logChangePasswordRequired;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _logChangePassword;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _logUserManagementRequired;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _logUserManagement;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _toolTip;
    }
}
