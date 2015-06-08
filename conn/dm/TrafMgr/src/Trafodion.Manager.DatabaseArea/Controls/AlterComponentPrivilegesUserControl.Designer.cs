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
﻿namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class AlterComponentPrivilegesUserControl
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
            MyDispose(disposing);
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AlterComponentPrivilegesUserControl));
            this._usersGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._helpButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._applyButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._actionButtonsPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._resetGuiButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._splitContainer = new Trafodion.Manager.Framework.Controls.TrafodionSplitContainer();
            this._componentHeaderPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._componentComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._componentNameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._componentPrivilegesUserControl = new Trafodion.Manager.DatabaseArea.Controls.ComponentPrivilegesUserControl();
            this._actionButtonsPanel.SuspendLayout();
            this._splitContainer.Panel1.SuspendLayout();
            this._splitContainer.Panel2.SuspendLayout();
            this._splitContainer.SuspendLayout();
            this._componentHeaderPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // _usersGroupBox
            // 
            this._usersGroupBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._usersGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._usersGroupBox.Location = new System.Drawing.Point(0, 37);
            this._usersGroupBox.Name = "_usersGroupBox";
            this._usersGroupBox.Size = new System.Drawing.Size(923, 213);
            this._usersGroupBox.TabIndex = 0;
            this._usersGroupBox.TabStop = false;
            this._usersGroupBox.Text = "Grantees";
            // 
            // _helpButton
            // 
            this._helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._helpButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._helpButton.Location = new System.Drawing.Point(841, 6);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(75, 23);
            this._helpButton.TabIndex = 0;
            this._helpButton.Text = "He&lp";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // _applyButton
            // 
            this._applyButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._applyButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._applyButton.Location = new System.Drawing.Point(760, 6);
            this._applyButton.Name = "_applyButton";
            this._applyButton.Size = new System.Drawing.Size(75, 23);
            this._applyButton.TabIndex = 0;
            this._applyButton.Text = "&Apply";
            this._applyButton.UseVisualStyleBackColor = true;
            this._applyButton.Click += new System.EventHandler(this._applyButton_Click);
            // 
            // _actionButtonsPanel
            // 
            this._actionButtonsPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._actionButtonsPanel.Controls.Add(this._helpButton);
            this._actionButtonsPanel.Controls.Add(this._resetGuiButton);
            this._actionButtonsPanel.Controls.Add(this._applyButton);
            this._actionButtonsPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._actionButtonsPanel.Location = new System.Drawing.Point(0, 575);
            this._actionButtonsPanel.Name = "_actionButtonsPanel";
            this._actionButtonsPanel.Size = new System.Drawing.Size(923, 33);
            this._actionButtonsPanel.TabIndex = 4;
            // 
            // _resetGuiButton
            // 
            this._resetGuiButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._resetGuiButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._resetGuiButton.Location = new System.Drawing.Point(3, 6);
            this._resetGuiButton.Name = "_resetGuiButton";
            this._resetGuiButton.Size = new System.Drawing.Size(75, 23);
            this._resetGuiButton.TabIndex = 0;
            this._resetGuiButton.Text = "Re&set";
            this._resetGuiButton.UseVisualStyleBackColor = true;
            this._resetGuiButton.Click += new System.EventHandler(this._resetGuiButton_Click);
            // 
            // _splitContainer
            // 
            this._splitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._splitContainer.Location = new System.Drawing.Point(0, 0);
            this._splitContainer.Name = "_splitContainer";
            this._splitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // _splitContainer.Panel1
            // 
            this._splitContainer.Panel1.Controls.Add(this._componentHeaderPanel);
            this._splitContainer.Panel1.Controls.Add(this._usersGroupBox);
            // 
            // _splitContainer.Panel2
            // 
            this._splitContainer.Panel2.Controls.Add(this._componentPrivilegesUserControl);
            this._splitContainer.Size = new System.Drawing.Size(923, 575);
            this._splitContainer.SplitterDistance = 250;
            this._splitContainer.SplitterWidth = 9;
            this._splitContainer.TabIndex = 3;
            // 
            // _componentHeaderPanel
            // 
            this._componentHeaderPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._componentHeaderPanel.Controls.Add(this._componentComboBox);
            this._componentHeaderPanel.Controls.Add(this._componentNameLabel);
            this._componentHeaderPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._componentHeaderPanel.Location = new System.Drawing.Point(0, 0);
            this._componentHeaderPanel.Name = "_componentHeaderPanel";
            this._componentHeaderPanel.Size = new System.Drawing.Size(923, 39);
            this._componentHeaderPanel.TabIndex = 3;
            // 
            // _componentComboBox
            // 
            this._componentComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._componentComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._componentComboBox.FormattingEnabled = true;
            this._componentComboBox.Location = new System.Drawing.Point(87, 8);
            this._componentComboBox.Name = "_componentComboBox";
            this._componentComboBox.Size = new System.Drawing.Size(145, 21);
            this._componentComboBox.TabIndex = 0;
            // 
            // _componentNameLabel
            // 
            this._componentNameLabel.AutoSize = true;
            this._componentNameLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this._componentNameLabel.Location = new System.Drawing.Point(9, 11);
            this._componentNameLabel.Name = "_componentNameLabel";
            this._componentNameLabel.Size = new System.Drawing.Size(72, 13);
            this._componentNameLabel.TabIndex = 1;
            this._componentNameLabel.Text = "Component";
            // 
            // _componentPrivilegesUserControl
            // 
            this._componentPrivilegesUserControl.Component = null;
            this._componentPrivilegesUserControl.ConnectionDefinition = null;
            this._componentPrivilegesUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._componentPrivilegesUserControl.GrantedByDataSource = null;
            this._componentPrivilegesUserControl.GranteeName = null;
            this._componentPrivilegesUserControl.IsPublicGranteeType = false;
            this._componentPrivilegesUserControl.Location = new System.Drawing.Point(0, 0);
            this._componentPrivilegesUserControl.Name = "_componentPrivilegesUserControl";
            this._componentPrivilegesUserControl.Size = new System.Drawing.Size(923, 316);
            this._componentPrivilegesUserControl.TabIndex = 0;
            // 
            // AlterComponentPrivilegesUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._splitContainer);
            this.Controls.Add(this._actionButtonsPanel);
            this.Name = "AlterComponentPrivilegesUserControl";
            this.Size = new System.Drawing.Size(923, 608);
            this.Load += new System.EventHandler(this.AlterComponentPrivilegesUserControl_Load);
            this._actionButtonsPanel.ResumeLayout(false);
            this._splitContainer.Panel1.ResumeLayout(false);
            this._splitContainer.Panel2.ResumeLayout(false);
            this._splitContainer.ResumeLayout(false);
            this._componentHeaderPanel.ResumeLayout(false);
            this._componentHeaderPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionGroupBox _usersGroupBox;
        private Framework.Controls.TrafodionButton _helpButton;
        private Framework.Controls.TrafodionButton _applyButton;
        private Framework.Controls.TrafodionPanel _actionButtonsPanel;
        private Framework.Controls.TrafodionSplitContainer _splitContainer;
        private ComponentPrivilegesUserControl _componentPrivilegesUserControl;
        private Framework.Controls.TrafodionButton _resetGuiButton;
        private Framework.Controls.TrafodionPanel _componentHeaderPanel;
        private Framework.Controls.TrafodionComboBox _componentComboBox;
        private Framework.Controls.TrafodionLabel _componentNameLabel;
    }
}
