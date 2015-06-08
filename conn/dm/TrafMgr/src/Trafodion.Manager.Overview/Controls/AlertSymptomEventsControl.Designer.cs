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
    partial class AlertSymptomEventsControl
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
            this._symptomsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._buttonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._closeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._alertGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._statusTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._severityTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._typeDescriptionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._statusLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._severityLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._descriptionTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._resourceTypeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._resourceNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._processNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._typeDescriptionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._componentNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._processIDTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._descriptionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._resourceTypeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._resourceNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._processNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._closeTimeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._lastUpdateTimeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._createTimeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._componentLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._processIDLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._closeTimeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._lastUpdateTimeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._createTimeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._buttonsPanel.SuspendLayout();
            this._alertGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _symptomsGroupBox
            // 
            this._symptomsGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._symptomsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._symptomsGroupBox.Location = new System.Drawing.Point(0, 186);
            this._symptomsGroupBox.Name = "_symptomsGroupBox";
            this._symptomsGroupBox.Size = new System.Drawing.Size(964, 342);
            this._symptomsGroupBox.TabIndex = 0;
            this._symptomsGroupBox.TabStop = false;
            this._symptomsGroupBox.Text = "Symptom Events";
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._closeButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 528);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(964, 34);
            this._buttonsPanel.TabIndex = 20;
            // 
            // _closeButton
            // 
            this._closeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._closeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._closeButton.Location = new System.Drawing.Point(886, 6);
            this._closeButton.Name = "_closeButton";
            this._closeButton.Size = new System.Drawing.Size(75, 23);
            this._closeButton.TabIndex = 1;
            this._closeButton.Text = "&Close";
            this._closeButton.UseVisualStyleBackColor = true;
            this._closeButton.Click += new System.EventHandler(this._closeButton_Click);
            // 
            // _alertGroupBox
            // 
            this._alertGroupBox.Controls.Add(this._statusTextBox);
            this._alertGroupBox.Controls.Add(this._severityTextBox);
            this._alertGroupBox.Controls.Add(this._typeDescriptionTextBox);
            this._alertGroupBox.Controls.Add(this._statusLabel);
            this._alertGroupBox.Controls.Add(this._severityLabel);
            this._alertGroupBox.Controls.Add(this._descriptionTextBox);
            this._alertGroupBox.Controls.Add(this._resourceTypeTextBox);
            this._alertGroupBox.Controls.Add(this._resourceNameTextBox);
            this._alertGroupBox.Controls.Add(this._processNameTextBox);
            this._alertGroupBox.Controls.Add(this._typeDescriptionLabel);
            this._alertGroupBox.Controls.Add(this._componentNameTextBox);
            this._alertGroupBox.Controls.Add(this._processIDTextBox);
            this._alertGroupBox.Controls.Add(this._descriptionLabel);
            this._alertGroupBox.Controls.Add(this._resourceTypeLabel);
            this._alertGroupBox.Controls.Add(this._resourceNameLabel);
            this._alertGroupBox.Controls.Add(this._processNameLabel);
            this._alertGroupBox.Controls.Add(this._closeTimeTextBox);
            this._alertGroupBox.Controls.Add(this._lastUpdateTimeTextBox);
            this._alertGroupBox.Controls.Add(this._createTimeTextBox);
            this._alertGroupBox.Controls.Add(this._componentLabel);
            this._alertGroupBox.Controls.Add(this._processIDLabel);
            this._alertGroupBox.Controls.Add(this._closeTimeLabel);
            this._alertGroupBox.Controls.Add(this._lastUpdateTimeLabel);
            this._alertGroupBox.Controls.Add(this._createTimeLabel);
            this._alertGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._alertGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alertGroupBox.Location = new System.Drawing.Point(0, 0);
            this._alertGroupBox.Name = "_alertGroupBox";
            this._alertGroupBox.Size = new System.Drawing.Size(964, 186);
            this._alertGroupBox.TabIndex = 1;
            this._alertGroupBox.TabStop = false;
            this._alertGroupBox.Text = "Alert Details";
            // 
            // _statusTextBox
            // 
            this._statusTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._statusTextBox.Location = new System.Drawing.Point(734, 21);
            this._statusTextBox.Name = "_statusTextBox";
            this._statusTextBox.ReadOnly = true;
            this._statusTextBox.Size = new System.Drawing.Size(211, 21);
            this._statusTextBox.TabIndex = 13;
            // 
            // _severityTextBox
            // 
            this._severityTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._severityTextBox.Location = new System.Drawing.Point(414, 24);
            this._severityTextBox.Name = "_severityTextBox";
            this._severityTextBox.ReadOnly = true;
            this._severityTextBox.Size = new System.Drawing.Size(192, 21);
            this._severityTextBox.TabIndex = 11;
            // 
            // _typeDescriptionTextBox
            // 
            this._typeDescriptionTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._typeDescriptionTextBox.Location = new System.Drawing.Point(155, 120);
            this._typeDescriptionTextBox.Name = "_typeDescriptionTextBox";
            this._typeDescriptionTextBox.ReadOnly = true;
            this._typeDescriptionTextBox.Size = new System.Drawing.Size(449, 21);
            this._typeDescriptionTextBox.TabIndex = 9;
            // 
            // _statusLabel
            // 
            this._statusLabel.AutoSize = true;
            this._statusLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._statusLabel.Location = new System.Drawing.Point(619, 24);
            this._statusLabel.Name = "_statusLabel";
            this._statusLabel.Size = new System.Drawing.Size(38, 13);
            this._statusLabel.TabIndex = 12;
            this._statusLabel.Text = "Status";
            // 
            // _severityLabel
            // 
            this._severityLabel.AutoSize = true;
            this._severityLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._severityLabel.Location = new System.Drawing.Point(325, 28);
            this._severityLabel.Name = "_severityLabel";
            this._severityLabel.Size = new System.Drawing.Size(47, 13);
            this._severityLabel.TabIndex = 10;
            this._severityLabel.Text = "Severity";
            // 
            // _descriptionTextBox
            // 
            this._descriptionTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._descriptionTextBox.Location = new System.Drawing.Point(155, 152);
            this._descriptionTextBox.Name = "_descriptionTextBox";
            this._descriptionTextBox.ReadOnly = true;
            this._descriptionTextBox.Size = new System.Drawing.Size(790, 21);
            this._descriptionTextBox.TabIndex = 7;
            // 
            // _resourceTypeTextBox
            // 
            this._resourceTypeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._resourceTypeTextBox.Location = new System.Drawing.Point(415, 87);
            this._resourceTypeTextBox.Name = "_resourceTypeTextBox";
            this._resourceTypeTextBox.ReadOnly = true;
            this._resourceTypeTextBox.Size = new System.Drawing.Size(191, 21);
            this._resourceTypeTextBox.TabIndex = 5;
            // 
            // _resourceNameTextBox
            // 
            this._resourceNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._resourceNameTextBox.Location = new System.Drawing.Point(415, 56);
            this._resourceNameTextBox.Name = "_resourceNameTextBox";
            this._resourceNameTextBox.ReadOnly = true;
            this._resourceNameTextBox.Size = new System.Drawing.Size(191, 21);
            this._resourceNameTextBox.TabIndex = 5;
            // 
            // _processNameTextBox
            // 
            this._processNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._processNameTextBox.Location = new System.Drawing.Point(155, 88);
            this._processNameTextBox.Name = "_processNameTextBox";
            this._processNameTextBox.ReadOnly = true;
            this._processNameTextBox.Size = new System.Drawing.Size(157, 21);
            this._processNameTextBox.TabIndex = 5;
            // 
            // _typeDescriptionLabel
            // 
            this._typeDescriptionLabel.AutoSize = true;
            this._typeDescriptionLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._typeDescriptionLabel.Location = new System.Drawing.Point(10, 121);
            this._typeDescriptionLabel.Name = "_typeDescriptionLabel";
            this._typeDescriptionLabel.Size = new System.Drawing.Size(128, 13);
            this._typeDescriptionLabel.TabIndex = 8;
            this._typeDescriptionLabel.Text = "Problem Type Description";
            // 
            // _componentNameTextBox
            // 
            this._componentNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._componentNameTextBox.Location = new System.Drawing.Point(155, 24);
            this._componentNameTextBox.Name = "_componentNameTextBox";
            this._componentNameTextBox.ReadOnly = true;
            this._componentNameTextBox.Size = new System.Drawing.Size(157, 21);
            this._componentNameTextBox.TabIndex = 1;
            // 
            // _processIDTextBox
            // 
            this._processIDTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._processIDTextBox.Location = new System.Drawing.Point(155, 56);
            this._processIDTextBox.Name = "_processIDTextBox";
            this._processIDTextBox.ReadOnly = true;
            this._processIDTextBox.Size = new System.Drawing.Size(157, 21);
            this._processIDTextBox.TabIndex = 3;
            // 
            // _descriptionLabel
            // 
            this._descriptionLabel.AutoSize = true;
            this._descriptionLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._descriptionLabel.Location = new System.Drawing.Point(10, 152);
            this._descriptionLabel.Name = "_descriptionLabel";
            this._descriptionLabel.Size = new System.Drawing.Size(101, 13);
            this._descriptionLabel.TabIndex = 6;
            this._descriptionLabel.Text = "Problem Description";
            // 
            // _resourceTypeLabel
            // 
            this._resourceTypeLabel.AutoSize = true;
            this._resourceTypeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._resourceTypeLabel.Location = new System.Drawing.Point(325, 90);
            this._resourceTypeLabel.Name = "_resourceTypeLabel";
            this._resourceTypeLabel.Size = new System.Drawing.Size(79, 13);
            this._resourceTypeLabel.TabIndex = 4;
            this._resourceTypeLabel.Text = "Resource Type";
            // 
            // _resourceNameLabel
            // 
            this._resourceNameLabel.AutoSize = true;
            this._resourceNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._resourceNameLabel.Location = new System.Drawing.Point(325, 59);
            this._resourceNameLabel.Name = "_resourceNameLabel";
            this._resourceNameLabel.Size = new System.Drawing.Size(82, 13);
            this._resourceNameLabel.TabIndex = 4;
            this._resourceNameLabel.Text = "Resource Name";
            // 
            // _processNameLabel
            // 
            this._processNameLabel.AutoSize = true;
            this._processNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._processNameLabel.Location = new System.Drawing.Point(10, 90);
            this._processNameLabel.Name = "_processNameLabel";
            this._processNameLabel.Size = new System.Drawing.Size(115, 13);
            this._processNameLabel.TabIndex = 4;
            this._processNameLabel.Text = "Problem Process Name";
            // 
            // _closeTimeTextBox
            // 
            this._closeTimeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._closeTimeTextBox.Location = new System.Drawing.Point(734, 120);
            this._closeTimeTextBox.Name = "_closeTimeTextBox";
            this._closeTimeTextBox.ReadOnly = true;
            this._closeTimeTextBox.Size = new System.Drawing.Size(211, 21);
            this._closeTimeTextBox.TabIndex = 19;
            // 
            // _lastUpdateTimeTextBox
            // 
            this._lastUpdateTimeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._lastUpdateTimeTextBox.Location = new System.Drawing.Point(734, 88);
            this._lastUpdateTimeTextBox.Name = "_lastUpdateTimeTextBox";
            this._lastUpdateTimeTextBox.ReadOnly = true;
            this._lastUpdateTimeTextBox.Size = new System.Drawing.Size(211, 21);
            this._lastUpdateTimeTextBox.TabIndex = 17;
            // 
            // _createTimeTextBox
            // 
            this._createTimeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createTimeTextBox.Location = new System.Drawing.Point(734, 56);
            this._createTimeTextBox.Name = "_createTimeTextBox";
            this._createTimeTextBox.ReadOnly = true;
            this._createTimeTextBox.Size = new System.Drawing.Size(211, 21);
            this._createTimeTextBox.TabIndex = 15;
            // 
            // _componentLabel
            // 
            this._componentLabel.AutoSize = true;
            this._componentLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._componentLabel.Location = new System.Drawing.Point(10, 28);
            this._componentLabel.Name = "_componentLabel";
            this._componentLabel.Size = new System.Drawing.Size(133, 13);
            this._componentLabel.TabIndex = 0;
            this._componentLabel.Text = "Problem Component Name";
            // 
            // _processIDLabel
            // 
            this._processIDLabel.AutoSize = true;
            this._processIDLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._processIDLabel.Location = new System.Drawing.Point(10, 59);
            this._processIDLabel.Name = "_processIDLabel";
            this._processIDLabel.Size = new System.Drawing.Size(99, 13);
            this._processIDLabel.TabIndex = 2;
            this._processIDLabel.Text = "Problem Process ID";
            // 
            // _closeTimeLabel
            // 
            this._closeTimeLabel.AutoSize = true;
            this._closeTimeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._closeTimeLabel.Location = new System.Drawing.Point(619, 121);
            this._closeTimeLabel.Name = "_closeTimeLabel";
            this._closeTimeLabel.Size = new System.Drawing.Size(79, 13);
            this._closeTimeLabel.TabIndex = 18;
            this._closeTimeLabel.Text = "Close Time LCT";
            // 
            // _lastUpdateTimeLabel
            // 
            this._lastUpdateTimeLabel.AutoSize = true;
            this._lastUpdateTimeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._lastUpdateTimeLabel.Location = new System.Drawing.Point(619, 90);
            this._lastUpdateTimeLabel.Name = "_lastUpdateTimeLabel";
            this._lastUpdateTimeLabel.Size = new System.Drawing.Size(111, 13);
            this._lastUpdateTimeLabel.TabIndex = 16;
            this._lastUpdateTimeLabel.Text = "Last Update Time LCT";
            // 
            // _createTimeLabel
            // 
            this._createTimeLabel.AutoSize = true;
            this._createTimeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createTimeLabel.Location = new System.Drawing.Point(619, 58);
            this._createTimeLabel.Name = "_createTimeLabel";
            this._createTimeLabel.Size = new System.Drawing.Size(86, 13);
            this._createTimeLabel.TabIndex = 14;
            this._createTimeLabel.Text = "Create Time LCT";
            // 
            // AlertSymptomEventsControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._symptomsGroupBox);
            this.Controls.Add(this._buttonsPanel);
            this.Controls.Add(this._alertGroupBox);
            this.Name = "AlertSymptomEventsControl";
            this.Size = new System.Drawing.Size(964, 562);
            this._buttonsPanel.ResumeLayout(false);
            this._alertGroupBox.ResumeLayout(false);
            this._alertGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox _alertGroupBox;
        private Framework.Controls.TrafodionTextBox _statusTextBox;
        private Framework.Controls.TrafodionTextBox _severityTextBox;
        private Framework.Controls.TrafodionTextBox _typeDescriptionTextBox;
        private Framework.Controls.TrafodionLabel _statusLabel;
        private Framework.Controls.TrafodionLabel _severityLabel;
        private Framework.Controls.TrafodionTextBox _descriptionTextBox;
        private Framework.Controls.TrafodionTextBox _processNameTextBox;
        private Framework.Controls.TrafodionLabel _typeDescriptionLabel;
        private Framework.Controls.TrafodionTextBox _componentNameTextBox;
        private Framework.Controls.TrafodionTextBox _processIDTextBox;
        private Framework.Controls.TrafodionLabel _descriptionLabel;
        private Framework.Controls.TrafodionLabel _processNameLabel;
        private Framework.Controls.TrafodionTextBox _closeTimeTextBox;
        private Framework.Controls.TrafodionTextBox _lastUpdateTimeTextBox;
        private Framework.Controls.TrafodionTextBox _createTimeTextBox;
        private Framework.Controls.TrafodionLabel _componentLabel;
        private Framework.Controls.TrafodionLabel _processIDLabel;
        private Framework.Controls.TrafodionLabel _closeTimeLabel;
        private Framework.Controls.TrafodionLabel _lastUpdateTimeLabel;
        private Framework.Controls.TrafodionLabel _createTimeLabel;
        private Framework.Controls.TrafodionGroupBox _symptomsGroupBox;
        private Framework.Controls.TrafodionTextBox _resourceTypeTextBox;
        private Framework.Controls.TrafodionTextBox _resourceNameTextBox;
        private Framework.Controls.TrafodionLabel _resourceTypeLabel;
        private Framework.Controls.TrafodionLabel _resourceNameLabel;
        private Framework.Controls.TrafodionPanel _buttonsPanel;
        private Framework.Controls.TrafodionButton _closeButton;
    }
}
