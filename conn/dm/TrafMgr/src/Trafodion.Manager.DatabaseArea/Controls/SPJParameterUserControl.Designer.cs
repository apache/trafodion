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
namespace Trafodion.Manager.DatabaseArea.Controls
{
    partial class SPJParameterUserControl
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
            this._headerPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._errorText = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._directionGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._outRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._inOutRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._inRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._sqlDataTypeComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._javaTypeTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._nameTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._javaDataTypeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._sqlDataTypeLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._nameLabel = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._dataTypePanelContainer = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._headerPanel.SuspendLayout();
            this._directionGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // _headerPanel
            // 
            this._headerPanel.Controls.Add(this._errorText);
            this._headerPanel.Controls.Add(this._directionGroupBox);
            this._headerPanel.Controls.Add(this._sqlDataTypeComboBox);
            this._headerPanel.Controls.Add(this._javaTypeTextBox);
            this._headerPanel.Controls.Add(this._nameTextBox);
            this._headerPanel.Controls.Add(this._javaDataTypeLabel);
            this._headerPanel.Controls.Add(this._sqlDataTypeLabel);
            this._headerPanel.Controls.Add(this._nameLabel);
            this._headerPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this._headerPanel.Location = new System.Drawing.Point(0, 0);
            this._headerPanel.Name = "_headerPanel";
            this._headerPanel.Size = new System.Drawing.Size(555, 129);
            this._headerPanel.TabIndex = 0;
            // 
            // _errorText
            // 
            this._errorText.AutoSize = true;
            this._errorText.Font = new System.Drawing.Font("Tahoma", 8F);
            this._errorText.ForeColor = System.Drawing.Color.Red;
            this._errorText.Location = new System.Drawing.Point(12, 2);
            this._errorText.Name = "_errorText";
            this._errorText.Size = new System.Drawing.Size(0, 14);
            this._errorText.TabIndex = 1;
            // 
            // _directionGroupBox
            // 
            this._directionGroupBox.Controls.Add(this._outRadioButton);
            this._directionGroupBox.Controls.Add(this._inOutRadioButton);
            this._directionGroupBox.Controls.Add(this._inRadioButton);
            this._directionGroupBox.Location = new System.Drawing.Point(280, 49);
            this._directionGroupBox.Name = "_directionGroupBox";
            this._directionGroupBox.Size = new System.Drawing.Size(160, 36);
            this._directionGroupBox.TabIndex = 6;
            this._directionGroupBox.TabStop = false;
            // 
            // _outRadioButton
            // 
            this._outRadioButton.AutoSize = true;
            this._outRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this._outRadioButton.Location = new System.Drawing.Point(107, 12);
            this._outRadioButton.Name = "_outRadioButton";
            this._outRadioButton.Size = new System.Drawing.Size(46, 18);
            this._outRadioButton.TabIndex = 4;
            this._outRadioButton.TabStop = true;
            this._outRadioButton.Text = "OUT";
            this._outRadioButton.UseVisualStyleBackColor = true;
            // 
            // _inOutRadioButton
            // 
            this._inOutRadioButton.AutoSize = true;
            this._inOutRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this._inOutRadioButton.Location = new System.Drawing.Point(47, 12);
            this._inOutRadioButton.Name = "_inOutRadioButton";
            this._inOutRadioButton.Size = new System.Drawing.Size(55, 18);
            this._inOutRadioButton.TabIndex = 3;
            this._inOutRadioButton.TabStop = true;
            this._inOutRadioButton.Text = "INOUT";
            this._inOutRadioButton.UseVisualStyleBackColor = true;
            this._inOutRadioButton.Click += new System.EventHandler(this._inOutRadioButton_Click);
            // 
            // _inRadioButton
            // 
            this._inRadioButton.AutoSize = true;
            this._inRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this._inRadioButton.Location = new System.Drawing.Point(6, 12);
            this._inRadioButton.Name = "_inRadioButton";
            this._inRadioButton.Size = new System.Drawing.Size(34, 18);
            this._inRadioButton.TabIndex = 2;
            this._inRadioButton.TabStop = true;
            this._inRadioButton.Text = "IN";
            this._inRadioButton.UseVisualStyleBackColor = true;
            // 
            // _sqlDataTypeComboBox
            // 
            this._sqlDataTypeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._sqlDataTypeComboBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._sqlDataTypeComboBox.FormattingEnabled = true;
            this._sqlDataTypeComboBox.Location = new System.Drawing.Point(99, 58);
            this._sqlDataTypeComboBox.Name = "_sqlDataTypeComboBox";
            this._sqlDataTypeComboBox.Size = new System.Drawing.Size(166, 22);
            this._sqlDataTypeComboBox.TabIndex = 1;
            this._sqlDataTypeComboBox.SelectedIndexChanged += new System.EventHandler(this._sqlDataTypeComboBox_SelectedIndexChanged);
            // 
            // _javaTypeTextBox
            // 
            this._javaTypeTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._javaTypeTextBox.Location = new System.Drawing.Point(99, 93);
            this._javaTypeTextBox.Name = "_javaTypeTextBox";
            this._javaTypeTextBox.ReadOnly = true;
            this._javaTypeTextBox.Size = new System.Drawing.Size(432, 20);
            this._javaTypeTextBox.TabIndex = 66;
            // 
            // _nameTextBox
            // 
            this._nameTextBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._nameTextBox.Location = new System.Drawing.Point(99, 23);
            this._nameTextBox.Name = "_nameTextBox";
            this._nameTextBox.Size = new System.Drawing.Size(432, 20);
            this._nameTextBox.TabIndex = 0;
            this._nameTextBox.TextChanged += new System.EventHandler(this._nameTextBox_TextChanged);
            // 
            // _javaDataTypeLabel
            // 
            this._javaDataTypeLabel.AutoSize = true;
            this._javaDataTypeLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._javaDataTypeLabel.Location = new System.Drawing.Point(12, 95);
            this._javaDataTypeLabel.Name = "_javaDataTypeLabel";
            this._javaDataTypeLabel.Size = new System.Drawing.Size(82, 14);
            this._javaDataTypeLabel.TabIndex = 0;
            this._javaDataTypeLabel.Text = "Java Data Type";
            // 
            // _sqlDataTypeLabel
            // 
            this._sqlDataTypeLabel.AutoSize = true;
            this._sqlDataTypeLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._sqlDataTypeLabel.Location = new System.Drawing.Point(14, 62);
            this._sqlDataTypeLabel.Name = "_sqlDataTypeLabel";
            this._sqlDataTypeLabel.Size = new System.Drawing.Size(80, 14);
            this._sqlDataTypeLabel.TabIndex = 0;
            this._sqlDataTypeLabel.Text = "SQL Data Type";
            // 
            // _nameLabel
            // 
            this._nameLabel.AutoSize = true;
            this._nameLabel.Font = new System.Drawing.Font("Tahoma", 8F);
            this._nameLabel.Location = new System.Drawing.Point(60, 26);
            this._nameLabel.Name = "_nameLabel";
            this._nameLabel.Size = new System.Drawing.Size(34, 14);
            this._nameLabel.TabIndex = 0;
            this._nameLabel.Text = "Name";
            // 
            // _dataTypePanelContainer
            // 
            this._dataTypePanelContainer.Dock = System.Windows.Forms.DockStyle.Fill;
            this._dataTypePanelContainer.Location = new System.Drawing.Point(0, 129);
            this._dataTypePanelContainer.Name = "_dataTypePanelContainer";
            this._dataTypePanelContainer.Size = new System.Drawing.Size(555, 118);
            this._dataTypePanelContainer.TabIndex = 0;
            // 
            // SPJParameterUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._dataTypePanelContainer);
            this.Controls.Add(this._headerPanel);
            this.Name = "SPJParameterUserControl";
            this.Size = new System.Drawing.Size(555, 247);
            this._headerPanel.ResumeLayout(false);
            this._headerPanel.PerformLayout();
            this._directionGroupBox.ResumeLayout(false);
            this._directionGroupBox.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _headerPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _dataTypePanelContainer;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _nameTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _sqlDataTypeLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _nameLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _directionGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _outRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _inOutRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _inRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _sqlDataTypeComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _javaTypeTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _javaDataTypeLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel _errorText;
    }
}
