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
    partial class PlatformUserPanel
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
            this._thePlatformUserProperties = new Trafodion.Manager.SecurityArea.Controls.PlatformUserPropertyPanel();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theUserNameAndRolePanel = new Trafodion.Manager.SecurityArea.Controls.PlatformUserRolePanel();
            this._thePlatformUserProperty = new Trafodion.Manager.SecurityArea.Controls.PlatformUserPropertyPanel();
            this._theMainPanel.SuspendLayout();
            this.oneGuiPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.AutoSize = true;
            this._theMainPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._thePlatformUserProperties);
            this._theMainPanel.Controls.Add(this.oneGuiPanel1);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(482, 406);
            this._theMainPanel.TabIndex = 0;
            // 
            // _thePlatformUserProperties
            // 
            this._thePlatformUserProperties.AutoSize = true;
            this._thePlatformUserProperties.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._thePlatformUserProperties.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thePlatformUserProperties.Location = new System.Drawing.Point(0, 65);
            this._thePlatformUserProperties.Name = "_thePlatformUserProperties";
            this._thePlatformUserProperties.Size = new System.Drawing.Size(482, 341);
            this._thePlatformUserProperties.TabIndex = 0;
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this._theUserNameAndRolePanel);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(482, 65);
            this.oneGuiPanel1.TabIndex = 17;
            // 
            // _theUserNameAndRolePanel
            // 
            this._theUserNameAndRolePanel.AutoSize = true;
            this._theUserNameAndRolePanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theUserNameAndRolePanel.ConnectionDefinition = null;
            this._theUserNameAndRolePanel.Location = new System.Drawing.Point(-2, 3);
            this._theUserNameAndRolePanel.Mode = Trafodion.Manager.SecurityArea.Model.User.EditMode.Create;
            this._theUserNameAndRolePanel.Name = "_theUserNameAndRolePanel";
            this._theUserNameAndRolePanel.Role = "";
            this._theUserNameAndRolePanel.Size = new System.Drawing.Size(458, 59);
            this._theUserNameAndRolePanel.TabIndex = 0;
            this._theUserNameAndRolePanel.UserName = "";
            // 
            // _thePlatformUserProperty
            // 
            this._thePlatformUserProperty.AutoSize = true;
            this._thePlatformUserProperty.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._thePlatformUserProperty.Dock = System.Windows.Forms.DockStyle.Fill;
            this._thePlatformUserProperty.Location = new System.Drawing.Point(0, 65);
            this._thePlatformUserProperty.Name = "_thePlatformUserProperty";
            this._thePlatformUserProperty.Size = new System.Drawing.Size(482, 341);
            this._thePlatformUserProperty.TabIndex = 0;
            // 
            // PlatformUserPanel
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this._theMainPanel);
            this.Name = "PlatformUserPanel";
            this.Size = new System.Drawing.Size(482, 406);
            this._theMainPanel.ResumeLayout(false);
            this._theMainPanel.PerformLayout();
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiPanel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private PlatformUserPropertyPanel _thePlatformUserProperties;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private PlatformUserRolePanel _theUserNameAndRolePanel;
        private PlatformUserPropertyPanel _thePlatformUserProperty;
    }
}
