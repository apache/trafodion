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
    partial class DatabaseUserPanel
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DatabaseUserPanel));
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._thePlatformUserProperties = new Trafodion.Manager.SecurityArea.Controls.PlatformUserPropertyPanel();
            this._theUserRolePanel = new Trafodion.Manager.SecurityArea.Controls.UserRolePanel();
            this._theUserNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theUserNameText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theSelectorPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theCreatePlatformUserCheck = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.oneGuiLabel8 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theMainPanel.SuspendLayout();
            this._theSelectorPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.AutoSize = true;
            this._theMainPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._thePlatformUserProperties);
            this._theMainPanel.Controls.Add(this._theUserRolePanel);
            this._theMainPanel.Controls.Add(this._theUserNameLabel);
            this._theMainPanel.Controls.Add(this._theUserNameText);
            this._theMainPanel.Controls.Add(this._theSelectorPanel);
            this._theMainPanel.Location = new System.Drawing.Point(3, 3);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(496, 563);
            this._theMainPanel.TabIndex = 10;
            // 
            // _thePlatformUserProperties
            // 
            this._thePlatformUserProperties.AutoSize = true;
            this._thePlatformUserProperties.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._thePlatformUserProperties.Location = new System.Drawing.Point(4, 219);
            this._thePlatformUserProperties.Name = "_thePlatformUserProperties";
            this._thePlatformUserProperties.Size = new System.Drawing.Size(482, 341);
            this._thePlatformUserProperties.TabIndex = 4;
            this._thePlatformUserProperties.TabStop = false;
            // 
            // _theUserRolePanel
            // 
            this._theUserRolePanel.AdditionalRoles = ((System.Collections.Generic.List<string>)(resources.GetObject("_theUserRolePanel.AdditionalRoles")));
            this._theUserRolePanel.ConnectionDefinition = null;
            this._theUserRolePanel.DefaultRole = "";
            this._theUserRolePanel.Location = new System.Drawing.Point(9, 40);
            this._theUserRolePanel.Name = "_theUserRolePanel";
            this._theUserRolePanel.ShowAdditionalRole = true;
            this._theUserRolePanel.Size = new System.Drawing.Size(477, 158);
            this._theUserRolePanel.TabIndex = 2;
            // 
            // _theUserNameLabel
            // 
            this._theUserNameLabel.AutoSize = true;
            this._theUserNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserNameLabel.Location = new System.Drawing.Point(18, 20);
            this._theUserNameLabel.Name = "_theUserNameLabel";
            this._theUserNameLabel.ShowRequired = true;
            this._theUserNameLabel.Size = new System.Drawing.Size(116, 18);
            this._theUserNameLabel.TabIndex = 11;
            this._theUserNameLabel.TabStop = false;
            // 
            // _theUserNameText
            // 
            this._theUserNameText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserNameText.Location = new System.Drawing.Point(134, 17);
            this._theUserNameText.Name = "_theUserNameText";
            this._theUserNameText.Size = new System.Drawing.Size(343, 21);
            this._theUserNameText.TabIndex = 1;
            // 
            // _theSelectorPanel
            // 
            this._theSelectorPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theSelectorPanel.Controls.Add(this._theCreatePlatformUserCheck);
            this._theSelectorPanel.Controls.Add(this.oneGuiLabel8);
            this._theSelectorPanel.Location = new System.Drawing.Point(6, 200);
            this._theSelectorPanel.Name = "_theSelectorPanel";
            this._theSelectorPanel.Size = new System.Drawing.Size(487, 22);
            this._theSelectorPanel.TabIndex = 3;
            // 
            // _theCreatePlatformUserCheck
            // 
            this._theCreatePlatformUserCheck.AutoSize = true;
            this._theCreatePlatformUserCheck.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCreatePlatformUserCheck.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCreatePlatformUserCheck.Location = new System.Drawing.Point(3, 4);
            this._theCreatePlatformUserCheck.Name = "_theCreatePlatformUserCheck";
            this._theCreatePlatformUserCheck.Size = new System.Drawing.Size(149, 18);
            this._theCreatePlatformUserCheck.TabIndex = 1;
            this._theCreatePlatformUserCheck.Text = "Locally Authenticated User";
            this._theCreatePlatformUserCheck.UseVisualStyleBackColor = true;
            this._theCreatePlatformUserCheck.Click += new System.EventHandler(this._theCreatePlatformUserCheck_Click);
            // 
            // oneGuiLabel8
            // 
            this.oneGuiLabel8.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.oneGuiLabel8.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel8.Location = new System.Drawing.Point(156, 13);
            this.oneGuiLabel8.Name = "oneGuiLabel8";
            this.oneGuiLabel8.Size = new System.Drawing.Size(320, 2);
            this.oneGuiLabel8.TabIndex = 1;
            this.oneGuiLabel8.Text = "oneGuiLabel8";
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(66, 20);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(59, 13);
            this.oneGuiLabel1.TabIndex = 11;
            this.oneGuiLabel1.Text = "User Name";
            // 
            // DatabaseUserPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this._theMainPanel);
            this.Name = "DatabaseUserPanel";
            this.Size = new System.Drawing.Size(502, 569);
            this._theMainPanel.ResumeLayout(false);
            this._theMainPanel.PerformLayout();
            this._theSelectorPanel.ResumeLayout(false);
            this._theSelectorPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theSelectorPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel8;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theUserNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theUserNameText;
        private UserRolePanel _theUserRolePanel;
        private PlatformUserPropertyPanel _thePlatformUserProperties;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _theCreatePlatformUserCheck;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
    }
}
