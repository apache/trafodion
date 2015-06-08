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
    partial class ConfigureDirectoryServerDialog
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
            this._theBottomPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theResetButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theAddButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.oneGuiPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._directoryServerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._serverTypeComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theStatusPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theStatusTextBox = new Trafodion.Manager.Framework.Controls.TrafodionMessagePanel();
            this._theBottomPanel.SuspendLayout();
            this.oneGuiPanel2.SuspendLayout();
            this.oneGuiPanel1.SuspendLayout();
            this._theStatusPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theBottomPanel
            // 
            this._theBottomPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theBottomPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this._theBottomPanel.Controls.Add(this._theResetButton);
            this._theBottomPanel.Controls.Add(this._theAddButton);
            this._theBottomPanel.Controls.Add(this._theCancelButton);
            this._theBottomPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theBottomPanel.Location = new System.Drawing.Point(0, 559);
            this._theBottomPanel.Name = "_theBottomPanel";
            this._theBottomPanel.Size = new System.Drawing.Size(668, 36);
            this._theBottomPanel.TabIndex = 1;
            // 
            // _theResetButton
            // 
            this._theResetButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theResetButton.Location = new System.Drawing.Point(10, 6);
            this._theResetButton.Name = "_theResetButton";
            this._theResetButton.Size = new System.Drawing.Size(84, 23);
            this._theResetButton.TabIndex = 1;
            this._theResetButton.Text = "&Reload";
            this._theResetButton.UseVisualStyleBackColor = true;
            this._theResetButton.Click += new System.EventHandler(this._theResetButton_Click);
            // 
            // _theAddButton
            // 
            this._theAddButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theAddButton.Enabled = false;
            this._theAddButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theAddButton.Location = new System.Drawing.Point(471, 6);
            this._theAddButton.Name = "_theAddButton";
            this._theAddButton.Size = new System.Drawing.Size(84, 23);
            this._theAddButton.TabIndex = 2;
            this._theAddButton.Text = "&OK";
            this._theAddButton.UseVisualStyleBackColor = true;
            this._theAddButton.Click += new System.EventHandler(this._theOKButton_Click);
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(570, 6);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(84, 23);
            this._theCancelButton.TabIndex = 3;
            this._theCancelButton.Text = "&Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // oneGuiPanel2
            // 
            this.oneGuiPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel2.Controls.Add(this._directoryServerPanel);
            this.oneGuiPanel2.Controls.Add(this.oneGuiPanel1);
            this.oneGuiPanel2.Controls.Add(this._theStatusPanel);
            this.oneGuiPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiPanel2.Location = new System.Drawing.Point(0, 0);
            this.oneGuiPanel2.Name = "oneGuiPanel2";
            this.oneGuiPanel2.Size = new System.Drawing.Size(668, 559);
            this.oneGuiPanel2.TabIndex = 0;
            // 
            // _directoryServerPanel
            // 
            this._directoryServerPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._directoryServerPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._directoryServerPanel.Location = new System.Drawing.Point(0, 68);
            this._directoryServerPanel.Name = "_directoryServerPanel";
            this._directoryServerPanel.Size = new System.Drawing.Size(668, 491);
            this._directoryServerPanel.TabIndex = 3;
            // 
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this._serverTypeComboBox);
            this.oneGuiPanel1.Controls.Add(this.oneGuiLabel1);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 24);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(668, 44);
            this.oneGuiPanel1.TabIndex = 2;
            // 
            // _serverTypeComboBox
            // 
            this._serverTypeComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this._serverTypeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._serverTypeComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._serverTypeComboBox.FormattingEnabled = true;
            this._serverTypeComboBox.Items.AddRange(new object[] {
            "LDAP",
            "Active Directory"});
            this._serverTypeComboBox.Location = new System.Drawing.Point(134, 12);
            this._serverTypeComboBox.Name = "_serverTypeComboBox";
            this._serverTypeComboBox.Size = new System.Drawing.Size(487, 21);
            this._serverTypeComboBox.TabIndex = 17;
            this._serverTypeComboBox.SelectedIndexChanged += new System.EventHandler(this._serverTypeComboBox_SelectedIndexChanged);
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(51, 15);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(66, 13);
            this.oneGuiLabel1.TabIndex = 15;
            this.oneGuiLabel1.Text = "Server Type";
            this.oneGuiLabel1.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // _theStatusPanel
            // 
            this._theStatusPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theStatusPanel.Controls.Add(this._theStatusTextBox);
            this._theStatusPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._theStatusPanel.Location = new System.Drawing.Point(0, 0);
            this._theStatusPanel.Name = "_theStatusPanel";
            this._theStatusPanel.Size = new System.Drawing.Size(668, 24);
            this._theStatusPanel.TabIndex = 1;
            // 
            // _theStatusTextBox
            // 
            this._theStatusTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theStatusTextBox.Location = new System.Drawing.Point(0, 0);
            this._theStatusTextBox.Name = "_theStatusTextBox";
            this._theStatusTextBox.Size = new System.Drawing.Size(668, 24);
            this._theStatusTextBox.TabIndex = 0;
            // 
            // ConfigureDirectoryServerDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.ClientSize = new System.Drawing.Size(668, 595);
            this.Controls.Add(this.oneGuiPanel2);
            this.Controls.Add(this._theBottomPanel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Name = "ConfigureDirectoryServerDialog";
            this.Text = "HP Database Manager - Add Directory Server";
            this._theBottomPanel.ResumeLayout(false);
            this.oneGuiPanel2.ResumeLayout(false);
            this.oneGuiPanel1.ResumeLayout(false);
            this.oneGuiPanel1.PerformLayout();
            this._theStatusPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theBottomPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theAddButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel2;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theResetButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theStatusPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel oneGuiPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _directoryServerPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _serverTypeComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionMessagePanel _theStatusTextBox;
    }
}
