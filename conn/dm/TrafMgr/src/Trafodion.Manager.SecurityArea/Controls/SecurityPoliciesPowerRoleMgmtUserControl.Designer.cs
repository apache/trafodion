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
    partial class SecurityPoliciesPowerRoleMgmtUserControl
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
            this._rolePasswordGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._platformUsersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._nonePlatformUsersRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._powerPlatformUsersRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._allPlatformUsersRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._roleLevelGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._noneDatabaseUsersRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._powerDatabaseUsersRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.oneGuiLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._pwdRequiredToResetSuperCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._pwdRequiredToResetPowerRolesCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._toolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.oneGuiPanel1.SuspendLayout();
            this.oneGuiGroupBox1.SuspendLayout();
            this._rolePasswordGroupBox.SuspendLayout();
            this._platformUsersGroupBox.SuspendLayout();
            this._roleLevelGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this.oneGuiGroupBox1);
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(672, 346);
            this.oneGuiPanel1.TabIndex = 0;
            // 
            // oneGuiGroupBox1
            // 
            this.oneGuiGroupBox1.Controls.Add(this._rolePasswordGroupBox);
            this.oneGuiGroupBox1.Controls.Add(this.oneGuiLabel2);
            this.oneGuiGroupBox1.Controls.Add(this.oneGuiLabel1);
            this.oneGuiGroupBox1.Controls.Add(this._pwdRequiredToResetSuperCheckBox);
            this.oneGuiGroupBox1.Controls.Add(this._pwdRequiredToResetPowerRolesCheckBox);
            this.oneGuiGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiGroupBox1.Name = "oneGuiGroupBox1";
            this.oneGuiGroupBox1.Size = new System.Drawing.Size(599, 309);
            this.oneGuiGroupBox1.TabIndex = 1;
            this.oneGuiGroupBox1.TabStop = false;
            this.oneGuiGroupBox1.Text = "Power Role Management Policy";
            // 
            // _rolePasswordGroupBox
            // 
            this._rolePasswordGroupBox.Controls.Add(this._platformUsersGroupBox);
            this._rolePasswordGroupBox.Controls.Add(this._roleLevelGroupBox);
            this._rolePasswordGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._rolePasswordGroupBox.Location = new System.Drawing.Point(23, 126);
            this._rolePasswordGroupBox.Name = "_rolePasswordGroupBox";
            this._rolePasswordGroupBox.Size = new System.Drawing.Size(562, 154);
            this._rolePasswordGroupBox.TabIndex = 9;
            this._rolePasswordGroupBox.TabStop = false;
            this._rolePasswordGroupBox.Text = "Role Password Required at Login";
            // 
            // _platformUsersGroupBox
            // 
            this._platformUsersGroupBox.Controls.Add(this._nonePlatformUsersRadioButton);
            this._platformUsersGroupBox.Controls.Add(this._powerPlatformUsersRadioButton);
            this._platformUsersGroupBox.Controls.Add(this._allPlatformUsersRadioButton);
            this._platformUsersGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._platformUsersGroupBox.Location = new System.Drawing.Point(284, 32);
            this._platformUsersGroupBox.Name = "_platformUsersGroupBox";
            this._platformUsersGroupBox.Size = new System.Drawing.Size(275, 120);
            this._platformUsersGroupBox.TabIndex = 2;
            this._platformUsersGroupBox.TabStop = false;
            this._platformUsersGroupBox.Text = "Platform Users";
            // 
            // _nonePlatformUsersRadioButton
            // 
            this._nonePlatformUsersRadioButton.AutoSize = true;
            this._nonePlatformUsersRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._nonePlatformUsersRadioButton.Location = new System.Drawing.Point(27, 77);
            this._nonePlatformUsersRadioButton.Name = "_nonePlatformUsersRadioButton";
            this._nonePlatformUsersRadioButton.Size = new System.Drawing.Size(114, 18);
            this._nonePlatformUsersRadioButton.TabIndex = 2;
            this._nonePlatformUsersRadioButton.TabStop = true;
            this._nonePlatformUsersRadioButton.Text = "None Is Required";
            this._nonePlatformUsersRadioButton.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._nonePlatformUsersRadioButton.UseVisualStyleBackColor = true;
            // 
            // _powerPlatformUsersRadioButton
            // 
            this._powerPlatformUsersRadioButton.AutoSize = true;
            this._powerPlatformUsersRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._powerPlatformUsersRadioButton.Location = new System.Drawing.Point(27, 53);
            this._powerPlatformUsersRadioButton.Name = "_powerPlatformUsersRadioButton";
            this._powerPlatformUsersRadioButton.Size = new System.Drawing.Size(157, 18);
            this._powerPlatformUsersRadioButton.TabIndex = 1;
            this._powerPlatformUsersRadioButton.TabStop = true;
            this._powerPlatformUsersRadioButton.Text = "Power Users Are Required";
            this._powerPlatformUsersRadioButton.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._powerPlatformUsersRadioButton.UseVisualStyleBackColor = true;
            // 
            // _allPlatformUsersRadioButton
            // 
            this._allPlatformUsersRadioButton.AutoSize = true;
            this._allPlatformUsersRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._allPlatformUsersRadioButton.Location = new System.Drawing.Point(27, 29);
            this._allPlatformUsersRadioButton.Name = "_allPlatformUsersRadioButton";
            this._allPlatformUsersRadioButton.Size = new System.Drawing.Size(181, 18);
            this._allPlatformUsersRadioButton.TabIndex = 0;
            this._allPlatformUsersRadioButton.TabStop = true;
            this._allPlatformUsersRadioButton.Text = "All Platform Users Are Required";
            this._allPlatformUsersRadioButton.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._allPlatformUsersRadioButton.UseVisualStyleBackColor = true;
            // 
            // _roleLevelGroupBox
            // 
            this._roleLevelGroupBox.Controls.Add(this._noneDatabaseUsersRadioButton);
            this._roleLevelGroupBox.Controls.Add(this._powerDatabaseUsersRadioButton);
            this._roleLevelGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleLevelGroupBox.Location = new System.Drawing.Point(3, 32);
            this._roleLevelGroupBox.Name = "_roleLevelGroupBox";
            this._roleLevelGroupBox.Size = new System.Drawing.Size(275, 120);
            this._roleLevelGroupBox.TabIndex = 1;
            this._roleLevelGroupBox.TabStop = false;
            this._roleLevelGroupBox.Text = "Database Users";
            // 
            // _noneDatabaseUsersRadioButton
            // 
            this._noneDatabaseUsersRadioButton.AutoSize = true;
            this._noneDatabaseUsersRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._noneDatabaseUsersRadioButton.Location = new System.Drawing.Point(35, 53);
            this._noneDatabaseUsersRadioButton.Name = "_noneDatabaseUsersRadioButton";
            this._noneDatabaseUsersRadioButton.Size = new System.Drawing.Size(114, 18);
            this._noneDatabaseUsersRadioButton.TabIndex = 3;
            this._noneDatabaseUsersRadioButton.TabStop = true;
            this._noneDatabaseUsersRadioButton.Text = "None Is Required";
            this._noneDatabaseUsersRadioButton.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._noneDatabaseUsersRadioButton.UseVisualStyleBackColor = true;
            // 
            // _powerDatabaseUsersRadioButton
            // 
            this._powerDatabaseUsersRadioButton.AutoSize = true;
            this._powerDatabaseUsersRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._powerDatabaseUsersRadioButton.Location = new System.Drawing.Point(35, 29);
            this._powerDatabaseUsersRadioButton.Name = "_powerDatabaseUsersRadioButton";
            this._powerDatabaseUsersRadioButton.Size = new System.Drawing.Size(206, 18);
            this._powerDatabaseUsersRadioButton.TabIndex = 2;
            this._powerDatabaseUsersRadioButton.TabStop = true;
            this._powerDatabaseUsersRadioButton.Text = "Power Database Users Are Required";
            this._powerDatabaseUsersRadioButton.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._powerDatabaseUsersRadioButton.UseVisualStyleBackColor = true;
            // 
            // oneGuiLabel2
            // 
            this.oneGuiLabel2.AutoSize = true;
            this.oneGuiLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel2.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel2.Location = new System.Drawing.Point(44, 98);
            this.oneGuiLabel2.Name = "oneGuiLabel2";
            this.oneGuiLabel2.Size = new System.Drawing.Size(263, 13);
            this.oneGuiLabel2.TabIndex = 7;
            this.oneGuiLabel2.Text = "Warning: If selected, only the Super ID can deselect.";
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.ForeColor = System.Drawing.Color.Red;
            this.oneGuiLabel1.Location = new System.Drawing.Point(44, 51);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(263, 13);
            this.oneGuiLabel1.TabIndex = 6;
            this.oneGuiLabel1.Text = "Warning: If selected, only the Super ID can deselect.";
            // 
            // _pwdRequiredToResetSuperCheckBox
            // 
            this._pwdRequiredToResetSuperCheckBox.AutoSize = true;
            this._pwdRequiredToResetSuperCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._pwdRequiredToResetSuperCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pwdRequiredToResetSuperCheckBox.Location = new System.Drawing.Point(23, 78);
            this._pwdRequiredToResetSuperCheckBox.Name = "_pwdRequiredToResetSuperCheckBox";
            this._pwdRequiredToResetSuperCheckBox.Size = new System.Drawing.Size(279, 18);
            this._pwdRequiredToResetSuperCheckBox.TabIndex = 6;
            this._pwdRequiredToResetSuperCheckBox.Text = "Change Password for Super Requires Old Password";
            this._pwdRequiredToResetSuperCheckBox.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._pwdRequiredToResetSuperCheckBox.UseVisualStyleBackColor = true;
            // 
            // _pwdRequiredToResetPowerRolesCheckBox
            // 
            this._pwdRequiredToResetPowerRolesCheckBox.AutoSize = true;
            this._pwdRequiredToResetPowerRolesCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._pwdRequiredToResetPowerRolesCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pwdRequiredToResetPowerRolesCheckBox.Location = new System.Drawing.Point(23, 31);
            this._pwdRequiredToResetPowerRolesCheckBox.Name = "_pwdRequiredToResetPowerRolesCheckBox";
            this._pwdRequiredToResetPowerRolesCheckBox.Size = new System.Drawing.Size(310, 18);
            this._pwdRequiredToResetPowerRolesCheckBox.TabIndex = 5;
            this._pwdRequiredToResetPowerRolesCheckBox.Text = "Change Password for Power Roles Requires Old Password";
            this._pwdRequiredToResetPowerRolesCheckBox.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._pwdRequiredToResetPowerRolesCheckBox.UseVisualStyleBackColor = true;
            // 
            // _toolTip
            // 
            this._toolTip.IsBalloon = true;
            // 
            // SecurityPoliciesPowerRoleMgmtUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.oneGuiPanel1);
            this.Name = "SecurityPoliciesPowerRoleMgmtUserControl";
            this.Size = new System.Drawing.Size(672, 346);
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiGroupBox1.ResumeLayout(false);
            this.oneGuiGroupBox1.PerformLayout();
            this._rolePasswordGroupBox.ResumeLayout(false);
            this._platformUsersGroupBox.ResumeLayout(false);
            this._platformUsersGroupBox.PerformLayout();
            this._roleLevelGroupBox.ResumeLayout(false);
            this._roleLevelGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _pwdRequiredToResetSuperCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _pwdRequiredToResetPowerRolesCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _platformUsersGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _roleLevelGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _nonePlatformUsersRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _powerPlatformUsersRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _allPlatformUsersRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _noneDatabaseUsersRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _powerDatabaseUsersRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _rolePasswordGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip _toolTip;
    }
}
