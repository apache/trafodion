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
namespace Trafodion.Manager.Connections.Controls
{
    partial class ChangePasswordDialog
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ChangePasswordDialog));
            this._theUpperPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theStatusTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMessagePanel();
            this._theChangePasswordUserControl = new Trafodion.Manager.Connections.Controls.ChangePasswordUserControl();
            this._theBottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theUpperPanel.SuspendLayout();
            this._theBottomPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theUpperPanel
            // 
            this._theUpperPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theUpperPanel.Controls.Add(this._theStatusTextBox);
            this._theUpperPanel.Controls.Add(this._theChangePasswordUserControl);
            this._theUpperPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theUpperPanel.Location = new System.Drawing.Point(0, 0);
            this._theUpperPanel.Name = "_theUpperPanel";
            this._theUpperPanel.Size = new System.Drawing.Size(439, 222);
            this._theUpperPanel.TabIndex = 1;
            // 
            // _theStatusTextBox
            // 
            this._theStatusTextBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._theStatusTextBox.Location = new System.Drawing.Point(0, 0);
            this._theStatusTextBox.Name = "_theStatusTextBox";
            this._theStatusTextBox.Size = new System.Drawing.Size(439, 25);
            this._theStatusTextBox.TabIndex = 0;
            this._theStatusTextBox.Visible = false;
            // 
            // _theChangePasswordUserControl
            // 
            this._theChangePasswordUserControl.ConfirmNewPassword = "";
            this._theChangePasswordUserControl.Location = new System.Drawing.Point(-1, 32);
            this._theChangePasswordUserControl.Name = "_theChangePasswordUserControl";
            this._theChangePasswordUserControl.NewPassword = "";
            this._theChangePasswordUserControl.OldPassword = "";
            this._theChangePasswordUserControl.Size = new System.Drawing.Size(440, 197);
            this._theChangePasswordUserControl.SystemName = "";
            this._theChangePasswordUserControl.TabIndex = 0;
            this._theChangePasswordUserControl.UserName = "";
            // 
            // _theBottomPanel
            // 
            this._theBottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theBottomPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this._theBottomPanel.Controls.Add(this._applyButton);
            this._theBottomPanel.Controls.Add(this._cancelButton);
            this._theBottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theBottomPanel.Location = new System.Drawing.Point(0, 222);
            this._theBottomPanel.Name = "_theBottomPanel";
            this._theBottomPanel.Size = new System.Drawing.Size(439, 33);
            this._theBottomPanel.TabIndex = 0;
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Location = new System.Drawing.Point(128, 3);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(89, 23);
            this._applyButton.TabIndex = 1;
            this._applyButton.Text = "&OK";
            this._applyButton.UseVisualStyleBackColor = true;
            this._applyButton.Click += new System.EventHandler(this._applyButton_Click);
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(223, 3);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(89, 23);
            this._cancelButton.TabIndex = 0;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            this._cancelButton.Click += new System.EventHandler(this._cancelButton_Click);
            // 
            // ChangePasswordDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(439, 255);
            this.Controls.Add(this._theUpperPanel);
            this.Controls.Add(this._theBottomPanel);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "ChangePasswordDialog";
            this.Text = "Trafodion Database Manager - Change Password";
            this._theUpperPanel.ResumeLayout(false);
            this._theBottomPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theBottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _applyButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theUpperPanel;
        private ChangePasswordUserControl _theChangePasswordUserControl;
        private Trafodion.Manager.Framework.Controls.TrafodionMessagePanel _theStatusTextBox;
    }
}
