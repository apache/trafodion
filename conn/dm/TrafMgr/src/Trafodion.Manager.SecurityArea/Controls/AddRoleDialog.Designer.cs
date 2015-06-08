// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//
namespace Trafodion.Manager.SecurityArea.Controls
{
    partial class AddRoleDialog
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
            this._roleNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._roleNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.SuspendLayout();
            // 
            // _roleNameLabel
            // 
            this._roleNameLabel.AutoSize = true;
            this._roleNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameLabel.Location = new System.Drawing.Point(12, 9);
            this._roleNameLabel.Name = "_roleNameLabel";
            this._roleNameLabel.Size = new System.Drawing.Size(252, 13);
            this._roleNameLabel.TabIndex = 0;
            this._roleNameLabel.Text = "Enter one or more role names separated by comma";
            // 
            // _roleNameTextBox
            // 
            this._roleNameTextBox.CharacterCasing = System.Windows.Forms.CharacterCasing.Upper;
            this._roleNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._roleNameTextBox.Location = new System.Drawing.Point(15, 25);
            this._roleNameTextBox.Multiline = true;
            this._roleNameTextBox.Name = "_roleNameTextBox";
            this._roleNameTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this._roleNameTextBox.Size = new System.Drawing.Size(324, 99);
            this._roleNameTextBox.TabIndex = 0;
            this._roleNameTextBox.TextChanged += new System.EventHandler(this._roleNameTextBox_TextChanged);
            // 
            // _okButton
            // 
            this._okButton.Enabled = false;
            this._okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._okButton.Location = new System.Drawing.Point(86, 139);
            this._okButton.Name = "_okButton";
            this._okButton.Size = new System.Drawing.Size(75, 23);
            this._okButton.TabIndex = 1;
            this._okButton.Text = "&OK";
            this._okButton.UseVisualStyleBackColor = true;
            this._okButton.Click += new System.EventHandler(this._okButton_Click);
            // 
            // _cancelButton
            // 
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(190, 139);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 2;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            // 
            // AddRoleDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(351, 168);
            this.Controls.Add(this._cancelButton);
            this.Controls.Add(this._okButton);
            this.Controls.Add(this._roleNameTextBox);
            this.Controls.Add(this._roleNameLabel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "AddRoleDialog";
            this.Text = "HP Database Manager - Add Role(s)";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionLabel _roleNameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _roleNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
    }
}
