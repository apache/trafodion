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
    partial class SecurityOverviewUserControl
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
            //MyDispose();
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._theBottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theChangePasswordButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theChangeRoleButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theContentPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDefaultRoleCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theUserNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.oneGuiLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theUserInfoGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._progressPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theBottomPanel.SuspendLayout();
            this._theContentPanel.SuspendLayout();
            this._theUserInfoGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theBottomPanel
            // 
            this._theBottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theBottomPanel.Controls.Add(this._theChangePasswordButton);
            this._theBottomPanel.Controls.Add(this._theChangeRoleButton);
            this._theBottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theBottomPanel.Location = new System.Drawing.Point(3, 120);
            this._theBottomPanel.Name = "_theBottomPanel";
            this._theBottomPanel.Size = new System.Drawing.Size(616, 33);
            this._theBottomPanel.TabIndex = 0;
            // 
            // _theChangePasswordButton
            // 
            this._theChangePasswordButton.AutoSize = true;
            this._theChangePasswordButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theChangePasswordButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theChangePasswordButton.Location = new System.Drawing.Point(144, 6);
            this._theChangePasswordButton.Name = "_theChangePasswordButton";
            this._theChangePasswordButton.Size = new System.Drawing.Size(120, 23);
            this._theChangePasswordButton.TabIndex = 1;
            this._theChangePasswordButton.Text = "Change &Password";
            this._theChangePasswordButton.UseVisualStyleBackColor = true;
            this._theChangePasswordButton.Click += new System.EventHandler(this._theChangePasswordButton_Click);
            // 
            // _theChangeRoleButton
            // 
            this._theChangeRoleButton.AutoSize = true;
            this._theChangeRoleButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theChangeRoleButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theChangeRoleButton.Location = new System.Drawing.Point(9, 6);
            this._theChangeRoleButton.Name = "_theChangeRoleButton";
            this._theChangeRoleButton.Size = new System.Drawing.Size(120, 23);
            this._theChangeRoleButton.TabIndex = 0;
            this._theChangeRoleButton.Text = "Change &Default Role";
            this._theChangeRoleButton.UseVisualStyleBackColor = true;
            this._theChangeRoleButton.Click += new System.EventHandler(this._theChangeRoleButton_Click);
            // 
            // _theContentPanel
            // 
            this._theContentPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theContentPanel.Controls.Add(this._theDefaultRoleCombo);
            this._theContentPanel.Controls.Add(this._theUserNameTextBox);
            this._theContentPanel.Controls.Add(this.oneGuiLabel2);
            this._theContentPanel.Controls.Add(this.oneGuiLabel1);
            this._theContentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theContentPanel.Location = new System.Drawing.Point(3, 17);
            this._theContentPanel.Name = "_theContentPanel";
            this._theContentPanel.Size = new System.Drawing.Size(616, 103);
            this._theContentPanel.TabIndex = 1;
            // 
            // _theDefaultRoleCombo
            // 
            this._theDefaultRoleCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theDefaultRoleCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theDefaultRoleCombo.FormattingEnabled = true;
            this._theDefaultRoleCombo.Location = new System.Drawing.Point(101, 57);
            this._theDefaultRoleCombo.Name = "_theDefaultRoleCombo";
            this._theDefaultRoleCombo.Size = new System.Drawing.Size(416, 21);
            this._theDefaultRoleCombo.TabIndex = 4;
            // 
            // _theUserNameTextBox
            // 
            this._theUserNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theUserNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserNameTextBox.Location = new System.Drawing.Point(101, 18);
            this._theUserNameTextBox.Name = "_theUserNameTextBox";
            this._theUserNameTextBox.ReadOnly = true;
            this._theUserNameTextBox.Size = new System.Drawing.Size(416, 21);
            this._theUserNameTextBox.TabIndex = 3;
            // 
            // oneGuiLabel2
            // 
            this.oneGuiLabel2.AutoSize = true;
            this.oneGuiLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel2.Location = new System.Drawing.Point(6, 61);
            this.oneGuiLabel2.Name = "oneGuiLabel2";
            this.oneGuiLabel2.Size = new System.Drawing.Size(66, 13);
            this.oneGuiLabel2.TabIndex = 1;
            this.oneGuiLabel2.Text = "Default Role";
            this.oneGuiLabel2.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(13, 22);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(59, 13);
            this.oneGuiLabel1.TabIndex = 0;
            this.oneGuiLabel1.Text = "User Name";
            this.oneGuiLabel1.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theUserInfoGroupBox
            // 
            this._theUserInfoGroupBox.Controls.Add(this._theContentPanel);
            this._theUserInfoGroupBox.Controls.Add(this._theBottomPanel);
            this._theUserInfoGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._theUserInfoGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserInfoGroupBox.Location = new System.Drawing.Point(0, 0);
            this._theUserInfoGroupBox.Name = "_theUserInfoGroupBox";
            this._theUserInfoGroupBox.Size = new System.Drawing.Size(622, 156);
            this._theUserInfoGroupBox.TabIndex = 2;
            this._theUserInfoGroupBox.TabStop = false;
            this._theUserInfoGroupBox.Text = "Sign On User Info";
            // 
            // _progressPanel
            // 
            this._progressPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._progressPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._progressPanel.Location = new System.Drawing.Point(0, 156);
            this._progressPanel.Name = "_progressPanel";
            this._progressPanel.Size = new System.Drawing.Size(622, 216);
            this._progressPanel.TabIndex = 3;
            // 
            // SecurityOverviewUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._progressPanel);
            this.Controls.Add(this._theUserInfoGroupBox);
            this.Name = "SecurityOverviewUserControl";
            this.Size = new System.Drawing.Size(622, 378);
            this.Load += new System.EventHandler(this.SecurityOverviewUserControl_Load);
            this._theBottomPanel.ResumeLayout(false);
            this._theBottomPanel.PerformLayout();
            this._theContentPanel.ResumeLayout(false);
            this._theContentPanel.PerformLayout();
            this._theUserInfoGroupBox.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theBottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theChangePasswordButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theChangeRoleButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theContentPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theUserNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theUserInfoGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _theDefaultRoleCombo;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _progressPanel;
    }
}
