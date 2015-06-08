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
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class LibraryPrivilegeOptionsControl
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
            this._privGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._executeCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._objectLevelPrivilegesGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._usageCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._updateCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._allCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this._privGroupBox.SuspendLayout();
            this._objectLevelPrivilegesGroupBox.SuspendLayout();
            this.TrafodionPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _privGroupBox
            // 
            this._privGroupBox.Controls.Add(this._executeCheckBox);
            this._privGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._privGroupBox.Location = new System.Drawing.Point(7, 2);
            this._privGroupBox.Name = "_privGroupBox";
            this._privGroupBox.Size = new System.Drawing.Size(280, 62);
            this._privGroupBox.TabIndex = 5;
            this._privGroupBox.TabStop = false;
            this._privGroupBox.Text = "Library Privileges";
            // 
            // _executeCheckBox
            // 
            this._executeCheckBox.AutoSize = true;
            this._executeCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._executeCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._executeCheckBox.Location = new System.Drawing.Point(24, 32);
            this._executeCheckBox.Name = "_executeCheckBox";
            this._executeCheckBox.Size = new System.Drawing.Size(71, 18);
            this._executeCheckBox.TabIndex = 4;
            this._executeCheckBox.Text = "Execute";
            this._executeCheckBox.UseVisualStyleBackColor = true;
            // 
            // _objectLevelPrivilegesGroupBox
            // 
            this._objectLevelPrivilegesGroupBox.Controls.Add(this._usageCheckBox);
            this._objectLevelPrivilegesGroupBox.Controls.Add(this._updateCheckBox);
            this._objectLevelPrivilegesGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._objectLevelPrivilegesGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._objectLevelPrivilegesGroupBox.Location = new System.Drawing.Point(0, 39);
            this._objectLevelPrivilegesGroupBox.Name = "_objectLevelPrivilegesGroupBox";
            this._objectLevelPrivilegesGroupBox.Size = new System.Drawing.Size(382, 55);
            this._objectLevelPrivilegesGroupBox.TabIndex = 10;
            this._objectLevelPrivilegesGroupBox.TabStop = false;
            this._objectLevelPrivilegesGroupBox.Text = "Object Level Privileges";
            // 
            // _usageCheckBox
            // 
            this._usageCheckBox.AutoSize = true;
            this._usageCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._usageCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._usageCheckBox.Location = new System.Drawing.Point(121, 22);
            this._usageCheckBox.Name = "_usageCheckBox";
            this._usageCheckBox.Size = new System.Drawing.Size(62, 18);
            this._usageCheckBox.TabIndex = 7;
            this._usageCheckBox.Text = "Usage";
            this._usageCheckBox.UseVisualStyleBackColor = true;
            this._usageCheckBox.CheckedChanged += new System.EventHandler(this._usageCheckBox_CheckedChanged);
            // 
            // _updateCheckBox
            // 
            this._updateCheckBox.AutoSize = true;
            this._updateCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._updateCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._updateCheckBox.Location = new System.Drawing.Point(12, 22);
            this._updateCheckBox.Name = "_updateCheckBox";
            this._updateCheckBox.Size = new System.Drawing.Size(67, 18);
            this._updateCheckBox.TabIndex = 6;
            this._updateCheckBox.Text = "Update";
            this._updateCheckBox.UseVisualStyleBackColor = true;
            this._updateCheckBox.CheckedChanged += new System.EventHandler(this._usageCheckBox_CheckedChanged);
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._allCheckBox);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(382, 39);
            this.TrafodionPanel1.TabIndex = 11;
            // 
            // _allCheckBox
            // 
            this._allCheckBox.AutoSize = true;
            this._allCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._allCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._allCheckBox.Location = new System.Drawing.Point(12, 8);
            this._allCheckBox.Name = "_allCheckBox";
            this._allCheckBox.Size = new System.Drawing.Size(49, 18);
            this._allCheckBox.TabIndex = 4;
            this._allCheckBox.Text = "ALL";
            this._allCheckBox.UseVisualStyleBackColor = true;
            this._allCheckBox.CheckedChanged += new System.EventHandler(this._allCheckBox_CheckedChanged);
            // 
            // LibraryPrivilegeOptionsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoScroll = true;
            this.Controls.Add(this._objectLevelPrivilegesGroupBox);
            this.Controls.Add(this.TrafodionPanel1);
            this.Controls.Add(this._privGroupBox);
            this.Name = "LibraryPrivilegeOptionsControl";
            this.Size = new System.Drawing.Size(382, 202);
            this._privGroupBox.ResumeLayout(false);
            this._privGroupBox.PerformLayout();
            this._objectLevelPrivilegesGroupBox.ResumeLayout(false);
            this._objectLevelPrivilegesGroupBox.PerformLayout();
            this.TrafodionPanel1.ResumeLayout(false);
            this.TrafodionPanel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _privGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _executeCheckBox;
        private Framework.Controls.TrafodionGroupBox _objectLevelPrivilegesGroupBox;
        private Framework.Controls.TrafodionCheckBox _usageCheckBox;
        private Framework.Controls.TrafodionCheckBox _updateCheckBox;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Framework.Controls.TrafodionCheckBox _allCheckBox;
    }
}
