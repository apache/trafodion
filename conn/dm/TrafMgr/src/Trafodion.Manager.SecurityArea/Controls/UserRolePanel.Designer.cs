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
    partial class UserRolePanel
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
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theAdditionalRolePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalRoles = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this._theAdditionalRolesButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theDefaultRolePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDefaultRoleRequiredLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theDefaultRoleButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theDefaultRole = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theMainPanel.SuspendLayout();
            this.oneGuiGroupBox1.SuspendLayout();
            this._theAdditionalRolePanel.SuspendLayout();
            this._theDefaultRolePanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this.oneGuiGroupBox1);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(469, 158);
            this._theMainPanel.TabIndex = 0;
            // 
            // oneGuiGroupBox1
            // 
            this.oneGuiGroupBox1.Controls.Add(this._theAdditionalRolePanel);
            this.oneGuiGroupBox1.Controls.Add(this._theDefaultRolePanel);
            this.oneGuiGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiGroupBox1.Name = "oneGuiGroupBox1";
            this.oneGuiGroupBox1.Size = new System.Drawing.Size(469, 158);
            this.oneGuiGroupBox1.TabIndex = 1;
            this.oneGuiGroupBox1.TabStop = false;
            this.oneGuiGroupBox1.Text = "Role";
            // 
            // _theAdditionalRolePanel
            // 
            this._theAdditionalRolePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalRolePanel.Controls.Add(this._theAdditionalRoles);
            this._theAdditionalRolePanel.Controls.Add(this._theAdditionalRolesButton);
            this._theAdditionalRolePanel.Location = new System.Drawing.Point(3, 53);
            this._theAdditionalRolePanel.Name = "_theAdditionalRolePanel";
            this._theAdditionalRolePanel.Size = new System.Drawing.Size(463, 102);
            this._theAdditionalRolePanel.TabIndex = 3;
            // 
            // _theAdditionalRoles
            // 
            this._theAdditionalRoles.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAdditionalRoles.Location = new System.Drawing.Point(121, 3);
            this._theAdditionalRoles.Name = "_theAdditionalRoles";
            this._theAdditionalRoles.Size = new System.Drawing.Size(335, 96);
            this._theAdditionalRoles.TabIndex = 2;
            this._theAdditionalRoles.Text = global::Trafodion.Manager.SecurityArea.Properties.Resources.DeployCACertSuccess;
            this._theAdditionalRoles.Leave += new System.EventHandler(this._theAdditionalRoles_Leave);
            // 
            // _theAdditionalRolesButton
            // 
            this._theAdditionalRolesButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAdditionalRolesButton.Location = new System.Drawing.Point(1, 3);
            this._theAdditionalRolesButton.Name = "_theAdditionalRolesButton";
            this._theAdditionalRolesButton.Size = new System.Drawing.Size(109, 23);
            this._theAdditionalRolesButton.TabIndex = 1;
            this._theAdditionalRolesButton.Text = "Additional Roles...";
            this._theAdditionalRolesButton.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theAdditionalRolesButton.UseVisualStyleBackColor = true;
            this._theAdditionalRolesButton.Click += new System.EventHandler(this._theAdditionalRolesButton_Click);
            // 
            // _theDefaultRolePanel
            // 
            this._theDefaultRolePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theDefaultRolePanel.Controls.Add(this._theDefaultRole);
            this._theDefaultRolePanel.Controls.Add(this._theDefaultRoleRequiredLabel);
            this._theDefaultRolePanel.Controls.Add(this._theDefaultRoleButton);
            this._theDefaultRolePanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theDefaultRolePanel.Location = new System.Drawing.Point(3, 17);
            this._theDefaultRolePanel.Name = "_theDefaultRolePanel";
            this._theDefaultRolePanel.Size = new System.Drawing.Size(463, 36);
            this._theDefaultRolePanel.TabIndex = 2;
            // 
            // _theDefaultRoleRequiredLabel
            // 
            this._theDefaultRoleRequiredLabel.Location = new System.Drawing.Point(110, 14);
            this._theDefaultRoleRequiredLabel.Name = "_theDefaultRoleRequiredLabel";
            this._theDefaultRoleRequiredLabel.ShowRequired = true;
            this._theDefaultRoleRequiredLabel.Size = new System.Drawing.Size(10, 18);
            this._theDefaultRoleRequiredLabel.TabIndex = 8;
            // 
            // _theDefaultRoleButton
            // 
            this._theDefaultRoleButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDefaultRoleButton.Location = new System.Drawing.Point(1, 9);
            this._theDefaultRoleButton.Name = "_theDefaultRoleButton";
            this._theDefaultRoleButton.Size = new System.Drawing.Size(109, 23);
            this._theDefaultRoleButton.TabIndex = 1;
            this._theDefaultRoleButton.Text = "Default Role...";
            this._theDefaultRoleButton.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theDefaultRoleButton.UseVisualStyleBackColor = true;
            this._theDefaultRoleButton.Click += new System.EventHandler(this._theDefaultRoleButton_Click);
            // 
            // _theDefaultRole
            // 
            this._theDefaultRole.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theDefaultRole.FormattingEnabled = true;
            this._theDefaultRole.Location = new System.Drawing.Point(121, 9);
            this._theDefaultRole.Name = "_theDefaultRole";
            this._theDefaultRole.Size = new System.Drawing.Size(335, 21);
            this._theDefaultRole.TabIndex = 9;
            // 
            // UserRolePanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theMainPanel);
            this.Name = "UserRolePanel";
            this.Size = new System.Drawing.Size(469, 158);
            this._theMainPanel.ResumeLayout(false);
            this.oneGuiGroupBox1.ResumeLayout(false);
            this._theAdditionalRolePanel.ResumeLayout(false);
            this._theDefaultRolePanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theDefaultRolePanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theDefaultRoleButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theAdditionalRolePanel;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox _theAdditionalRoles;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theAdditionalRolesButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theDefaultRoleRequiredLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _theDefaultRole;
    }
}
