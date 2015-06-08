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
    partial class HealthSymptomDetailControl
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
            this._publicationLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._genTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._publicationTypeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._healthLCTTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._componentLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._eventLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._componentNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._logical2TextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._currentHealthTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._logicalObjectTypeNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._logicObjectPathTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._changeTimeLCTLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._intervalSecondsTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._qualifier2Label = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._curHealthNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._typeNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._objectPathLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._logical1TextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._previousHealthTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._logicalObjectNameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._intervalSecondsLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._qualifier1Label = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._preHealthNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._genLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._objectNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._buttonsPanel.SuspendLayout();
            this._alertGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _symptomsGroupBox
            // 
            this._symptomsGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._symptomsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._symptomsGroupBox.Location = new System.Drawing.Point(0, 174);
            this._symptomsGroupBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._symptomsGroupBox.Name = "_symptomsGroupBox";
            this._symptomsGroupBox.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._symptomsGroupBox.Size = new System.Drawing.Size(1346, 435);
            this._symptomsGroupBox.TabIndex = 0;
            this._symptomsGroupBox.TabStop = false;
            this._symptomsGroupBox.Text = "Health/State Symptom Details";
            // 
            // _buttonsPanel
            // 
            this._buttonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._buttonsPanel.Controls.Add(this._closeButton);
            this._buttonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._buttonsPanel.Location = new System.Drawing.Point(0, 609);
            this._buttonsPanel.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._buttonsPanel.Name = "_buttonsPanel";
            this._buttonsPanel.Size = new System.Drawing.Size(1346, 39);
            this._buttonsPanel.TabIndex = 20;
            // 
            // _closeButton
            // 
            this._closeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._closeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._closeButton.Location = new System.Drawing.Point(1242, 7);
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
            this._alertGroupBox.Controls.Add(this._publicationLabel);
            this._alertGroupBox.Controls.Add(this._genTextBox);
            this._alertGroupBox.Controls.Add(this._publicationTypeTextBox);
            this._alertGroupBox.Controls.Add(this._healthLCTTextBox);
            this._alertGroupBox.Controls.Add(this._componentLabel);
            this._alertGroupBox.Controls.Add(this._eventLabel);
            this._alertGroupBox.Controls.Add(this._componentNameTextBox);
            this._alertGroupBox.Controls.Add(this._logical2TextBox);
            this._alertGroupBox.Controls.Add(this._currentHealthTextBox);
            this._alertGroupBox.Controls.Add(this._logicalObjectTypeNameTextBox);
            this._alertGroupBox.Controls.Add(this._logicObjectPathTextBox);
            this._alertGroupBox.Controls.Add(this._changeTimeLCTLabel);
            this._alertGroupBox.Controls.Add(this._intervalSecondsTextBox);
            this._alertGroupBox.Controls.Add(this._qualifier2Label);
            this._alertGroupBox.Controls.Add(this._curHealthNameLabel);
            this._alertGroupBox.Controls.Add(this._typeNameLabel);
            this._alertGroupBox.Controls.Add(this._objectPathLabel);
            this._alertGroupBox.Controls.Add(this._logical1TextBox);
            this._alertGroupBox.Controls.Add(this._previousHealthTextBox);
            this._alertGroupBox.Controls.Add(this._logicalObjectNameTextBox);
            this._alertGroupBox.Controls.Add(this._intervalSecondsLabel);
            this._alertGroupBox.Controls.Add(this._qualifier1Label);
            this._alertGroupBox.Controls.Add(this._preHealthNameLabel);
            this._alertGroupBox.Controls.Add(this._genLabel);
            this._alertGroupBox.Controls.Add(this._objectNameLabel);
            this._alertGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._alertGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._alertGroupBox.Location = new System.Drawing.Point(0, 0);
            this._alertGroupBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._alertGroupBox.Name = "_alertGroupBox";
            this._alertGroupBox.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._alertGroupBox.Size = new System.Drawing.Size(1346, 174);
            this._alertGroupBox.TabIndex = 1;
            this._alertGroupBox.TabStop = false;
            this._alertGroupBox.Text = "Health/State Symptom";
            // 
            // _publicationLabel
            // 
            this._publicationLabel.AutoSize = true;
            this._publicationLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._publicationLabel.Location = new System.Drawing.Point(918, 32);
            this._publicationLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._publicationLabel.Name = "_publicationLabel";
            this._publicationLabel.Size = new System.Drawing.Size(108, 17);
            this._publicationLabel.TabIndex = 10;
            this._publicationLabel.Text = "Publication Type";
            // 
            // _genTextBox
            // 
            this._genTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._genTextBox.Location = new System.Drawing.Point(188, 28);
            this._genTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._genTextBox.Name = "_genTextBox";
            this._genTextBox.ReadOnly = true;
            this._genTextBox.Size = new System.Drawing.Size(236, 24);
            this._genTextBox.TabIndex = 13;
            // 
            // _publicationTypeTextBox
            // 
            this._publicationTypeTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._publicationTypeTextBox.Location = new System.Drawing.Point(1098, 28);
            this._publicationTypeTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._publicationTypeTextBox.Name = "_publicationTypeTextBox";
            this._publicationTypeTextBox.ReadOnly = true;
            this._publicationTypeTextBox.Size = new System.Drawing.Size(236, 24);
            this._publicationTypeTextBox.TabIndex = 11;
            // 
            // _healthLCTTextBox
            // 
            this._healthLCTTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._healthLCTTextBox.Location = new System.Drawing.Point(188, 136);
            this._healthLCTTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._healthLCTTextBox.Name = "_healthLCTTextBox";
            this._healthLCTTextBox.ReadOnly = true;
            this._healthLCTTextBox.Size = new System.Drawing.Size(236, 24);
            this._healthLCTTextBox.TabIndex = 9;
            // 
            // _componentLabel
            // 
            this._componentLabel.AutoSize = true;
            this._componentLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._componentLabel.Location = new System.Drawing.Point(459, 32);
            this._componentLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._componentLabel.Name = "_componentLabel";
            this._componentLabel.Size = new System.Drawing.Size(120, 17);
            this._componentLabel.TabIndex = 0;
            this._componentLabel.Text = "Component Name";
            // 
            // _eventLabel
            // 
            this._eventLabel.AutoSize = true;
            this._eventLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._eventLabel.Location = new System.Drawing.Point(825, 28);
            this._eventLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._eventLabel.Name = "_eventLabel";
            this._eventLabel.Size = new System.Drawing.Size(0, 17);
            this._eventLabel.TabIndex = 12;
            // 
            // _componentNameTextBox
            // 
            this._componentNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._componentNameTextBox.Location = new System.Drawing.Point(647, 28);
            this._componentNameTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._componentNameTextBox.Name = "_componentNameTextBox";
            this._componentNameTextBox.ReadOnly = true;
            this._componentNameTextBox.Size = new System.Drawing.Size(236, 24);
            this._componentNameTextBox.TabIndex = 1;
            // 
            // _logical2TextBox
            // 
            this._logical2TextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logical2TextBox.Location = new System.Drawing.Point(1098, 136);
            this._logical2TextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._logical2TextBox.Name = "_logical2TextBox";
            this._logical2TextBox.ReadOnly = true;
            this._logical2TextBox.Size = new System.Drawing.Size(236, 24);
            this._logical2TextBox.TabIndex = 7;
            // 
            // _currentHealthTextBox
            // 
            this._currentHealthTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._currentHealthTextBox.Location = new System.Drawing.Point(647, 100);
            this._currentHealthTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._currentHealthTextBox.Name = "_currentHealthTextBox";
            this._currentHealthTextBox.ReadOnly = true;
            this._currentHealthTextBox.Size = new System.Drawing.Size(236, 24);
            this._currentHealthTextBox.TabIndex = 5;
            // 
            // _logicalObjectTypeNameTextBox
            // 
            this._logicalObjectTypeNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logicalObjectTypeNameTextBox.Location = new System.Drawing.Point(647, 64);
            this._logicalObjectTypeNameTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._logicalObjectTypeNameTextBox.Name = "_logicalObjectTypeNameTextBox";
            this._logicalObjectTypeNameTextBox.ReadOnly = true;
            this._logicalObjectTypeNameTextBox.Size = new System.Drawing.Size(236, 24);
            this._logicalObjectTypeNameTextBox.TabIndex = 5;
            // 
            // _logicObjectPathTextBox
            // 
            this._logicObjectPathTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logicObjectPathTextBox.Location = new System.Drawing.Point(188, 100);
            this._logicObjectPathTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._logicObjectPathTextBox.Name = "_logicObjectPathTextBox";
            this._logicObjectPathTextBox.ReadOnly = true;
            this._logicObjectPathTextBox.Size = new System.Drawing.Size(236, 24);
            this._logicObjectPathTextBox.TabIndex = 5;
            // 
            // _changeTimeLCTLabel
            // 
            this._changeTimeLCTLabel.AutoSize = true;
            this._changeTimeLCTLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._changeTimeLCTLabel.Location = new System.Drawing.Point(11, 140);
            this._changeTimeLCTLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._changeTimeLCTLabel.Name = "_changeTimeLCTLabel";
            this._changeTimeLCTLabel.Size = new System.Drawing.Size(158, 17);
            this._changeTimeLCTLabel.TabIndex = 8;
            this._changeTimeLCTLabel.Text = "Health Change Time LCT";
            // 
            // _intervalSecondsTextBox
            // 
            this._intervalSecondsTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._intervalSecondsTextBox.Location = new System.Drawing.Point(188, 64);
            this._intervalSecondsTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._intervalSecondsTextBox.Name = "_intervalSecondsTextBox";
            this._intervalSecondsTextBox.ReadOnly = true;
            this._intervalSecondsTextBox.Size = new System.Drawing.Size(236, 24);
            this._intervalSecondsTextBox.TabIndex = 3;
            // 
            // _qualifier2Label
            // 
            this._qualifier2Label.AutoSize = true;
            this._qualifier2Label.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._qualifier2Label.Location = new System.Drawing.Point(918, 140);
            this._qualifier2Label.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._qualifier2Label.Name = "_qualifier2Label";
            this._qualifier2Label.Size = new System.Drawing.Size(157, 17);
            this._qualifier2Label.TabIndex = 6;
            this._qualifier2Label.Text = "Logical Object Qualifier 2";
            // 
            // _curHealthNameLabel
            // 
            this._curHealthNameLabel.AutoSize = true;
            this._curHealthNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._curHealthNameLabel.Location = new System.Drawing.Point(459, 104);
            this._curHealthNameLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._curHealthNameLabel.Name = "_curHealthNameLabel";
            this._curHealthNameLabel.Size = new System.Drawing.Size(136, 17);
            this._curHealthNameLabel.TabIndex = 4;
            this._curHealthNameLabel.Text = "Current Health Name";
            // 
            // _typeNameLabel
            // 
            this._typeNameLabel.AutoSize = true;
            this._typeNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._typeNameLabel.Location = new System.Drawing.Point(459, 68);
            this._typeNameLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._typeNameLabel.Name = "_typeNameLabel";
            this._typeNameLabel.Size = new System.Drawing.Size(168, 17);
            this._typeNameLabel.TabIndex = 4;
            this._typeNameLabel.Text = "Logical Object Type Name";
            // 
            // _objectPathLabel
            // 
            this._objectPathLabel.AutoSize = true;
            this._objectPathLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._objectPathLabel.Location = new System.Drawing.Point(11, 104);
            this._objectPathLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._objectPathLabel.Name = "_objectPathLabel";
            this._objectPathLabel.Size = new System.Drawing.Size(126, 17);
            this._objectPathLabel.TabIndex = 4;
            this._objectPathLabel.Text = "Logical Object Path";
            // 
            // _logical1TextBox
            // 
            this._logical1TextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logical1TextBox.Location = new System.Drawing.Point(647, 136);
            this._logical1TextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._logical1TextBox.Name = "_logical1TextBox";
            this._logical1TextBox.ReadOnly = true;
            this._logical1TextBox.Size = new System.Drawing.Size(236, 24);
            this._logical1TextBox.TabIndex = 19;
            // 
            // _previousHealthTextBox
            // 
            this._previousHealthTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._previousHealthTextBox.Location = new System.Drawing.Point(1098, 100);
            this._previousHealthTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._previousHealthTextBox.Name = "_previousHealthTextBox";
            this._previousHealthTextBox.ReadOnly = true;
            this._previousHealthTextBox.Size = new System.Drawing.Size(236, 24);
            this._previousHealthTextBox.TabIndex = 17;
            // 
            // _logicalObjectNameTextBox
            // 
            this._logicalObjectNameTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._logicalObjectNameTextBox.Location = new System.Drawing.Point(1098, 64);
            this._logicalObjectNameTextBox.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this._logicalObjectNameTextBox.Name = "_logicalObjectNameTextBox";
            this._logicalObjectNameTextBox.ReadOnly = true;
            this._logicalObjectNameTextBox.Size = new System.Drawing.Size(236, 24);
            this._logicalObjectNameTextBox.TabIndex = 15;
            // 
            // _intervalSecondsLabel
            // 
            this._intervalSecondsLabel.AutoSize = true;
            this._intervalSecondsLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._intervalSecondsLabel.Location = new System.Drawing.Point(11, 68);
            this._intervalSecondsLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._intervalSecondsLabel.Name = "_intervalSecondsLabel";
            this._intervalSecondsLabel.Size = new System.Drawing.Size(152, 17);
            this._intervalSecondsLabel.TabIndex = 2;
            this._intervalSecondsLabel.Text = "Check Interval Seconds";
            // 
            // _qualifier1Label
            // 
            this._qualifier1Label.AutoSize = true;
            this._qualifier1Label.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._qualifier1Label.Location = new System.Drawing.Point(459, 140);
            this._qualifier1Label.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._qualifier1Label.Name = "_qualifier1Label";
            this._qualifier1Label.Size = new System.Drawing.Size(157, 17);
            this._qualifier1Label.TabIndex = 18;
            this._qualifier1Label.Text = "Logical Object Qualifier 1";
            // 
            // _preHealthNameLabel
            // 
            this._preHealthNameLabel.AutoSize = true;
            this._preHealthNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._preHealthNameLabel.Location = new System.Drawing.Point(918, 104);
            this._preHealthNameLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._preHealthNameLabel.Name = "_preHealthNameLabel";
            this._preHealthNameLabel.Size = new System.Drawing.Size(141, 17);
            this._preHealthNameLabel.TabIndex = 16;
            this._preHealthNameLabel.Text = "Previous Health Name";
            // 
            // _genLabel
            // 
            this._genLabel.AutoSize = true;
            this._genLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._genLabel.Location = new System.Drawing.Point(11, 32);
            this._genLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._genLabel.Name = "_genLabel";
            this._genLabel.Size = new System.Drawing.Size(93, 17);
            this._genLabel.TabIndex = 14;
            this._genLabel.Text = "Gen Time LCT";
            // 
            // _objectNameLabel
            // 
            this._objectNameLabel.AutoSize = true;
            this._objectNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._objectNameLabel.Location = new System.Drawing.Point(918, 68);
            this._objectNameLabel.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
            this._objectNameLabel.Name = "_objectNameLabel";
            this._objectNameLabel.Size = new System.Drawing.Size(133, 17);
            this._objectNameLabel.TabIndex = 14;
            this._objectNameLabel.Text = "Logical Object Name";
            // 
            // HealthSymptomDetailControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._symptomsGroupBox);
            this.Controls.Add(this._buttonsPanel);
            this.Controls.Add(this._alertGroupBox);
            this.Margin = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this.Name = "HealthSymptomDetailControl";
            this.Size = new System.Drawing.Size(1346, 648);
            this._buttonsPanel.ResumeLayout(false);
            this._alertGroupBox.ResumeLayout(false);
            this._alertGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox _alertGroupBox;
        private Framework.Controls.TrafodionTextBox _genTextBox;
        private Framework.Controls.TrafodionTextBox _publicationTypeTextBox;
        private Framework.Controls.TrafodionTextBox _healthLCTTextBox;
        private Framework.Controls.TrafodionLabel _eventLabel;
        private Framework.Controls.TrafodionLabel _publicationLabel;
        private Framework.Controls.TrafodionTextBox _logical2TextBox;
        private Framework.Controls.TrafodionTextBox _logicObjectPathTextBox;
        private Framework.Controls.TrafodionLabel _changeTimeLCTLabel;
        private Framework.Controls.TrafodionTextBox _componentNameTextBox;
        private Framework.Controls.TrafodionTextBox _intervalSecondsTextBox;
        private Framework.Controls.TrafodionLabel _qualifier2Label;
        private Framework.Controls.TrafodionLabel _objectPathLabel;
        private Framework.Controls.TrafodionTextBox _logical1TextBox;
        private Framework.Controls.TrafodionTextBox _previousHealthTextBox;
        private Framework.Controls.TrafodionTextBox _logicalObjectNameTextBox;
        private Framework.Controls.TrafodionLabel _componentLabel;
        private Framework.Controls.TrafodionLabel _intervalSecondsLabel;
        private Framework.Controls.TrafodionLabel _qualifier1Label;
        private Framework.Controls.TrafodionLabel _preHealthNameLabel;
        private Framework.Controls.TrafodionLabel _objectNameLabel;
        private Framework.Controls.TrafodionGroupBox _symptomsGroupBox;
        private Framework.Controls.TrafodionTextBox _currentHealthTextBox;
        private Framework.Controls.TrafodionTextBox _logicalObjectTypeNameTextBox;
        private Framework.Controls.TrafodionLabel _curHealthNameLabel;
        private Framework.Controls.TrafodionLabel _typeNameLabel;
        private Framework.Controls.TrafodionPanel _buttonsPanel;
        private Framework.Controls.TrafodionButton _closeButton;
        private Framework.Controls.TrafodionLabel _genLabel;
    }
}
