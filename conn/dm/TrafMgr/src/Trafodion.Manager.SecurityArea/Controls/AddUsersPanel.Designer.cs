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
    partial class AddUsersPanel
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AddUsersPanel));
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theUserRolePanel = new Trafodion.Manager.SecurityArea.Controls.UserRolePanel();
            this._theUsers = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this._theUserNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theMainPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.AutoSize = true;
            this._theMainPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._theUserNameLabel);
            this._theMainPanel.Controls.Add(this._theUsers);
            this._theMainPanel.Controls.Add(this._theUserRolePanel);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(485, 276);
            this._theMainPanel.TabIndex = 0;
            // 
            // _theUserRolePanel
            // 
            this._theUserRolePanel.AdditionalRoles = ((System.Collections.Generic.List<string>)(resources.GetObject("_theUserRolePanel.AdditionalRoles")));
            this._theUserRolePanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theUserRolePanel.ConnectionDefinition = null;
            this._theUserRolePanel.DefaultRole = "";
            this._theUserRolePanel.Location = new System.Drawing.Point(3, 114);
            this._theUserRolePanel.Name = "_theUserRolePanel";
            this._theUserRolePanel.ShowAdditionalRole = true;
            this._theUserRolePanel.Size = new System.Drawing.Size(479, 159);
            this._theUserRolePanel.TabIndex = 0;
            // 
            // _theUsers
            // 
            this._theUsers.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUsers.Location = new System.Drawing.Point(127, 28);
            this._theUsers.Name = "_theUsers";
            this._theUsers.Size = new System.Drawing.Size(337, 80);
            this._theUsers.TabIndex = 4;
            this._theUsers.Text = "";
            // 
            // _theUserNameLabel
            // 
            this._theUserNameLabel.AutoSize = true;
            this._theUserNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserNameLabel.Location = new System.Drawing.Point(9, 32);
            this._theUserNameLabel.Name = "_theUserNameLabel";
            this._theUserNameLabel.ShowRequired = true;
            this._theUserNameLabel.Size = new System.Drawing.Size(116, 18);
            this._theUserNameLabel.TabIndex = 12;
            this._theUserNameLabel.TabStop = false;
            // 
            // AddUsersPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this._theMainPanel);
            this.Name = "AddUsersPanel";
            this.Size = new System.Drawing.Size(485, 276);
            this._theMainPanel.ResumeLayout(false);
            this._theMainPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private UserRolePanel _theUserRolePanel;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox _theUsers;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theUserNameLabel;
    }
}
