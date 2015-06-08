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
    partial class ChangeRolePasswordDialog
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
            this._confirmPasswordLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._confirmPasswordTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._newPasswordTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._newPasswordLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._roleNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._roleNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.mainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._passwordGuiGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._oldPasswordLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._oldPasswordTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._passwordExpirationPolicyControl = new Trafodion.Manager.SecurityArea.Controls.PasswordExpirationPolicyControl();
            this._buttonsPanel.SuspendLayout();
            this.mainPanel.SuspendLayout();
            this._passwordGuiGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _confirmPasswordLabel
            // 
            this._confirmPasswordLabel.AutoSize = true;
            this._confirmPasswordLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._confirmPasswordLabel.Location = new System.Drawing.Point(5, 92);
            this._confirmPasswordLabel.Name = "_confirmPasswordLabel";
            this._confirmPasswordLabel.Size = new System.Drawing.Size(117, 13);
            this._confirmPasswordLabel.TabIndex = 4;
            this._confirmPasswordLabel.Text = "Confirm New Password";
            // 
            // _confirmPasswordTextBox
            // 
            this._confirmPasswordTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._confirmPasswordTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._confirmPasswordTextBox.Location = new System.Drawing.Point(128, 86);
            this._confirmPasswordTextBox.Name = "_confirmPasswordTextBox";
            this._confirmPasswordTextBox.PasswordChar = '*';
            this._confirmPasswordTextBox.Size = new System.Drawing.Size(493, 21);
            this._confirmPasswordTextBox.TabIndex = 5;
            this._confirmPasswordTextBox.UseSystemPasswordChar = true;
            this._confirmPasswordTextBox.TextChanged += new System.EventHandler(this._confirmPasswordTextBox_TextChanged);
            // 
            // _newPasswordTextBox
            // 
            this._newPasswordTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._newPasswordTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._newPasswordTextBox.Location = new System.Drawing.Point(128, 53);
            this._newPasswordTextBox.Name = "_newPasswordTextBox";
            this._newPasswordTextBox.PasswordChar = '*';
            this._newPasswordTextBox.Size = new System.Drawing.Size(493, 21);
            this._newPasswordTextBox.TabIndex = 3;
            this._newPasswordTextBox.UseSystemPasswordChar = true;
            this._newPasswordTextBox.TextChanged += new System.EventHandler(this._newPasswordTextBox_TextChanged);
            // 
            // _newPasswordLabel
            // 
            this._newPasswordLabel.AutoSize = true;
            this._newPasswordLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._newPasswordLabel.Location = new System.Drawing.Point(45, 58);
            this._newPasswordLabel.Name = "_newPasswordLabel";
            this._newPasswordLabel.Size = new System.Drawing.Size(77, 13);
            this._newPasswordLabel.TabIndex = 2;
            this._newPasswordLabel.Text = "New Password";
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._cancelButton);
            this._buttonsPanel.Controls.Add(this._okButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 316);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(654, 34);
            this._buttonsPanel.TabIndex = 1;
            // 
            // _cancelButton
            // 
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(343, 6);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 1;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            // 
            // _okButton
            // 
            this._okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._okButton.Location = new System.Drawing.Point(236, 6);
            this._okButton.Name = "_okButton";
            this._okButton.Size = new System.Drawing.Size(75, 23);
            this._okButton.TabIndex = 0;
            this._okButton.Text = "&OK";
            this._okButton.UseVisualStyleBackColor = true;
            this._okButton.Click += new System.EventHandler(this._okButton_Click);
            // 
            // _roleNameLabel
            // 
            this._roleNameLabel.AutoSize = true;
            this._roleNameLabel.Enabled = false;
            this._roleNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameLabel.Location = new System.Drawing.Point(76, 15);
            this._roleNameLabel.Name = "_roleNameLabel";
            this._roleNameLabel.Size = new System.Drawing.Size(58, 13);
            this._roleNameLabel.TabIndex = 0;
            this._roleNameLabel.Text = "Role Name";
            // 
            // _roleNameTextBox
            // 
            this._roleNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._roleNameTextBox.Enabled = false;
            this._roleNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameTextBox.Location = new System.Drawing.Point(140, 12);
            this._roleNameTextBox.Name = "_roleNameTextBox";
            this._roleNameTextBox.ReadOnly = true;
            this._roleNameTextBox.Size = new System.Drawing.Size(493, 21);
            this._roleNameTextBox.TabIndex = 1;
            // 
            // mainPanel
            // 
            this.mainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.mainPanel.Controls.Add(this._passwordGuiGroupBox);
            this.mainPanel.Controls.Add(this._passwordExpirationPolicyControl);
            this.mainPanel.Controls.Add(this._roleNameLabel);
            this.mainPanel.Controls.Add(this._roleNameTextBox);
            this.mainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.mainPanel.Location = new System.Drawing.Point(0, 0);
            this.mainPanel.Name = "mainPanel";
            this.mainPanel.Size = new System.Drawing.Size(654, 316);
            this.mainPanel.TabIndex = 0;
            // 
            // _passwordGuiGroupBox
            // 
            this._passwordGuiGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._passwordGuiGroupBox.Controls.Add(this._oldPasswordLabel);
            this._passwordGuiGroupBox.Controls.Add(this._newPasswordLabel);
            this._passwordGuiGroupBox.Controls.Add(this._confirmPasswordTextBox);
            this._passwordGuiGroupBox.Controls.Add(this._confirmPasswordLabel);
            this._passwordGuiGroupBox.Controls.Add(this._oldPasswordTextBox);
            this._passwordGuiGroupBox.Controls.Add(this._newPasswordTextBox);
            this._passwordGuiGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._passwordGuiGroupBox.Location = new System.Drawing.Point(12, 39);
            this._passwordGuiGroupBox.Name = "_passwordGuiGroupBox";
            this._passwordGuiGroupBox.Size = new System.Drawing.Size(630, 122);
            this._passwordGuiGroupBox.TabIndex = 2;
            this._passwordGuiGroupBox.TabStop = false;
            this._passwordGuiGroupBox.Text = "Password";
            // 
            // _oldPasswordLabel
            // 
            this._oldPasswordLabel.AutoSize = true;
            this._oldPasswordLabel.Enabled = false;
            this._oldPasswordLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._oldPasswordLabel.Location = new System.Drawing.Point(50, 23);
            this._oldPasswordLabel.Name = "_oldPasswordLabel";
            this._oldPasswordLabel.Size = new System.Drawing.Size(72, 13);
            this._oldPasswordLabel.TabIndex = 0;
            this._oldPasswordLabel.Text = "Old Password";
            // 
            // _oldPasswordTextBox
            // 
            this._oldPasswordTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._oldPasswordTextBox.Enabled = false;
            this._oldPasswordTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._oldPasswordTextBox.Location = new System.Drawing.Point(128, 20);
            this._oldPasswordTextBox.Name = "_oldPasswordTextBox";
            this._oldPasswordTextBox.PasswordChar = '*';
            this._oldPasswordTextBox.Size = new System.Drawing.Size(493, 21);
            this._oldPasswordTextBox.TabIndex = 1;
            this._oldPasswordTextBox.UseSystemPasswordChar = true;
            this._oldPasswordTextBox.TextChanged += new System.EventHandler(this._newPasswordTextBox_TextChanged);
            // 
            // _passwordExpirationPolicyControl
            // 
            this._passwordExpirationPolicyControl.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._passwordExpirationPolicyControl.ExpiryDate = "Mar 10 2010";
            this._passwordExpirationPolicyControl.ExpiryDays = 0;
            this._passwordExpirationPolicyControl.Location = new System.Drawing.Point(12, 167);
            this._passwordExpirationPolicyControl.Name = "_passwordExpirationPolicyControl";
            this._passwordExpirationPolicyControl.NoExpiry = true;
            this._passwordExpirationPolicyControl.Size = new System.Drawing.Size(630, 135);
            this._passwordExpirationPolicyControl.TabIndex = 3;
            // 
            // ChangeRolePasswordDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(654, 350);
            this.Controls.Add(this.mainPanel);
            this.Controls.Add(this._buttonsPanel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "ChangeRolePasswordDialog";
            this.Text = "HP Database Manager - Change Role Password";
            this._buttonsPanel.ResumeLayout(false);
            this.mainPanel.ResumeLayout(false);
            this.mainPanel.PerformLayout();
            this._passwordGuiGroupBox.ResumeLayout(false);
            this._passwordGuiGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _confirmPasswordLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _confirmPasswordTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _newPasswordTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _newPasswordLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _buttonsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _roleNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _roleNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel mainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _passwordGuiGroupBox;
        private PasswordExpirationPolicyControl _passwordExpirationPolicyControl;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _oldPasswordLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _oldPasswordTextBox;
    }
}
