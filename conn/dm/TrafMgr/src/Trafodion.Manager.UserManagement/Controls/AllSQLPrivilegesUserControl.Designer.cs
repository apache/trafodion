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
    partial class AllSQLPrivilegesUserControl
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
            this._userNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._userNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._headerPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _privilegesPanel
            // 
            this._privilegesPanel.AutoScroll = true;
            this._privilegesPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._privilegesPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._privilegesPanel.Location = new System.Drawing.Point(0, 78);
            this._privilegesPanel.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._privilegesPanel.Name = "_privilegesPanel";
            this._privilegesPanel.Size = new System.Drawing.Size(1163, 604);
            this._privilegesPanel.TabIndex = 4;
            // 
            // _headerPanel
            // 
            this._headerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._headerPanel.Controls.Add(this._userNameTextBox);
            this._headerPanel.Controls.Add(this._userNameLabel);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(1163, 78);
            this._headerPanel.TabIndex = 3;
            // 
            // _userNameTextBox
            // 
            this._userNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._userNameTextBox.Location = new System.Drawing.Point(94, 27);
            this._userNameTextBox.Name = "_userNameTextBox";
            this._userNameTextBox.ReadOnly = true;
            this._userNameTextBox.Size = new System.Drawing.Size(813, 24);
            this._userNameTextBox.TabIndex = 3;
            // 
            // _userNameLabel
            // 
            this._userNameLabel.AutoSize = true;
            this._userNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._userNameLabel.Location = new System.Drawing.Point(13, 34);
            this._userNameLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._userNameLabel.Name = "_userNameLabel";
            this._userNameLabel.Size = new System.Drawing.Size(74, 17);
            this._userNameLabel.TabIndex = 0;
            this._userNameLabel.Text = "User Name";
            // 
            // AllSQLPrivilegesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._privilegesPanel);
            this.Controls.Add(this._headerPanel);
            this.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this.Name = "AllSQLPrivilegesUserControl";
            this.Size = new System.Drawing.Size(1163, 682);
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _privilegesPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _headerPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _userNameLabel;
        private Framework.Controls.TrafodionTextBox _userNameTextBox;
    }
}
