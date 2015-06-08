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
    partial class EventsSymptomDetailControl
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
            this._eventTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._severityTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._pnidTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._eventLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._severityLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._ipTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._threadTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._textTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._processNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._typeDescriptionLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._componentNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._processIDTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._ipLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._threadIDLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._resourceNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._processNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._hostIDTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._nodeIDTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._genTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
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
            this._symptomsGroupBox.Location = new System.Drawing.Point(0, 226);
            this._symptomsGroupBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._symptomsGroupBox.Name = "_symptomsGroupBox";
            this._symptomsGroupBox.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._symptomsGroupBox.Size = new System.Drawing.Size(1285, 383);
            this._symptomsGroupBox.TabIndex = 0;
            this._symptomsGroupBox.TabStop = false;
            this._symptomsGroupBox.Text = "Event Symptom Details";
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._closeButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 609);
            this._buttonsPanel.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(1285, 39);
            this._buttonsPanel.TabIndex = 20;
            // 
            // _closeButton
            // 
            this._closeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._closeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._closeButton.Location = new System.Drawing.Point(1181, 7);
            this._closeButton.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._closeButton.Name = "_closeButton";
            this._closeButton.Size = new System.Drawing.Size(100, 27);
            this._closeButton.TabIndex = 1;
            this._closeButton.Text = "&Close";
            this._closeButton.UseVisualStyleBackColor = true;
            this._closeButton.Click += new System.EventHandler(this._closeButton_Click);
            // 
            // _alertGroupBox
            // 
            this._alertGroupBox.Controls.Add(this._eventTextBox);
            this._alertGroupBox.Controls.Add(this._severityTextBox);
            this._alertGroupBox.Controls.Add(this._pnidTextBox);
            this._alertGroupBox.Controls.Add(this._eventLabel);
            this._alertGroupBox.Controls.Add(this._severityLabel);
            this._alertGroupBox.Controls.Add(this._ipTextBox);
            this._alertGroupBox.Controls.Add(this._threadTextBox);
            this._alertGroupBox.Controls.Add(this._textTextBox);
            this._alertGroupBox.Controls.Add(this._processNameTextBox);
            this._alertGroupBox.Controls.Add(this._typeDescriptionLabel);
            this._alertGroupBox.Controls.Add(this._componentNameTextBox);
            this._alertGroupBox.Controls.Add(this._processIDTextBox);
            this._alertGroupBox.Controls.Add(this._ipLabel);
            this._alertGroupBox.Controls.Add(this._threadIDLabel);
            this._alertGroupBox.Controls.Add(this._resourceNameLabel);
            this._alertGroupBox.Controls.Add(this._processNameLabel);
            this._alertGroupBox.Controls.Add(this._hostIDTextBox);
            this._alertGroupBox.Controls.Add(this._nodeIDTextBox);
            this._alertGroupBox.Controls.Add(this._genTextBox);
            this._alertGroupBox.Controls.Add(this._componentLabel);
            this._alertGroupBox.Controls.Add(this._processIDLabel);
            this._alertGroupBox.Controls.Add(this._closeTimeLabel);
            this._alertGroupBox.Controls.Add(this._lastUpdateTimeLabel);
            this._alertGroupBox.Controls.Add(this._createTimeLabel);
            this._alertGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._alertGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alertGroupBox.Location = new System.Drawing.Point(0, 0);
            this._alertGroupBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._alertGroupBox.Name = "_alertGroupBox";
            this._alertGroupBox.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._alertGroupBox.Size = new System.Drawing.Size(1285, 226);
            this._alertGroupBox.TabIndex = 1;
            this._alertGroupBox.TabStop = false;
            this._alertGroupBox.Text = "Event Symptom";
            // 
            // _eventTextBox
            // 
            this._eventTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._eventTextBox.Location = new System.Drawing.Point(979, 28);
            this._eventTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._eventTextBox.Name = "_eventTextBox";
            this._eventTextBox.ReadOnly = true;
            this._eventTextBox.Size = new System.Drawing.Size(274, 24);
            this._eventTextBox.TabIndex = 13;
            // 
            // _severityTextBox
            // 
            this._severityTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._severityTextBox.Location = new System.Drawing.Point(135, 63);
            this._severityTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._severityTextBox.Name = "_severityTextBox";
            this._severityTextBox.ReadOnly = true;
            this._severityTextBox.Size = new System.Drawing.Size(274, 24);
            this._severityTextBox.TabIndex = 11;
            // 
            // _pnidTextBox
            // 
            this._pnidTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._pnidTextBox.Location = new System.Drawing.Point(979, 98);
            this._pnidTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._pnidTextBox.Name = "_pnidTextBox";
            this._pnidTextBox.ReadOnly = true;
            this._pnidTextBox.Size = new System.Drawing.Size(274, 24);
            this._pnidTextBox.TabIndex = 9;
            // 
            // _eventLabel
            // 
            this._eventLabel.AutoSize = true;
            this._eventLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._eventLabel.Location = new System.Drawing.Point(866, 32);
            this._eventLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._eventLabel.Name = "_eventLabel";
            this._eventLabel.Size = new System.Drawing.Size(62, 17);
            this._eventLabel.TabIndex = 12;
            this._eventLabel.Text = "Event ID";
            // 
            // _severityLabel
            // 
            this._severityLabel.AutoSize = true;
            this._severityLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._severityLabel.Location = new System.Drawing.Point(13, 67);
            this._severityLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._severityLabel.Name = "_severityLabel";
            this._severityLabel.Size = new System.Drawing.Size(58, 17);
            this._severityLabel.TabIndex = 10;
            this._severityLabel.Text = "Severity";
            // 
            // _ipTextBox
            // 
            this._ipTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._ipTextBox.Location = new System.Drawing.Point(565, 133);
            this._ipTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._ipTextBox.Name = "_ipTextBox";
            this._ipTextBox.ReadOnly = true;
            this._ipTextBox.Size = new System.Drawing.Size(274, 24);
            this._ipTextBox.TabIndex = 7;
            // 
            // _threadTextBox
            // 
            this._threadTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._threadTextBox.Location = new System.Drawing.Point(135, 98);
            this._threadTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._threadTextBox.Name = "_threadTextBox";
            this._threadTextBox.ReadOnly = true;
            this._threadTextBox.Size = new System.Drawing.Size(274, 24);
            this._threadTextBox.TabIndex = 5;
            // 
            // _textTextBox
            // 
            this._textTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._textTextBox.Location = new System.Drawing.Point(135, 168);
            this._textTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._textTextBox.Multiline = true;
            this._textTextBox.Name = "_textTextBox";
            this._textTextBox.ReadOnly = true;
            this._textTextBox.Size = new System.Drawing.Size(1118, 45);
            this._textTextBox.TabIndex = 5;
            // 
            // _processNameTextBox
            // 
            this._processNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._processNameTextBox.Location = new System.Drawing.Point(565, 63);
            this._processNameTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._processNameTextBox.Name = "_processNameTextBox";
            this._processNameTextBox.ReadOnly = true;
            this._processNameTextBox.Size = new System.Drawing.Size(274, 24);
            this._processNameTextBox.TabIndex = 5;
            // 
            // _typeDescriptionLabel
            // 
            this._typeDescriptionLabel.AutoSize = true;
            this._typeDescriptionLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._typeDescriptionLabel.Location = new System.Drawing.Point(866, 102);
            this._typeDescriptionLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._typeDescriptionLabel.Name = "_typeDescriptionLabel";
            this._typeDescriptionLabel.Size = new System.Drawing.Size(57, 17);
            this._typeDescriptionLabel.TabIndex = 8;
            this._typeDescriptionLabel.Text = "PNID ID";
            // 
            // _componentNameTextBox
            // 
            this._componentNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._componentNameTextBox.Location = new System.Drawing.Point(565, 28);
            this._componentNameTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._componentNameTextBox.Name = "_componentNameTextBox";
            this._componentNameTextBox.ReadOnly = true;
            this._componentNameTextBox.Size = new System.Drawing.Size(274, 24);
            this._componentNameTextBox.TabIndex = 1;
            // 
            // _processIDTextBox
            // 
            this._processIDTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._processIDTextBox.Location = new System.Drawing.Point(979, 63);
            this._processIDTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._processIDTextBox.Name = "_processIDTextBox";
            this._processIDTextBox.ReadOnly = true;
            this._processIDTextBox.Size = new System.Drawing.Size(274, 24);
            this._processIDTextBox.TabIndex = 3;
            // 
            // _ipLabel
            // 
            this._ipLabel.AutoSize = true;
            this._ipLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._ipLabel.Location = new System.Drawing.Point(433, 137);
            this._ipLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._ipLabel.Name = "_ipLabel";
            this._ipLabel.Size = new System.Drawing.Size(90, 17);
            this._ipLabel.TabIndex = 6;
            this._ipLabel.Text = "IP Address ID";
            // 
            // _threadIDLabel
            // 
            this._threadIDLabel.AutoSize = true;
            this._threadIDLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._threadIDLabel.Location = new System.Drawing.Point(13, 102);
            this._threadIDLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._threadIDLabel.Name = "_threadIDLabel";
            this._threadIDLabel.Size = new System.Drawing.Size(69, 17);
            this._threadIDLabel.TabIndex = 4;
            this._threadIDLabel.Text = "Thread ID";
            // 
            // _resourceNameLabel
            // 
            this._resourceNameLabel.AutoSize = true;
            this._resourceNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._resourceNameLabel.Location = new System.Drawing.Point(13, 172);
            this._resourceNameLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._resourceNameLabel.Name = "_resourceNameLabel";
            this._resourceNameLabel.Size = new System.Drawing.Size(36, 17);
            this._resourceNameLabel.TabIndex = 4;
            this._resourceNameLabel.Text = "Text";
            // 
            // _processNameLabel
            // 
            this._processNameLabel.AutoSize = true;
            this._processNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._processNameLabel.Location = new System.Drawing.Point(433, 67);
            this._processNameLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._processNameLabel.Name = "_processNameLabel";
            this._processNameLabel.Size = new System.Drawing.Size(94, 17);
            this._processNameLabel.TabIndex = 4;
            this._processNameLabel.Text = "Process Name";
            // 
            // _hostIDTextBox
            // 
            this._hostIDTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._hostIDTextBox.Location = new System.Drawing.Point(135, 133);
            this._hostIDTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._hostIDTextBox.Name = "_hostIDTextBox";
            this._hostIDTextBox.ReadOnly = true;
            this._hostIDTextBox.Size = new System.Drawing.Size(274, 24);
            this._hostIDTextBox.TabIndex = 19;
            // 
            // _nodeIDTextBox
            // 
            this._nodeIDTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._nodeIDTextBox.Location = new System.Drawing.Point(565, 98);
            this._nodeIDTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._nodeIDTextBox.Name = "_nodeIDTextBox";
            this._nodeIDTextBox.ReadOnly = true;
            this._nodeIDTextBox.Size = new System.Drawing.Size(274, 24);
            this._nodeIDTextBox.TabIndex = 17;
            // 
            // _genTextBox
            // 
            this._genTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._genTextBox.Location = new System.Drawing.Point(135, 28);
            this._genTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._genTextBox.Name = "_genTextBox";
            this._genTextBox.ReadOnly = true;
            this._genTextBox.Size = new System.Drawing.Size(274, 24);
            this._genTextBox.TabIndex = 15;
            // 
            // _componentLabel
            // 
            this._componentLabel.AutoSize = true;
            this._componentLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._componentLabel.Location = new System.Drawing.Point(433, 32);
            this._componentLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._componentLabel.Name = "_componentLabel";
            this._componentLabel.Size = new System.Drawing.Size(120, 17);
            this._componentLabel.TabIndex = 0;
            this._componentLabel.Text = "Component Name";
            // 
            // _processIDLabel
            // 
            this._processIDLabel.AutoSize = true;
            this._processIDLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._processIDLabel.Location = new System.Drawing.Point(866, 67);
            this._processIDLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._processIDLabel.Name = "_processIDLabel";
            this._processIDLabel.Size = new System.Drawing.Size(73, 17);
            this._processIDLabel.TabIndex = 2;
            this._processIDLabel.Text = "Process ID";
            // 
            // _closeTimeLabel
            // 
            this._closeTimeLabel.AutoSize = true;
            this._closeTimeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._closeTimeLabel.Location = new System.Drawing.Point(13, 137);
            this._closeTimeLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._closeTimeLabel.Name = "_closeTimeLabel";
            this._closeTimeLabel.Size = new System.Drawing.Size(54, 17);
            this._closeTimeLabel.TabIndex = 18;
            this._closeTimeLabel.Text = "Host ID";
            // 
            // _lastUpdateTimeLabel
            // 
            this._lastUpdateTimeLabel.AutoSize = true;
            this._lastUpdateTimeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._lastUpdateTimeLabel.Location = new System.Drawing.Point(433, 102);
            this._lastUpdateTimeLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._lastUpdateTimeLabel.Name = "_lastUpdateTimeLabel";
            this._lastUpdateTimeLabel.Size = new System.Drawing.Size(58, 17);
            this._lastUpdateTimeLabel.TabIndex = 16;
            this._lastUpdateTimeLabel.Text = "Node ID";
            // 
            // _createTimeLabel
            // 
            this._createTimeLabel.AutoSize = true;
            this._createTimeLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._createTimeLabel.Location = new System.Drawing.Point(13, 32);
            this._createTimeLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._createTimeLabel.Name = "_createTimeLabel";
            this._createTimeLabel.Size = new System.Drawing.Size(93, 17);
            this._createTimeLabel.TabIndex = 14;
            this._createTimeLabel.Text = "Gen Time LCT";
            // 
            // EventsSymptomDetailControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._symptomsGroupBox);
            this.Controls.Add(this._buttonsPanel);
            this.Controls.Add(this._alertGroupBox);
            this.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this.Name = "EventsSymptomDetailControl";
            this.Size = new System.Drawing.Size(1285, 648);
            this._buttonsPanel.ResumeLayout(false);
            this._alertGroupBox.ResumeLayout(false);
            this._alertGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox _alertGroupBox;
        private Framework.Controls.TrafodionTextBox _eventTextBox;
        private Framework.Controls.TrafodionTextBox _severityTextBox;
        private Framework.Controls.TrafodionTextBox _pnidTextBox;
        private Framework.Controls.TrafodionLabel _eventLabel;
        private Framework.Controls.TrafodionLabel _severityLabel;
        private Framework.Controls.TrafodionTextBox _ipTextBox;
        private Framework.Controls.TrafodionTextBox _processNameTextBox;
        private Framework.Controls.TrafodionLabel _typeDescriptionLabel;
        private Framework.Controls.TrafodionTextBox _componentNameTextBox;
        private Framework.Controls.TrafodionTextBox _processIDTextBox;
        private Framework.Controls.TrafodionLabel _ipLabel;
        private Framework.Controls.TrafodionLabel _processNameLabel;
        private Framework.Controls.TrafodionTextBox _hostIDTextBox;
        private Framework.Controls.TrafodionTextBox _nodeIDTextBox;
        private Framework.Controls.TrafodionTextBox _genTextBox;
        private Framework.Controls.TrafodionLabel _componentLabel;
        private Framework.Controls.TrafodionLabel _processIDLabel;
        private Framework.Controls.TrafodionLabel _closeTimeLabel;
        private Framework.Controls.TrafodionLabel _lastUpdateTimeLabel;
        private Framework.Controls.TrafodionLabel _createTimeLabel;
        private Framework.Controls.TrafodionGroupBox _symptomsGroupBox;
        private Framework.Controls.TrafodionTextBox _threadTextBox;
        private Framework.Controls.TrafodionTextBox _textTextBox;
        private Framework.Controls.TrafodionLabel _threadIDLabel;
        private Framework.Controls.TrafodionLabel _resourceNameLabel;
        private Framework.Controls.TrafodionPanel _buttonsPanel;
        private Framework.Controls.TrafodionButton _closeButton;
    }
}
