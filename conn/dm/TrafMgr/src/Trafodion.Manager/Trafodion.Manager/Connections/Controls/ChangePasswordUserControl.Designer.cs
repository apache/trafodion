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
namespace Trafodion.Manager.Connections.Controls
{
    partial class ChangePasswordUserControl
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
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theConfirmNewPasswordTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theNewPasswordTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theOldPasswordTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theUserNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSystemLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theSystemTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.TrafodionPanel2.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this._theSystemTextBox);
            this.TrafodionPanel2.Controls.Add(this._theSystemLabel);
            this.TrafodionPanel2.Controls.Add(this._theConfirmNewPasswordTextBox);
            this.TrafodionPanel2.Controls.Add(this._theNewPasswordTextBox);
            this.TrafodionPanel2.Controls.Add(this._theOldPasswordTextBox);
            this.TrafodionPanel2.Controls.Add(this._theUserNameTextBox);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel4);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel3);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel2);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel1);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel2.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(484, 199);
            this.TrafodionPanel2.TabIndex = 2;
            // 
            // _theConfirmNewPasswordTextBox
            // 
            this._theConfirmNewPasswordTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theConfirmNewPasswordTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theConfirmNewPasswordTextBox.Location = new System.Drawing.Point(166, 151);
            this._theConfirmNewPasswordTextBox.Name = "_theConfirmNewPasswordTextBox";
            this._theConfirmNewPasswordTextBox.PasswordChar = '*';
            this._theConfirmNewPasswordTextBox.Size = new System.Drawing.Size(289, 21);
            this._theConfirmNewPasswordTextBox.TabIndex = 4;
            this._theConfirmNewPasswordTextBox.UseSystemPasswordChar = true;
            // 
            // _theNewPasswordTextBox
            // 
            this._theNewPasswordTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theNewPasswordTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theNewPasswordTextBox.Location = new System.Drawing.Point(166, 118);
            this._theNewPasswordTextBox.Name = "_theNewPasswordTextBox";
            this._theNewPasswordTextBox.PasswordChar = '*';
            this._theNewPasswordTextBox.Size = new System.Drawing.Size(289, 21);
            this._theNewPasswordTextBox.TabIndex = 3;
            this._theNewPasswordTextBox.UseSystemPasswordChar = true;
            // 
            // _theOldPasswordTextBox
            // 
            this._theOldPasswordTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theOldPasswordTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theOldPasswordTextBox.Location = new System.Drawing.Point(166, 85);
            this._theOldPasswordTextBox.Name = "_theOldPasswordTextBox";
            this._theOldPasswordTextBox.PasswordChar = '*';
            this._theOldPasswordTextBox.Size = new System.Drawing.Size(289, 21);
            this._theOldPasswordTextBox.TabIndex = 2;
            this._theOldPasswordTextBox.UseSystemPasswordChar = true;
            // 
            // _theUserNameTextBox
            // 
            this._theUserNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theUserNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theUserNameTextBox.Location = new System.Drawing.Point(166, 52);
            this._theUserNameTextBox.Name = "_theUserNameTextBox";
            this._theUserNameTextBox.ReadOnly = true;
            this._theUserNameTextBox.Size = new System.Drawing.Size(289, 21);
            this._theUserNameTextBox.TabIndex = 1;
            this._theUserNameTextBox.TabStop = false;
            // 
            // TrafodionLabel4
            // 
            this.TrafodionLabel4.AutoSize = true;
            this.TrafodionLabel4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel4.Location = new System.Drawing.Point(30, 155);
            this.TrafodionLabel4.Name = "TrafodionLabel4";
            this.TrafodionLabel4.Size = new System.Drawing.Size(117, 13);
            this.TrafodionLabel4.TabIndex = 0;
            this.TrafodionLabel4.Text = "Confirm New Password";
            this.TrafodionLabel4.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel3.Location = new System.Drawing.Point(70, 122);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(77, 13);
            this.TrafodionLabel3.TabIndex = 0;
            this.TrafodionLabel3.Text = "New Password";
            this.TrafodionLabel3.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(75, 89);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(72, 13);
            this.TrafodionLabel2.TabIndex = 0;
            this.TrafodionLabel2.Text = "Old Password";
            this.TrafodionLabel2.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // TrafodionLabel1
            // 
            this.TrafodionLabel1.AutoSize = true;
            this.TrafodionLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel1.Location = new System.Drawing.Point(88, 56);
            this.TrafodionLabel1.Name = "TrafodionLabel1";
            this.TrafodionLabel1.Size = new System.Drawing.Size(59, 13);
            this.TrafodionLabel1.TabIndex = 0;
            this.TrafodionLabel1.Text = "User Name";
            this.TrafodionLabel1.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theSystemLabel
            // 
            this._theSystemLabel.AutoSize = true;
            this._theSystemLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSystemLabel.Location = new System.Drawing.Point(105, 23);
            this._theSystemLabel.Name = "_theSystemLabel";
            this._theSystemLabel.Size = new System.Drawing.Size(42, 13);
            this._theSystemLabel.TabIndex = 5;
            this._theSystemLabel.Text = "System";
            this._theSystemLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theSystemTextBox
            // 
            this._theSystemTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._theSystemTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theSystemTextBox.Location = new System.Drawing.Point(166, 19);
            this._theSystemTextBox.Name = "_theSystemTextBox";
            this._theSystemTextBox.ReadOnly = true;
            this._theSystemTextBox.Size = new System.Drawing.Size(289, 21);
            this._theSystemTextBox.TabIndex = 6;
            this._theSystemTextBox.TabStop = false;
            // 
            // ChangePasswordUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel2);
            this.Name = "ChangePasswordUserControl";
            this.Size = new System.Drawing.Size(484, 199);
            this.TrafodionPanel2.ResumeLayout(false);
            this.TrafodionPanel2.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theConfirmNewPasswordTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theNewPasswordTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theOldPasswordTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theUserNameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel4;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel3;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel TrafodionLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _theSystemLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theSystemTextBox;
    }
}
