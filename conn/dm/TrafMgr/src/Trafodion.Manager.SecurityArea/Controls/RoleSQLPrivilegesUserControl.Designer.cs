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
    partial class RoleSQLPrivilegesUserControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._privilegesPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._roleNameComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._roleNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._headerPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _privilegesPanel
            // 
            this._privilegesPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._privilegesPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._privilegesPanel.Location = new System.Drawing.Point(0, 46);
            this._privilegesPanel.Name = "_privilegesPanel";
            this._privilegesPanel.Size = new System.Drawing.Size(872, 545);
            this._privilegesPanel.TabIndex = 4;
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this._roleNameComboBox);
            this._headerPanel.Controls.Add(this._roleNameLabel);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(872, 46);
            this._headerPanel.TabIndex = 3;
            // 
            // _roleNameComboBox
            // 
            this._roleNameComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._roleNameComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._roleNameComboBox.FormattingEnabled = true;
            this._roleNameComboBox.Location = new System.Drawing.Point(73, 10);
            this._roleNameComboBox.Name = "_roleNameComboBox";
            this._roleNameComboBox.Size = new System.Drawing.Size(611, 21);
            this._roleNameComboBox.TabIndex = 1;
            // 
            // _roleNameLabel
            // 
            this._roleNameLabel.AutoSize = true;
            this._roleNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameLabel.Location = new System.Drawing.Point(9, 13);
            this._roleNameLabel.Name = "_roleNameLabel";
            this._roleNameLabel.Size = new System.Drawing.Size(58, 13);
            this._roleNameLabel.TabIndex = 0;
            this._roleNameLabel.Text = "Role Name";
            // 
            // RoleSQLPrivilegesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._privilegesPanel);
            this.Controls.Add(this._headerPanel);
            this.Name = "RoleSQLPrivilegesUserControl";
            this.Size = new System.Drawing.Size(872, 591);
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _privilegesPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _headerPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _roleNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _roleNameComboBox;
    }
}
