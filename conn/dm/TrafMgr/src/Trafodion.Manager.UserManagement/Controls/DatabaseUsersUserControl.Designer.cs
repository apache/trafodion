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
    partial class DatabaseUsersUserControl
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
            this._theWidgetPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._bottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theAdditionalButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theRegisterButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._sqlPrivilegesButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAlterUserButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theUnRegisterButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._bottomPanel.SuspendLayout();
            this._theAdditionalButtonPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theWidgetPanel
            // 
            this._theWidgetPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theWidgetPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theWidgetPanel.Location = new System.Drawing.Point(0, 0);
            this._theWidgetPanel.Name = "_theWidgetPanel";
            this._theWidgetPanel.Size = new System.Drawing.Size(1059, 371);
            this._theWidgetPanel.TabIndex = 1;
            // 
            // _bottomPanel
            // 
            this._bottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._bottomPanel.Controls.Add(this._theAdditionalButtonPanel);
            this._bottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._bottomPanel.Location = new System.Drawing.Point(0, 371);
            this._bottomPanel.Name = "_bottomPanel";
            this._bottomPanel.Size = new System.Drawing.Size(1059, 33);
            this._bottomPanel.TabIndex = 0;
            // 
            // _theAdditionalButtonPanel
            // 
            this._theAdditionalButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theAdditionalButtonPanel.Controls.Add(this._theRegisterButton);
            this._theAdditionalButtonPanel.Controls.Add(this._sqlPrivilegesButton);
            this._theAdditionalButtonPanel.Controls.Add(this._theAlterUserButton);
            this._theAdditionalButtonPanel.Controls.Add(this._theUnRegisterButton);
            this._theAdditionalButtonPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theAdditionalButtonPanel.Location = new System.Drawing.Point(0, 0);
            this._theAdditionalButtonPanel.Name = "_theAdditionalButtonPanel";
            this._theAdditionalButtonPanel.Size = new System.Drawing.Size(1059, 33);
            this._theAdditionalButtonPanel.TabIndex = 1;
            // 
            // _theRegisterButton
            // 
            this._theRegisterButton.AutoSize = true;
            this._theRegisterButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theRegisterButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theRegisterButton.Location = new System.Drawing.Point(3, 5);
            this._theRegisterButton.Name = "_theRegisterButton";
            this._theRegisterButton.Size = new System.Drawing.Size(76, 23);
            this._theRegisterButton.TabIndex = 3;
            this._theRegisterButton.Text = "&Register ...";
            this._theRegisterButton.UseVisualStyleBackColor = true;
            this._theRegisterButton.Click += new System.EventHandler(this._theRegisterButton_Click);
            // 
            // _sqlPrivilegesButton
            // 
            this._sqlPrivilegesButton.AutoSize = true;
            this._sqlPrivilegesButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._sqlPrivilegesButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._sqlPrivilegesButton.Location = new System.Drawing.Point(250, 5);
            this._sqlPrivilegesButton.Name = "_sqlPrivilegesButton";
            this._sqlPrivilegesButton.Size = new System.Drawing.Size(88, 23);
            this._sqlPrivilegesButton.TabIndex = 2;
            this._sqlPrivilegesButton.Text = "S&QL Privileges";
            this._sqlPrivilegesButton.UseVisualStyleBackColor = true;
            this._sqlPrivilegesButton.Click += new System.EventHandler(this.sqlPrivilegesButton_Click);
            // 
            // _theAlterUserButton
            // 
            this._theAlterUserButton.AutoSize = true;
            this._theAlterUserButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theAlterUserButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAlterUserButton.Location = new System.Drawing.Point(167, 5);
            this._theAlterUserButton.Name = "_theAlterUserButton";
            this._theAlterUserButton.Size = new System.Drawing.Size(78, 23);
            this._theAlterUserButton.TabIndex = 2;
            this._theAlterUserButton.Text = "&Alter...";
            this._theAlterUserButton.UseVisualStyleBackColor = true;
            this._theAlterUserButton.Click += new System.EventHandler(this._theAlterUserButton_Click);
            // 
            // _theUnRegisterButton
            // 
            this._theUnRegisterButton.AutoSize = true;
            this._theUnRegisterButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theUnRegisterButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUnRegisterButton.Location = new System.Drawing.Point(84, 5);
            this._theUnRegisterButton.Name = "_theUnRegisterButton";
            this._theUnRegisterButton.Size = new System.Drawing.Size(78, 23);
            this._theUnRegisterButton.TabIndex = 2;
            this._theUnRegisterButton.Text = "&Un-Register";
            this._theUnRegisterButton.UseVisualStyleBackColor = true;
            this._theUnRegisterButton.Click += new System.EventHandler(this._theUnRegisterButton_Click);
            // 
            // DatabaseUsersUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.Controls.Add(this._theWidgetPanel);
            this.Controls.Add(this._bottomPanel);
            this.Name = "DatabaseUsersUserControl";
            this.Size = new System.Drawing.Size(1059, 404);
            this.Load += new System.EventHandler(this.DatabaseUsersUserControl_Load);
            this._bottomPanel.ResumeLayout(false);
            this._theAdditionalButtonPanel.ResumeLayout(false);
            this._theAdditionalButtonPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _bottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theAdditionalButtonPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theUnRegisterButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theWidgetPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theRegisterButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _sqlPrivilegesButton;
        private Framework.Controls.TrafodionButton _theAlterUserButton;




    }
}
