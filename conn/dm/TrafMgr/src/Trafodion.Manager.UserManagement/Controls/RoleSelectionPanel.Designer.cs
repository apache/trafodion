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
    partial class RoleSelectionPanel
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
            this.TrafodionGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theAdditionalRolesPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDelAdditionalRoleBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._userListPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalRoleBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theRolesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theTypedRoleName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionGroupBox2.SuspendLayout();
            this._theAdditionalRolesPanel.SuspendLayout();
            this._theMainPanel.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionGroupBox2
            // 
            this.TrafodionGroupBox2.Controls.Add(this._theAdditionalRolesPanel);
            this.TrafodionGroupBox2.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox2.Location = new System.Drawing.Point(0, 298);
            this.TrafodionGroupBox2.Name = "TrafodionGroupBox2";
            this.TrafodionGroupBox2.Size = new System.Drawing.Size(523, 147);
            this.TrafodionGroupBox2.TabIndex = 3;
            this.TrafodionGroupBox2.TabStop = false;
            this.TrafodionGroupBox2.Text = "Selected Roles";
            // 
            // _theAdditionalRolesPanel
            // 
            this._theAdditionalRolesPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalRolesPanel.Controls.Add(this._theDelAdditionalRoleBtn);
            this._theAdditionalRolesPanel.Controls.Add(this._userListPanel);
            this._theAdditionalRolesPanel.Controls.Add(this._theAdditionalRoleBtn);
            this._theAdditionalRolesPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalRolesPanel.Location = new System.Drawing.Point(3, 17);
            this._theAdditionalRolesPanel.Name = "_theAdditionalRolesPanel";
            this._theAdditionalRolesPanel.Size = new System.Drawing.Size(517, 127);
            this._theAdditionalRolesPanel.TabIndex = 5;
            // 
            // _theDelAdditionalRoleBtn
            // 
            this._theDelAdditionalRoleBtn.Enabled = false;
            this._theDelAdditionalRoleBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDelAdditionalRoleBtn.Location = new System.Drawing.Point(3, 32);
            this._theDelAdditionalRoleBtn.Name = "_theDelAdditionalRoleBtn";
            this._theDelAdditionalRoleBtn.Size = new System.Drawing.Size(113, 23);
            this._theDelAdditionalRoleBtn.TabIndex = 5;
            this._theDelAdditionalRoleBtn.Text = "<- Remove";
            this._theDelAdditionalRoleBtn.UseVisualStyleBackColor = true;
            this._theDelAdditionalRoleBtn.Click += new System.EventHandler(this._theDelAdditionalRoleBtn_Click);
            // 
            // _userListPanel
            // 
            this._userListPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._userListPanel.AutoSize = true;
            this._userListPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._userListPanel.Location = new System.Drawing.Point(124, 0);
            this._userListPanel.Name = "_userListPanel";
            this._userListPanel.Size = new System.Drawing.Size(393, 127);
            this._userListPanel.TabIndex = 4;
            // 
            // _theAdditionalRoleBtn
            // 
            this._theAdditionalRoleBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAdditionalRoleBtn.Location = new System.Drawing.Point(3, 3);
            this._theAdditionalRoleBtn.Name = "_theAdditionalRoleBtn";
            this._theAdditionalRoleBtn.Size = new System.Drawing.Size(113, 23);
            this._theAdditionalRoleBtn.TabIndex = 2;
            this._theAdditionalRoleBtn.Text = "Add ->";
            this._theAdditionalRoleBtn.UseVisualStyleBackColor = true;
            this._theAdditionalRoleBtn.Click += new System.EventHandler(this._theAdditionalRoleBtn_Click);
            // 
            // _theRolesGroupBox
            // 
            this._theRolesGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theRolesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRolesGroupBox.Location = new System.Drawing.Point(0, 64);
            this._theRolesGroupBox.Name = "_theRolesGroupBox";
            this._theRolesGroupBox.Size = new System.Drawing.Size(523, 234);
            this._theRolesGroupBox.TabIndex = 2;
            this._theRolesGroupBox.TabStop = false;
            this._theRolesGroupBox.Text = "Roles";
            // 
            // _theTypedRoleName
            // 
            this._theTypedRoleName.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTypedRoleName.Location = new System.Drawing.Point(6, 30);
            this._theTypedRoleName.Name = "_theTypedRoleName";
            this._theTypedRoleName.Size = new System.Drawing.Size(331, 21);
            this._theTypedRoleName.TabIndex = 1;
            this._theTypedRoleName.TextChanged += new System.EventHandler(this._theTypedRoleName_TextChanged);
            // 
            // _theMainPanel
            // 
            this._theMainPanel.AutoSize = true;
            this._theMainPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._theRolesGroupBox);
            this._theMainPanel.Controls.Add(this.TrafodionPanel1);
            this._theMainPanel.Controls.Add(this.TrafodionGroupBox2);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(523, 445);
            this._theMainPanel.TabIndex = 1;
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(3, 11);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(174, 13);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "Type Role Name or Select from List";
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this.TrafodionLabel1);
            this.TrafodionPanel1.Controls.Add(this._theTypedRoleName);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(523, 64);
            this.TrafodionPanel1.TabIndex = 4;
            // 
            // RoleSelectionPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.Controls.Add(this._theMainPanel);
            this.Name = "RoleSelectionPanel";
            this.Size = new System.Drawing.Size(523, 445);
            this.TrafodionGroupBox2.ResumeLayout(false);
            this._theAdditionalRolesPanel.ResumeLayout(false);
            this._theAdditionalRolesPanel.PerformLayout();
            this._theMainPanel.ResumeLayout(false);
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox2;
        private Framework.Controls.TrafodionPanel _theAdditionalRolesPanel;
        private Framework.Controls.TrafodionButton _theAdditionalRoleBtn;
        private Framework.Controls.TrafodionGroupBox _theRolesGroupBox;
        private Framework.Controls.TrafodionTextBox _theTypedRoleName;
        private Framework.Controls.TrafodionPanel _theMainPanel;
        private Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Framework.Controls.TrafodionButton _theDelAdditionalRoleBtn;
        private Framework.Controls.TrafodionPanel _userListPanel;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;

    }
}
