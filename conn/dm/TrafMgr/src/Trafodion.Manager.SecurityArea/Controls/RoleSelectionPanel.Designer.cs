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
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theAdditionalRolesPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalRoleBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAdditionalRolesTxt = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theDefaultRolePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theDefaultRoleBtn = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theDefaultRoleText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theRolesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theTypedRoleName = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theMainPanel.SuspendLayout();
            this.oneGuiGroupBox2.SuspendLayout();
            this._theAdditionalRolesPanel.SuspendLayout();
            this._theDefaultRolePanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.AutoSize = true;
            this._theMainPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this.oneGuiGroupBox2);
            this._theMainPanel.Controls.Add(this._theRolesGroupBox);
            this._theMainPanel.Controls.Add(this._theTypedRoleName);
            this._theMainPanel.Controls.Add(this.oneGuiLabel1);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(519, 443);
            this._theMainPanel.TabIndex = 0;
            // 
            // oneGuiGroupBox2
            // 
            this.oneGuiGroupBox2.Controls.Add(this._theAdditionalRolesPanel);
            this.oneGuiGroupBox2.Controls.Add(this._theDefaultRolePanel);
            this.oneGuiGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox2.Location = new System.Drawing.Point(9, 312);
            this.oneGuiGroupBox2.Name = "oneGuiGroupBox2";
            this.oneGuiGroupBox2.Size = new System.Drawing.Size(507, 128);
            this.oneGuiGroupBox2.TabIndex = 3;
            this.oneGuiGroupBox2.TabStop = false;
            this.oneGuiGroupBox2.Text = "Selected Roles";
            // 
            // _theAdditionalRolesPanel
            // 
            this._theAdditionalRolesPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalRolesPanel.Controls.Add(this._theAdditionalRoleBtn);
            this._theAdditionalRolesPanel.Controls.Add(this._theAdditionalRolesTxt);
            this._theAdditionalRolesPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalRolesPanel.Location = new System.Drawing.Point(3, 43);
            this._theAdditionalRolesPanel.Name = "_theAdditionalRolesPanel";
            this._theAdditionalRolesPanel.Size = new System.Drawing.Size(501, 82);
            this._theAdditionalRolesPanel.TabIndex = 5;
            // 
            // _theAdditionalRoleBtn
            // 
            this._theAdditionalRoleBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAdditionalRoleBtn.Location = new System.Drawing.Point(3, 3);
            this._theAdditionalRoleBtn.Name = "_theAdditionalRoleBtn";
            this._theAdditionalRoleBtn.Size = new System.Drawing.Size(113, 23);
            this._theAdditionalRoleBtn.TabIndex = 2;
            this._theAdditionalRoleBtn.Text = "Additional Roles ->";
            this._theAdditionalRoleBtn.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theAdditionalRoleBtn.UseVisualStyleBackColor = true;
            this._theAdditionalRoleBtn.Click += new System.EventHandler(this._theAdditionalRoleBtn_Click);
            // 
            // _theAdditionalRolesTxt
            // 
            this._theAdditionalRolesTxt.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAdditionalRolesTxt.Location = new System.Drawing.Point(125, 5);
            this._theAdditionalRolesTxt.Multiline = true;
            this._theAdditionalRolesTxt.Name = "_theAdditionalRolesTxt";
            this._theAdditionalRolesTxt.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this._theAdditionalRolesTxt.Size = new System.Drawing.Size(367, 71);
            this._theAdditionalRolesTxt.TabIndex = 3;
            this._theAdditionalRolesTxt.Leave += new System.EventHandler(this._theAdditionalRolesTxt_Leave);
            // 
            // _theDefaultRolePanel
            // 
            this._theDefaultRolePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theDefaultRolePanel.Controls.Add(this._theDefaultRoleBtn);
            this._theDefaultRolePanel.Controls.Add(this._theDefaultRoleText);
            this._theDefaultRolePanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theDefaultRolePanel.Location = new System.Drawing.Point(3, 17);
            this._theDefaultRolePanel.Name = "_theDefaultRolePanel";
            this._theDefaultRolePanel.Size = new System.Drawing.Size(501, 26);
            this._theDefaultRolePanel.TabIndex = 4;
            // 
            // _theDefaultRoleBtn
            // 
            this._theDefaultRoleBtn.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDefaultRoleBtn.Location = new System.Drawing.Point(0, 0);
            this._theDefaultRoleBtn.Name = "_theDefaultRoleBtn";
            this._theDefaultRoleBtn.Size = new System.Drawing.Size(116, 23);
            this._theDefaultRoleBtn.TabIndex = 0;
            this._theDefaultRoleBtn.Text = "Default Role ->";
            this._theDefaultRoleBtn.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theDefaultRoleBtn.UseVisualStyleBackColor = true;
            this._theDefaultRoleBtn.Click += new System.EventHandler(this._theDefaultRoleBtn_Click);
            // 
            // _theDefaultRoleText
            // 
            this._theDefaultRoleText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDefaultRoleText.Location = new System.Drawing.Point(125, 2);
            this._theDefaultRoleText.Name = "_theDefaultRoleText";
            this._theDefaultRoleText.Size = new System.Drawing.Size(367, 21);
            this._theDefaultRoleText.TabIndex = 1;
            this._theDefaultRoleText.Leave += new System.EventHandler(this._theDefaultRoleText_Leave);
            // 
            // _theRolesGroupBox
            // 
            this._theRolesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRolesGroupBox.Location = new System.Drawing.Point(9, 55);
            this._theRolesGroupBox.Name = "_theRolesGroupBox";
            this._theRolesGroupBox.Size = new System.Drawing.Size(507, 251);
            this._theRolesGroupBox.TabIndex = 2;
            this._theRolesGroupBox.TabStop = false;
            this._theRolesGroupBox.Text = "Roles";
            // 
            // _theTypedRoleName
            // 
            this._theTypedRoleName.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTypedRoleName.Location = new System.Drawing.Point(9, 28);
            this._theTypedRoleName.Name = "_theTypedRoleName";
            this._theTypedRoleName.Size = new System.Drawing.Size(171, 21);
            this._theTypedRoleName.TabIndex = 1;
            this._theTypedRoleName.TextChanged += new System.EventHandler(this._theTypedRoleName_TextChanged);
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(6, 12);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(174, 13);
            this.oneGuiLabel1.TabIndex = 0;
            this.oneGuiLabel1.Text = "Type Role Name or Select from List";
            // 
            // RoleSelectionPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this._theMainPanel);
            this.Name = "RoleSelectionPanel";
            this.Size = new System.Drawing.Size(519, 443);
            this._theMainPanel.ResumeLayout(false);
            this._theMainPanel.PerformLayout();
            this.oneGuiGroupBox2.ResumeLayout(false);
            this._theAdditionalRolesPanel.ResumeLayout(false);
            this._theAdditionalRolesPanel.PerformLayout();
            this._theDefaultRolePanel.ResumeLayout(false);
            this._theDefaultRolePanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theTypedRoleName;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _theRolesGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theDefaultRoleBtn;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theAdditionalRolesTxt;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theAdditionalRoleBtn;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theDefaultRoleText;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theDefaultRolePanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theAdditionalRolesPanel;
    }
}
