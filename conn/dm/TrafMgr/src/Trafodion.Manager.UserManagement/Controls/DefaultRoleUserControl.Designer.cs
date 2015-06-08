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
﻿namespace Trafodion.Manager.UserManagement.Controls
{
    partial class DefaultRoleUserControl
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
            this._thePrimaryRoleCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theUserNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._dbUserNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._progressPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theContentPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theBottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theChangeRoleButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theUserInfoGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theContentPanel.SuspendLayout();
            this._theBottomPanel.SuspendLayout();
            this._theUserInfoGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _thePrimaryRoleCombo
            // 
            this._thePrimaryRoleCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._thePrimaryRoleCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._thePrimaryRoleCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._thePrimaryRoleCombo.FormattingEnabled = true;
            this._thePrimaryRoleCombo.Location = new System.Drawing.Point(127, 57);
            this._thePrimaryRoleCombo.Name = "_thePrimaryRoleCombo";
            this._thePrimaryRoleCombo.Size = new System.Drawing.Size(390, 21);
            this._thePrimaryRoleCombo.TabIndex = 4;
            this._thePrimaryRoleCombo.SelectedIndexChanged += new System.EventHandler(this._thePrimaryRoleCombo_SelectedIndexChanged);
            // 
            // _theUserNameTextBox
            // 
            this._theUserNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theUserNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserNameTextBox.Location = new System.Drawing.Point(127, 18);
            this._theUserNameTextBox.Name = "_theUserNameTextBox";
            this._theUserNameTextBox.ReadOnly = true;
            this._theUserNameTextBox.Size = new System.Drawing.Size(388, 21);
            this._theUserNameTextBox.TabIndex = 3;
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(14, 61);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(97, 13);
            this.TrafodionLabel2.TabIndex = 1;
            this.TrafodionLabel2.Text = "Primary Role Name";
            this.TrafodionLabel2.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _dbUserNameLabel
            // 
            this._dbUserNameLabel.AutoSize = true;
            this._dbUserNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._dbUserNameLabel.Location = new System.Drawing.Point(14, 22);
            this._dbUserNameLabel.Name = "_dbUserNameLabel";
            this._dbUserNameLabel.Size = new System.Drawing.Size(108, 13);
            this._dbUserNameLabel.TabIndex = 0;
            this._dbUserNameLabel.Text = "Database User Name";
            this._dbUserNameLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _progressPanel
            // 
            this._progressPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._progressPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._progressPanel.Location = new System.Drawing.Point(0, 156);
            this._progressPanel.Name = "_progressPanel";
            this._progressPanel.Size = new System.Drawing.Size(620, 216);
            this._progressPanel.TabIndex = 5;
            // 
            // _theContentPanel
            // 
            this._theContentPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theContentPanel.Controls.Add(this._thePrimaryRoleCombo);
            this._theContentPanel.Controls.Add(this._theUserNameTextBox);
            this._theContentPanel.Controls.Add(this.TrafodionLabel2);
            this._theContentPanel.Controls.Add(this._dbUserNameLabel);
            this._theContentPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theContentPanel.Location = new System.Drawing.Point(3, 17);
            this._theContentPanel.Name = "_theContentPanel";
            this._theContentPanel.Size = new System.Drawing.Size(614, 103);
            this._theContentPanel.TabIndex = 1;
            // 
            // _theBottomPanel
            // 
            this._theBottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theBottomPanel.Controls.Add(this._theChangeRoleButton);
            this._theBottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theBottomPanel.Location = new System.Drawing.Point(3, 120);
            this._theBottomPanel.Name = "_theBottomPanel";
            this._theBottomPanel.Size = new System.Drawing.Size(614, 33);
            this._theBottomPanel.TabIndex = 0;
            // 
            // _theChangeRoleButton
            // 
            this._theChangeRoleButton.AutoSize = true;
            this._theChangeRoleButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theChangeRoleButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theChangeRoleButton.Location = new System.Drawing.Point(9, 6);
            this._theChangeRoleButton.Name = "_theChangeRoleButton";
            this._theChangeRoleButton.Size = new System.Drawing.Size(121, 23);
            this._theChangeRoleButton.TabIndex = 0;
            this._theChangeRoleButton.Text = "&Change Primary Role";
            this._theChangeRoleButton.UseVisualStyleBackColor = true;
            this._theChangeRoleButton.Click += new System.EventHandler(this._theChangeRoleButton_Click);
            // 
            // _theUserInfoGroupBox
            // 
            this._theUserInfoGroupBox.Controls.Add(this._theContentPanel);
            this._theUserInfoGroupBox.Controls.Add(this._theBottomPanel);
            this._theUserInfoGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._theUserInfoGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserInfoGroupBox.Location = new System.Drawing.Point(0, 0);
            this._theUserInfoGroupBox.Name = "_theUserInfoGroupBox";
            this._theUserInfoGroupBox.Size = new System.Drawing.Size(620, 156);
            this._theUserInfoGroupBox.TabIndex = 4;
            this._theUserInfoGroupBox.TabStop = false;
            this._theUserInfoGroupBox.Text = "User Connection Settings";
            // 
            // DefaultRoleUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._progressPanel);
            this.Controls.Add(this._theUserInfoGroupBox);
            this.Name = "DefaultRoleUserControl";
            this.Size = new System.Drawing.Size(620, 378);
            this.Load += new System.EventHandler(this.DefaultRoleUserControl_Load);
            this._theContentPanel.ResumeLayout(false);
            this._theContentPanel.PerformLayout();
            this._theBottomPanel.ResumeLayout(false);
            this._theBottomPanel.PerformLayout();
            this._theUserInfoGroupBox.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionComboBox _thePrimaryRoleCombo;
        private Framework.Controls.TrafodionTextBox _theUserNameTextBox;
        private Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Framework.Controls.TrafodionLabel _dbUserNameLabel;
        private Framework.Controls.TrafodionPanel _progressPanel;
        private Framework.Controls.TrafodionPanel _theContentPanel;
        private Framework.Controls.TrafodionPanel _theBottomPanel;
        private Framework.Controls.TrafodionButton _theChangeRoleButton;
        private Framework.Controls.TrafodionGroupBox _theUserInfoGroupBox;

    }
}
