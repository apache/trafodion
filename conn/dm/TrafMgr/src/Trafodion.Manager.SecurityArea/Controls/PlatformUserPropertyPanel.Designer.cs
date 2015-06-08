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
﻿namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class PlatformUserPropertyPanel
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
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._thePasswordExpirationPolicy = new Trafodion.Manager.SecurityArea.Controls.PasswordExpirationPolicyControl();
            this._theAdditionalParametersPanel = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theDefaultSecurity = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theDefaultSecurityLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theInitialDirectory = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theInitialDirectoryLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theDefaultSubvolume = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theDefaultSubVolumeLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this.oneGuiGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theConfirmPasswordLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theRetypePasswordText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._thePasswordText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._thePasswordLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theMainPanel.SuspendLayout();
            this._theAdditionalParametersPanel.SuspendLayout();
            this.oneGuiGroupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.AutoSize = true;
            this._theMainPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._thePasswordExpirationPolicy);
            this._theMainPanel.Controls.Add(this._theAdditionalParametersPanel);
            this._theMainPanel.Controls.Add(this.oneGuiGroupBox2);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(482, 341);
            this._theMainPanel.TabIndex = 0;

            // 
            // _thePasswordExpirationPolicy
            // 
            this._thePasswordExpirationPolicy.ExpiryDate = "";
            this._thePasswordExpirationPolicy.ExpiryDays = 0;
            this._thePasswordExpirationPolicy.Location = new System.Drawing.Point(4, 90);
            this._thePasswordExpirationPolicy.Name = "_thePasswordExpirationPolicy";
            this._thePasswordExpirationPolicy.NoExpiry = true;
            this._thePasswordExpirationPolicy.Size = new System.Drawing.Size(475, 129);
            this._thePasswordExpirationPolicy.TabIndex = 2;
            this._thePasswordExpirationPolicy.TabStop = false;
            // 
            // _theAdditionalParametersPanel
            // 
            this._theAdditionalParametersPanel.Controls.Add(this._theDefaultSecurity);
            this._theAdditionalParametersPanel.Controls.Add(this._theDefaultSecurityLabel);
            this._theAdditionalParametersPanel.Controls.Add(this._theInitialDirectory);
            this._theAdditionalParametersPanel.Controls.Add(this._theInitialDirectoryLabel);
            this._theAdditionalParametersPanel.Controls.Add(this._theDefaultSubvolume);
            this._theAdditionalParametersPanel.Controls.Add(this._theDefaultSubVolumeLabel);
            this._theAdditionalParametersPanel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAdditionalParametersPanel.Location = new System.Drawing.Point(3, 225);
            this._theAdditionalParametersPanel.Name = "_theAdditionalParametersPanel";
            this._theAdditionalParametersPanel.Size = new System.Drawing.Size(476, 113);
            this._theAdditionalParametersPanel.TabIndex = 3;
            this._theAdditionalParametersPanel.TabStop = false;
            this._theAdditionalParametersPanel.Text = "Additional Parameters";
            // 
            // _theDefaultSecurity
            // 
            this._theDefaultSecurity.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDefaultSecurity.Location = new System.Drawing.Point(115, 74);
            this._theDefaultSecurity.Name = "_theDefaultSecurity";
            this._theDefaultSecurity.Size = new System.Drawing.Size(335, 21);
            this._theDefaultSecurity.TabIndex = 3;
            // 
            // _theDefaultSecurityLabel
            // 
            this._theDefaultSecurityLabel.AutoSize = true;
            this._theDefaultSecurityLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDefaultSecurityLabel.Location = new System.Drawing.Point(21, 77);
            this._theDefaultSecurityLabel.Name = "_theDefaultSecurityLabel";
            this._theDefaultSecurityLabel.ShowRequired = true;
            this._theDefaultSecurityLabel.Size = new System.Drawing.Size(94, 18);
            this._theDefaultSecurityLabel.TabIndex = 6;
            this._theDefaultSecurityLabel.TabStop = false;
            // 
            // _theInitialDirectory
            // 
            this._theInitialDirectory.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theInitialDirectory.Location = new System.Drawing.Point(115, 47);
            this._theInitialDirectory.Name = "_theInitialDirectory";
            this._theInitialDirectory.Size = new System.Drawing.Size(335, 21);
            this._theInitialDirectory.TabIndex = 2;
            // 
            // _theInitialDirectoryLabel
            // 
            this._theInitialDirectoryLabel.AutoSize = true;
            this._theInitialDirectoryLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theInitialDirectoryLabel.Location = new System.Drawing.Point(6, 50);
            this._theInitialDirectoryLabel.Name = "_theInitialDirectoryLabel";
            this._theInitialDirectoryLabel.ShowRequired = true;
            this._theInitialDirectoryLabel.Size = new System.Drawing.Size(109, 18);
            this._theInitialDirectoryLabel.TabIndex = 4;
            this._theInitialDirectoryLabel.TabStop = false;
            // 
            // _theDefaultSubvolume
            // 
            this._theDefaultSubvolume.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDefaultSubvolume.Location = new System.Drawing.Point(115, 20);
            this._theDefaultSubvolume.Name = "_theDefaultSubvolume";
            this._theDefaultSubvolume.Size = new System.Drawing.Size(335, 21);
            this._theDefaultSubvolume.TabIndex = 1;
            // 
            // _theDefaultSubVolumeLabel
            // 
            this._theDefaultSubVolumeLabel.AutoSize = true;
            this._theDefaultSubVolumeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDefaultSubVolumeLabel.Location = new System.Drawing.Point(6, 23);
            this._theDefaultSubVolumeLabel.Name = "_theDefaultSubVolumeLabel";
            this._theDefaultSubVolumeLabel.ShowRequired = true;
            this._theDefaultSubVolumeLabel.Size = new System.Drawing.Size(109, 18);
            this._theDefaultSubVolumeLabel.TabIndex = 2;
            this._theDefaultSubVolumeLabel.TabStop = false;
            // 
            // oneGuiGroupBox2
            // 
            this.oneGuiGroupBox2.Controls.Add(this._theConfirmPasswordLabel);
            this.oneGuiGroupBox2.Controls.Add(this._theRetypePasswordText);
            this.oneGuiGroupBox2.Controls.Add(this._thePasswordText);
            this.oneGuiGroupBox2.Controls.Add(this._thePasswordLabel);
            this.oneGuiGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox2.Location = new System.Drawing.Point(3, 4);
            this.oneGuiGroupBox2.Name = "oneGuiGroupBox2";
            this.oneGuiGroupBox2.Size = new System.Drawing.Size(476, 80);
            this.oneGuiGroupBox2.TabIndex = 1;
            this.oneGuiGroupBox2.TabStop = false;
            this.oneGuiGroupBox2.Text = "Password";
            // 
            // _theConfirmPasswordLabel
            // 
            this._theConfirmPasswordLabel.AutoSize = true;
            this._theConfirmPasswordLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theConfirmPasswordLabel.Location = new System.Drawing.Point(1, 47);
            this._theConfirmPasswordLabel.Name = "_theConfirmPasswordLabel";
            this._theConfirmPasswordLabel.ShowRequired = true;
            this._theConfirmPasswordLabel.Size = new System.Drawing.Size(114, 18);
            this._theConfirmPasswordLabel.TabIndex = 3;
            this._theConfirmPasswordLabel.TabStop = false;
            // 
            // _theRetypePasswordText
            // 
            this._theRetypePasswordText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRetypePasswordText.Location = new System.Drawing.Point(115, 44);
            this._theRetypePasswordText.Name = "_theRetypePasswordText";
            this._theRetypePasswordText.Size = new System.Drawing.Size(335, 21);
            this._theRetypePasswordText.TabIndex = 2;
            this._theRetypePasswordText.UseSystemPasswordChar = true;
            // 
            // _thePasswordText
            // 
            this._thePasswordText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._thePasswordText.Location = new System.Drawing.Point(115, 17);
            this._thePasswordText.Name = "_thePasswordText";
            this._thePasswordText.PasswordChar = '*';
            this._thePasswordText.Size = new System.Drawing.Size(335, 21);
            this._thePasswordText.TabIndex = 1;
            this._thePasswordText.UseSystemPasswordChar = true;
            // 
            // _thePasswordLabel
            // 
            this._thePasswordLabel.AutoSize = true;
            this._thePasswordLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._thePasswordLabel.Location = new System.Drawing.Point(45, 20);
            this._thePasswordLabel.Name = "_thePasswordLabel";
            this._thePasswordLabel.ShowRequired = true;
            this._thePasswordLabel.Size = new System.Drawing.Size(70, 18);
            this._thePasswordLabel.TabIndex = 0;
            this._thePasswordLabel.TabStop = false;
            // 
            // PlatformUserPropertyPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this._theMainPanel);
            this.Name = "PlatformUserPropertyPanel";
            this.Size = new System.Drawing.Size(482, 341);
            this._theMainPanel.ResumeLayout(false);
            this._theAdditionalParametersPanel.ResumeLayout(false);
            this._theAdditionalParametersPanel.PerformLayout();
            this.oneGuiGroupBox2.ResumeLayout(false);
            this.oneGuiGroupBox2.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theAdditionalParametersPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theConfirmPasswordLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theRetypePasswordText;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _thePasswordText;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _thePasswordLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theDefaultSubvolume;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theDefaultSubVolumeLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theDefaultSecurity;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theDefaultSecurityLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theInitialDirectory;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theInitialDirectoryLabel;
        private PasswordExpirationPolicyControl _thePasswordExpirationPolicy;

    }
}
