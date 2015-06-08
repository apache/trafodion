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
﻿namespace Trafodion.Manager.OverviewArea.Controls
{
    partial class AlarmStatusUpdateDialog
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
            this.notesPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._statusPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._statusGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._statusLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._statusComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._statusPanel.SuspendLayout();
            this._statusGroupBox.SuspendLayout();
            this._buttonsPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // notesPanel
            // 
            this.notesPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.notesPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.notesPanel.Location = new System.Drawing.Point(0, 47);
            this.notesPanel.Name = "notesPanel";
            this.notesPanel.Size = new System.Drawing.Size(706, 364);
            this.notesPanel.TabIndex = 2;
            // 
            // _statusPanel
            // 
            this._statusPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._statusPanel.Controls.Add(this._statusGroupBox);
            this._statusPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._statusPanel.Location = new System.Drawing.Point(0, 0);
            this._statusPanel.Name = "_statusPanel";
            this._statusPanel.Size = new System.Drawing.Size(706, 47);
            this._statusPanel.TabIndex = 0;
            // 
            // _statusGroupBox
            // 
            this._statusGroupBox.BackColor = System.Drawing.Color.WhiteSmoke;
            this._statusGroupBox.Controls.Add(this._statusLabel);
            this._statusGroupBox.Controls.Add(this._statusComboBox);
            this._statusGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._statusGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._statusGroupBox.Location = new System.Drawing.Point(0, 0);
            this._statusGroupBox.Name = "_statusGroupBox";
            this._statusGroupBox.Size = new System.Drawing.Size(706, 47);
            this._statusGroupBox.TabIndex = 2;
            this._statusGroupBox.TabStop = false;
            this._statusGroupBox.Text = "Select the new status of the alert(s)";
            // 
            // _statusLabel
            // 
            this._statusLabel.AutoSize = true;
            this._statusLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._statusLabel.Location = new System.Drawing.Point(6, 21);
            this._statusLabel.Name = "_statusLabel";
            this._statusLabel.Size = new System.Drawing.Size(38, 13);
            this._statusLabel.TabIndex = 0;
            this._statusLabel.Text = "Status";
            // 
            // _statusComboBox
            // 
            this._statusComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._statusComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._statusComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._statusComboBox.FormattingEnabled = true;
            this._statusComboBox.Location = new System.Drawing.Point(64, 16);
            this._statusComboBox.Name = "_statusComboBox";
            this._statusComboBox.Size = new System.Drawing.Size(219, 21);
            this._statusComboBox.TabIndex = 1;
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._helpButton);
            this._buttonsPanel.Controls.Add(this._cancelButton);
            this._buttonsPanel.Controls.Add(this._okButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 411);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(706, 37);
            this._buttonsPanel.TabIndex = 1;
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._cancelButton.Location = new System.Drawing.Point(540, 8);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 1;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            this._cancelButton.Click += new System.EventHandler(this._cancelButton_Click);
            // 
            // _okButton
            // 
            this._okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this._okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._okButton.Location = new System.Drawing.Point(459, 8);
            this._okButton.Name = "_okButton";
            this._okButton.Size = new System.Drawing.Size(75, 23);
            this._okButton.TabIndex = 0;
            this._okButton.Text = "&Apply";
            this._okButton.UseVisualStyleBackColor = true;
            this._okButton.Click += new System.EventHandler(this._okButton_Click);
            // 
            // _helpButton
            // 
            this._helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._helpButton.Location = new System.Drawing.Point(621, 8);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(75, 23);
            this._helpButton.TabIndex = 1;
            this._helpButton.Text = "&Help";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // AlarmStatusUpdateDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(706, 448);
            this.Controls.Add(this.notesPanel);
            this.Controls.Add(this._statusPanel);
            this.Controls.Add(this._buttonsPanel);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Name = "AlarmStatusUpdateDialog";
            this.Text = "HP Database Manager - Update Alert";
            this._statusPanel.ResumeLayout(false);
            this._statusGroupBox.ResumeLayout(false);
            this._statusGroupBox.PerformLayout();
            this._buttonsPanel.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _buttonsPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel notesPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _statusPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _statusComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _statusLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _statusGroupBox;
        private Framework.Controls.TrafodionButton _helpButton;
    }
}