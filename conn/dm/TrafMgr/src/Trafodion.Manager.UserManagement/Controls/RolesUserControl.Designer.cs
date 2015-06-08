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
    partial class RolesUserControl
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
            this._theCreateButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theWidgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAlterRoleButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._sqlPrivilegesButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theDropButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalButtonPanel.SuspendLayout();
            this._bottomPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theCreateButton
            // 
            this._theCreateButton.AutoSize = true;
            this._theCreateButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theCreateButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCreateButton.Location = new System.Drawing.Point(3, 6);
            this._theCreateButton.Name = "_theCreateButton";
            this._theCreateButton.Size = new System.Drawing.Size(76, 23);
            this._theCreateButton.TabIndex = 3;
            this._theCreateButton.Text = "&Create ...";
            this._theCreateButton.UseVisualStyleBackColor = true;
            this._theCreateButton.Click += new System.EventHandler(this._theCreateButton_Click);
            // 
            // _theWidgetPanel
            // 
            this._theWidgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetPanel.Location = new System.Drawing.Point(0, 0);
            this._theWidgetPanel.Name = "_theWidgetPanel";
            this._theWidgetPanel.Size = new System.Drawing.Size(725, 361);
            this._theWidgetPanel.TabIndex = 3;
            // 
            // _theAdditionalButtonPanel
            // 
            this._theAdditionalButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalButtonPanel.Controls.Add(this._theAlterRoleButton);
            this._theAdditionalButtonPanel.Controls.Add(this._theCreateButton);
            this._theAdditionalButtonPanel.Controls.Add(this._sqlPrivilegesButton);
            this._theAdditionalButtonPanel.Controls.Add(this._theDropButton);
            this._theAdditionalButtonPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalButtonPanel.Location = new System.Drawing.Point(0, 0);
            this._theAdditionalButtonPanel.Name = "_theAdditionalButtonPanel";
            this._theAdditionalButtonPanel.Size = new System.Drawing.Size(725, 33);
            this._theAdditionalButtonPanel.TabIndex = 1;
            // 
            // _theAlterRoleButton
            // 
            this._theAlterRoleButton.AutoSize = true;
            this._theAlterRoleButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAlterRoleButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAlterRoleButton.Location = new System.Drawing.Point(167, 6);
            this._theAlterRoleButton.Name = "_theAlterRoleButton";
            this._theAlterRoleButton.Size = new System.Drawing.Size(78, 23);
            this._theAlterRoleButton.TabIndex = 4;
            this._theAlterRoleButton.Text = "&Alter...";
            this._theAlterRoleButton.UseVisualStyleBackColor = true;
            this._theAlterRoleButton.Click += new System.EventHandler(this._theAlterRoleButton_Click);
            // 
            // _sqlPrivilegesButton
            // 
            this._sqlPrivilegesButton.AutoSize = true;
            this._sqlPrivilegesButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._sqlPrivilegesButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sqlPrivilegesButton.Location = new System.Drawing.Point(251, 6);
            this._sqlPrivilegesButton.Name = "_sqlPrivilegesButton";
            this._sqlPrivilegesButton.Size = new System.Drawing.Size(88, 23);
            this._sqlPrivilegesButton.TabIndex = 2;
            this._sqlPrivilegesButton.Text = "S&QL Privileges";
            this._sqlPrivilegesButton.UseVisualStyleBackColor = true;
            this._sqlPrivilegesButton.Click += new System.EventHandler(this.sqlPrivilegesButton_Click);
            // 
            // _theDropButton
            // 
            this._theDropButton.AutoSize = true;
            this._theDropButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theDropButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theDropButton.Location = new System.Drawing.Point(83, 6);
            this._theDropButton.Name = "_theDropButton";
            this._theDropButton.Size = new System.Drawing.Size(78, 23);
            this._theDropButton.TabIndex = 2;
            this._theDropButton.Text = "&Drop...";
            this._theDropButton.UseVisualStyleBackColor = true;
            this._theDropButton.Click += new System.EventHandler(this._theDropButton_Click);
            // 
            // _bottomPanel
            // 
            this._bottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._bottomPanel.Controls.Add(this._theAdditionalButtonPanel);
            this._bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._bottomPanel.Location = new System.Drawing.Point(0, 361);
            this._bottomPanel.Name = "_bottomPanel";
            this._bottomPanel.Size = new System.Drawing.Size(725, 33);
            this._bottomPanel.TabIndex = 2;
            // 
            // RolesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theWidgetPanel);
            this.Controls.Add(this._bottomPanel);
            this.Name = "RolesUserControl";
            this.Size = new System.Drawing.Size(725, 394);
            this._theAdditionalButtonPanel.ResumeLayout(false);
            this._theAdditionalButtonPanel.PerformLayout();
            this._bottomPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionButton _theCreateButton;
        private Framework.Controls.TrafodionPanel _theWidgetPanel;
        private Framework.Controls.TrafodionPanel _theAdditionalButtonPanel;
        private Framework.Controls.TrafodionButton _sqlPrivilegesButton;
        private Framework.Controls.TrafodionButton _theDropButton;
        private Framework.Controls.TrafodionPanel _bottomPanel;
        private Framework.Controls.TrafodionButton _theAlterRoleButton;
    }
}
