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
namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class EditNDCSUserDialog
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._userNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.roleNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._grantButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.SuspendLayout();
            // 
            // _userNameLabel
            // 
            this._userNameLabel.AutoSize = true;
            this._userNameLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._userNameLabel.Location = new System.Drawing.Point(12, 9);
            this._userNameLabel.Name = "_userNameLabel";
            this._userNameLabel.Size = new System.Drawing.Size(106, 14);
            this._userNameLabel.TabIndex = 0;
            this._userNameLabel.Text = "Grantee Role Name :";
            // 
            // roleNameTextBox
            // 
            this.roleNameTextBox.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper;
            this.roleNameTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.roleNameTextBox.Location = new System.Drawing.Point(124, 6);
            this.roleNameTextBox.MaxLength = 17;
            this.roleNameTextBox.Name = "roleNameTextBox";
            this.roleNameTextBox.Size = new System.Drawing.Size(209, 20);
            this.roleNameTextBox.TabIndex = 1;
            this.roleNameTextBox.TextChanged += new System.EventHandler(this.roleNameTextBox_TextChanged);
            this.roleNameTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.roleNameTextBox_KeyPress);
            // 
            // _grantButton
            // 
            this._grantButton.Location = new System.Drawing.Point(86, 44);
            this._grantButton.Name = "_grantButton";
            this._grantButton.Size = new System.Drawing.Size(75, 23);
            this._grantButton.TabIndex = 3;
            this._grantButton.Text = "Grant";
            this._grantButton.UseVisualStyleBackColor = true;
            this._grantButton.Click += new System.EventHandler(this._grantButton_Click);
            // 
            // _cancelButton
            // 
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Location = new System.Drawing.Point(183, 44);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 3;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            // 
            // EditNDCSUserDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ClientSize = new System.Drawing.Size(345, 77);
            this.Controls.Add(this._cancelButton);
            this.Controls.Add(this._grantButton);
            this.Controls.Add(this.roleNameTextBox);
            this.Controls.Add(this._userNameLabel);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "EditHPDCSUserDialog";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "Trafodion Database Manager - AddHPDCSUserDialog";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _userNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox roleNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _grantButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
    }
}