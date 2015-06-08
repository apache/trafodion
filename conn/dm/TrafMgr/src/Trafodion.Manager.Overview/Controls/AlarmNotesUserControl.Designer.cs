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
    partial class AlarmNotesUserControl
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
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.existingNotesPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._updateHistoryTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this._updateHistoryLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.customNotesTextBox = new Trafodion.Manager.Framework.Controls.TrafodionRichTextBox();
            this.customNoteRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.noActionRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.duplicateAlertRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.falseAlertRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.alertResolvedRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.TrafodionGroupBox1.SuspendLayout();
            this.existingNotesPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionGroupBox1.Controls.Add(this.existingNotesPanel);
            this.TrafodionGroupBox1.Controls.Add(this.customNotesTextBox);
            this.TrafodionGroupBox1.Controls.Add(this.customNoteRadioButton);
            this.TrafodionGroupBox1.Controls.Add(this.noActionRadioButton);
            this.TrafodionGroupBox1.Controls.Add(this.duplicateAlertRadioButton);
            this.TrafodionGroupBox1.Controls.Add(this.falseAlertRadioButton);
            this.TrafodionGroupBox1.Controls.Add(this.alertResolvedRadioButton);
            this.TrafodionGroupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(742, 369);
            this.TrafodionGroupBox1.TabIndex = 1;
            this.TrafodionGroupBox1.TabStop = false;
            this.TrafodionGroupBox1.Text = "Select a predefined note or enter your own:";
            // 
            // existingNotesPanel
            // 
            this.existingNotesPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.existingNotesPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.existingNotesPanel.Controls.Add(this._updateHistoryTextBox);
            this.existingNotesPanel.Controls.Add(this._updateHistoryLabel);
            this.existingNotesPanel.Location = new System.Drawing.Point(30, 216);
            this.existingNotesPanel.Name = "existingNotesPanel";
            this.existingNotesPanel.Size = new System.Drawing.Size(693, 126);
            this.existingNotesPanel.TabIndex = 6;
            // 
            // _updateHistoryTextBox
            // 
            this._updateHistoryTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._updateHistoryTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._updateHistoryTextBox.Location = new System.Drawing.Point(0, 13);
            this._updateHistoryTextBox.MaxLength = 1024;
            this._updateHistoryTextBox.Name = "_updateHistoryTextBox";
            this._updateHistoryTextBox.ReadOnly = true;
            this._updateHistoryTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.Vertical;
            this._updateHistoryTextBox.Size = new System.Drawing.Size(693, 113);
            this._updateHistoryTextBox.TabIndex = 4;
            this._updateHistoryTextBox.Text = "";
            this._updateHistoryTextBox.TextChanged += new System.EventHandler(this.customNotesTextBox_TextChanged);
            // 
            // _updateHistoryLabel
            // 
            this._updateHistoryLabel.AutoSize = true;
            this._updateHistoryLabel.Dock = System.Windows.Forms.DockStyle.Top;
            this._updateHistoryLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._updateHistoryLabel.Location = new System.Drawing.Point(0, 0);
            this._updateHistoryLabel.Name = "_updateHistoryLabel";
            this._updateHistoryLabel.Size = new System.Drawing.Size(83, 13);
            this._updateHistoryLabel.TabIndex = 5;
            this._updateHistoryLabel.Text = "Update History:";
            // 
            // customNotesTextBox
            // 
            this.customNotesTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.customNotesTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this.customNotesTextBox.Location = new System.Drawing.Point(30, 139);
            this.customNotesTextBox.MaxLength = 1024;
            this.customNotesTextBox.Name = "customNotesTextBox";
            this.customNotesTextBox.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.Vertical;
            this.customNotesTextBox.Size = new System.Drawing.Size(693, 61);
            this.customNotesTextBox.TabIndex = 4;
            this.customNotesTextBox.Text = "";
            this.customNotesTextBox.TextChanged += new System.EventHandler(this.customNotesTextBox_TextChanged);
            // 
            // customNoteRadioButton
            // 
            this.customNoteRadioButton.AutoSize = true;
            this.customNoteRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.customNoteRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this.customNoteRadioButton.Location = new System.Drawing.Point(12, 115);
            this.customNoteRadioButton.Name = "customNoteRadioButton";
            this.customNoteRadioButton.Size = new System.Drawing.Size(92, 18);
            this.customNoteRadioButton.TabIndex = 3;
            this.customNoteRadioButton.TabStop = true;
            this.customNoteRadioButton.Text = "Custom note";
            this.customNoteRadioButton.UseVisualStyleBackColor = true;
            this.customNoteRadioButton.Click += new System.EventHandler(this.customNoteRadioButton_Click);
            // 
            // noActionRadioButton
            // 
            this.noActionRadioButton.AutoSize = true;
            this.noActionRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.noActionRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this.noActionRadioButton.Location = new System.Drawing.Point(12, 91);
            this.noActionRadioButton.Name = "noActionRadioButton";
            this.noActionRadioButton.Size = new System.Drawing.Size(172, 18);
            this.noActionRadioButton.TabIndex = 2;
            this.noActionRadioButton.TabStop = true;
            this.noActionRadioButton.Text = "Alert does not require action.";
            this.noActionRadioButton.UseVisualStyleBackColor = true;
            // 
            // duplicateAlertRadioButton
            // 
            this.duplicateAlertRadioButton.AutoSize = true;
            this.duplicateAlertRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.duplicateAlertRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this.duplicateAlertRadioButton.Location = new System.Drawing.Point(12, 67);
            this.duplicateAlertRadioButton.Name = "duplicateAlertRadioButton";
            this.duplicateAlertRadioButton.Size = new System.Drawing.Size(144, 18);
            this.duplicateAlertRadioButton.TabIndex = 2;
            this.duplicateAlertRadioButton.TabStop = true;
            this.duplicateAlertRadioButton.Text = "Duplicate Alert, Ignore.";
            this.duplicateAlertRadioButton.UseVisualStyleBackColor = true;
            // 
            // falseAlertRadioButton
            // 
            this.falseAlertRadioButton.AutoSize = true;
            this.falseAlertRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.falseAlertRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this.falseAlertRadioButton.Location = new System.Drawing.Point(12, 43);
            this.falseAlertRadioButton.Name = "falseAlertRadioButton";
            this.falseAlertRadioButton.Size = new System.Drawing.Size(125, 18);
            this.falseAlertRadioButton.TabIndex = 1;
            this.falseAlertRadioButton.TabStop = true;
            this.falseAlertRadioButton.Text = "False Alert, Ignore.";
            this.falseAlertRadioButton.UseVisualStyleBackColor = true;
            // 
            // alertResolvedRadioButton
            // 
            this.alertResolvedRadioButton.AutoSize = true;
            this.alertResolvedRadioButton.Checked = true;
            this.alertResolvedRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.alertResolvedRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this.alertResolvedRadioButton.Location = new System.Drawing.Point(12, 19);
            this.alertResolvedRadioButton.Name = "alertResolvedRadioButton";
            this.alertResolvedRadioButton.Size = new System.Drawing.Size(111, 18);
            this.alertResolvedRadioButton.TabIndex = 0;
            this.alertResolvedRadioButton.TabStop = true;
            this.alertResolvedRadioButton.Text = " Issue resolved. ";
            this.alertResolvedRadioButton.UseVisualStyleBackColor = true;
            // 
            // AlarmNotesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionGroupBox1);
            this.Name = "AlarmNotesUserControl";
            this.Size = new System.Drawing.Size(742, 369);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this.existingNotesPanel.ResumeLayout(false);
            this.existingNotesPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox customNotesTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton customNoteRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton duplicateAlertRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton falseAlertRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton alertResolvedRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _updateHistoryLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionRichTextBox _updateHistoryTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel existingNotesPanel;
        private Framework.Controls.TrafodionRadioButton noActionRadioButton;
    }
}
