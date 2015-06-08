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
﻿namespace Trafodion.Manager.WorkloadArea.Controls
{
    partial class ClientRuleValue
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
        //private void InitializeComponent()
        //{
        //    this.components = new System.ComponentModel.Container();
        //    this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        //    this.Text = "ClientRuleValue";
        //}

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ClientRuleValue));
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle2 = new System.Windows.Forms.DataGridViewCellStyle();
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle3 = new System.Windows.Forms.DataGridViewCellStyle();
            this.valueEntryTextBox = new System.Windows.Forms.TextBox();
            this.clientRuleValueFinishButton = new System.Windows.Forms.Button();
            this.clientRuleValueCancelButton = new System.Windows.Forms.Button();
            this.valueEntryPanel = new System.Windows.Forms.Panel();
            this.valueEntryLabel = new System.Windows.Forms.Label();
            this.comboSelectPanel = new System.Windows.Forms.Panel();
            this.comboSelectLabel = new System.Windows.Forms.Label();
            this.comboSelectComboBox = new System.Windows.Forms.ComboBox();
            this.clientRuleValueAddButton = new System.Windows.Forms.Button();
            this.dataGridView_Thresholds = new System.Windows.Forms.DataGridView();
            this.DeleteThreshold = new System.Windows.Forms.DataGridViewImageColumn();
            this.durationEntryPanel = new System.Windows.Forms.Panel();
            this.durationEntrySecondsLabel = new System.Windows.Forms.Label();
            this.durationEntryMinutesLabel = new System.Windows.Forms.Label();
            this.durationEntryHoursLabel = new System.Windows.Forms.Label();
            this.durationEntrySecondsTextBox = new System.Windows.Forms.TextBox();
            this.durationEntryMinutesTextBox = new System.Windows.Forms.TextBox();
            this.durationEntryHoursTextBox = new System.Windows.Forms.TextBox();
            this.durationEntryLabel = new System.Windows.Forms.Label();
            this.durationEntryMillisecondsLabel = new System.Windows.Forms.Label();
            this.durationEntryMillisecondsTextBox = new System.Windows.Forms.TextBox();
            this.dataGridViewTextBoxColumn1 = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ThresholdValue = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.valueEntryPanel.SuspendLayout();
            this.comboSelectPanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView_Thresholds)).BeginInit();
            this.durationEntryPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // valueEntryTextBox
            // 
            this.valueEntryTextBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.valueEntryTextBox.Location = new System.Drawing.Point(90, 10);
            this.valueEntryTextBox.Name = "valueEntryTextBox";
            this.valueEntryTextBox.Size = new System.Drawing.Size(190, 20);
            this.valueEntryTextBox.TabIndex = 32;
            this.valueEntryTextBox.TextChanged += new System.EventHandler(this.valueEntryTextBox_TextChanged);
            // 
            // clientRuleValueFinishButton
            // 
            this.clientRuleValueFinishButton.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.clientRuleValueFinishButton.Image = ((System.Drawing.Image)(resources.GetObject("clientRuleValueFinishButton.Image")));
            this.clientRuleValueFinishButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.clientRuleValueFinishButton.Location = new System.Drawing.Point(290, 135);
            this.clientRuleValueFinishButton.Name = "clientRuleValueFinishButton";
            this.clientRuleValueFinishButton.Size = new System.Drawing.Size(90, 25);
            this.clientRuleValueFinishButton.TabIndex = 41;
            this.clientRuleValueFinishButton.Text = "Finish";
            this.clientRuleValueFinishButton.UseVisualStyleBackColor = true;
            this.clientRuleValueFinishButton.Click += new System.EventHandler(this.clientRuleValueFinishButton_Click);
            // 
            // clientRuleValueCancelButton
            // 
            this.clientRuleValueCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.clientRuleValueCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.clientRuleValueCancelButton.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.clientRuleValueCancelButton.Image = ((System.Drawing.Image)(resources.GetObject("clientRuleValueCancelButton.Image")));
            this.clientRuleValueCancelButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.clientRuleValueCancelButton.Location = new System.Drawing.Point(190, 135);
            this.clientRuleValueCancelButton.Name = "clientRuleValueCancelButton";
            this.clientRuleValueCancelButton.Size = new System.Drawing.Size(90, 25);
            this.clientRuleValueCancelButton.TabIndex = 42;
            this.clientRuleValueCancelButton.Text = "    Cancel";
            this.clientRuleValueCancelButton.UseVisualStyleBackColor = true;
            this.clientRuleValueCancelButton.Click += new System.EventHandler(this.clientRuleValueCancelButton_Click);
            // 
            // valueEntryPanel
            // 
            this.valueEntryPanel.Controls.Add(this.valueEntryTextBox);
            this.valueEntryPanel.Controls.Add(this.valueEntryLabel);
            this.valueEntryPanel.Location = new System.Drawing.Point(10, 10);
            this.valueEntryPanel.Name = "valueEntryPanel";
            this.valueEntryPanel.Size = new System.Drawing.Size(290, 38);
            this.valueEntryPanel.TabIndex = 31;
            // 
            // valueEntryLabel
            // 
            this.valueEntryLabel.AutoSize = true;
            this.valueEntryLabel.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.valueEntryLabel.Location = new System.Drawing.Point(10, 14);
            this.valueEntryLabel.Name = "valueEntryLabel";
            this.valueEntryLabel.Size = new System.Drawing.Size(70, 15);
            this.valueEntryLabel.TabIndex = 33;
            this.valueEntryLabel.Text = "Enter Value:";
            // 
            // comboSelectPanel
            // 
            this.comboSelectPanel.Controls.Add(this.comboSelectLabel);
            this.comboSelectPanel.Controls.Add(this.comboSelectComboBox);
            this.comboSelectPanel.Location = new System.Drawing.Point(10, 10);
            this.comboSelectPanel.Name = "comboSelectPanel";
            this.comboSelectPanel.Size = new System.Drawing.Size(290, 38);
            this.comboSelectPanel.TabIndex = 21;
            // 
            // comboSelectLabel
            // 
            this.comboSelectLabel.AutoSize = true;
            this.comboSelectLabel.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.comboSelectLabel.Location = new System.Drawing.Point(10, 14);
            this.comboSelectLabel.Name = "comboSelectLabel";
            this.comboSelectLabel.Size = new System.Drawing.Size(73, 15);
            this.comboSelectLabel.TabIndex = 23;
            this.comboSelectLabel.Text = "Select Value:";
            // 
            // comboSelectComboBox
            // 
            this.comboSelectComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboSelectComboBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.comboSelectComboBox.FormattingEnabled = true;
            this.comboSelectComboBox.Location = new System.Drawing.Point(90, 10);
            this.comboSelectComboBox.Name = "comboSelectComboBox";
            this.comboSelectComboBox.Size = new System.Drawing.Size(190, 24);
            this.comboSelectComboBox.TabIndex = 22;
            this.comboSelectComboBox.SelectedIndexChanged += new System.EventHandler(this.comboSelectComboBox_SelectedIndexChanged);
            // 
            // clientRuleValueAddButton
            // 
            this.clientRuleValueAddButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.clientRuleValueAddButton.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.clientRuleValueAddButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.clientRuleValueAddButton.Location = new System.Drawing.Point(306, 19);
            this.clientRuleValueAddButton.Name = "clientRuleValueAddButton";
            this.clientRuleValueAddButton.Size = new System.Drawing.Size(75, 25);
            this.clientRuleValueAddButton.TabIndex = 6;
            this.clientRuleValueAddButton.Text = "Add";
            this.clientRuleValueAddButton.UseVisualStyleBackColor = true;
            this.clientRuleValueAddButton.Click += new System.EventHandler(this.clientRuleValueAddButton_Click);
            // 
            // dataGridView_Thresholds
            // 
            this.dataGridView_Thresholds.AllowUserToAddRows = false;
            this.dataGridView_Thresholds.AllowUserToDeleteRows = false;
            this.dataGridView_Thresholds.AllowUserToResizeColumns = false;
            this.dataGridView_Thresholds.AllowUserToResizeRows = false;
            dataGridViewCellStyle1.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle1.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            dataGridViewCellStyle1.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle1.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle1.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle1.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.dataGridView_Thresholds.ColumnHeadersDefaultCellStyle = dataGridViewCellStyle1;
            this.dataGridView_Thresholds.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dataGridView_Thresholds.ColumnHeadersVisible = false;
            this.dataGridView_Thresholds.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.ThresholdValue,
            this.DeleteThreshold});
            dataGridViewCellStyle2.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle2.BackColor = System.Drawing.SystemColors.Window;
            dataGridViewCellStyle2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            dataGridViewCellStyle2.ForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle2.SelectionBackColor = System.Drawing.SystemColors.Window;
            dataGridViewCellStyle2.SelectionForeColor = System.Drawing.SystemColors.ControlText;
            dataGridViewCellStyle2.WrapMode = System.Windows.Forms.DataGridViewTriState.False;
            this.dataGridView_Thresholds.DefaultCellStyle = dataGridViewCellStyle2;
            this.dataGridView_Thresholds.Location = new System.Drawing.Point(10, 50);
            this.dataGridView_Thresholds.MultiSelect = false;
            this.dataGridView_Thresholds.Name = "dataGridView_Thresholds";
            this.dataGridView_Thresholds.ReadOnly = true;
            dataGridViewCellStyle3.Alignment = System.Windows.Forms.DataGridViewContentAlignment.MiddleLeft;
            dataGridViewCellStyle3.BackColor = System.Drawing.SystemColors.Control;
            dataGridViewCellStyle3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            dataGridViewCellStyle3.ForeColor = System.Drawing.SystemColors.WindowText;
            dataGridViewCellStyle3.SelectionBackColor = System.Drawing.SystemColors.Highlight;
            dataGridViewCellStyle3.SelectionForeColor = System.Drawing.SystemColors.HighlightText;
            dataGridViewCellStyle3.WrapMode = System.Windows.Forms.DataGridViewTriState.True;
            this.dataGridView_Thresholds.RowHeadersDefaultCellStyle = dataGridViewCellStyle3;
            this.dataGridView_Thresholds.RowHeadersVisible = false;
            this.dataGridView_Thresholds.RowTemplate.Height = 24;
            this.dataGridView_Thresholds.RowTemplate.ReadOnly = true;
            this.dataGridView_Thresholds.SelectionMode = System.Windows.Forms.DataGridViewSelectionMode.CellSelect;
            this.dataGridView_Thresholds.Size = new System.Drawing.Size(370, 79);
            this.dataGridView_Thresholds.TabIndex = 11;
            this.dataGridView_Thresholds.CellClick += new System.Windows.Forms.DataGridViewCellEventHandler(this.dataGridView_Thresholds_CellClick);
            // 
            // DeleteThreshold
            // 
            this.DeleteThreshold.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.None;
            this.DeleteThreshold.HeaderText = "DeleteThreshold";
            this.DeleteThreshold.Image = ((System.Drawing.Image)(resources.GetObject("DeleteThreshold.Image")));
            this.DeleteThreshold.MinimumWidth = 24;
            this.DeleteThreshold.Name = "DeleteThreshold";
            this.DeleteThreshold.ReadOnly = true;
            this.DeleteThreshold.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.DeleteThreshold.ToolTipText = "Delete Value";
            this.DeleteThreshold.Width = 24;
            // 
            // durationEntryPanel
            // 
            this.durationEntryPanel.Controls.Add(this.durationEntryMillisecondsTextBox);
            this.durationEntryPanel.Controls.Add(this.durationEntryMillisecondsLabel);
            this.durationEntryPanel.Controls.Add(this.durationEntrySecondsLabel);
            this.durationEntryPanel.Controls.Add(this.durationEntryMinutesLabel);
            this.durationEntryPanel.Controls.Add(this.durationEntryHoursLabel);
            this.durationEntryPanel.Controls.Add(this.durationEntrySecondsTextBox);
            this.durationEntryPanel.Controls.Add(this.durationEntryMinutesTextBox);
            this.durationEntryPanel.Controls.Add(this.durationEntryHoursTextBox);
            this.durationEntryPanel.Controls.Add(this.durationEntryLabel);
            this.durationEntryPanel.Location = new System.Drawing.Point(10, 10);
            this.durationEntryPanel.Name = "durationEntryPanel";
            this.durationEntryPanel.Size = new System.Drawing.Size(370, 119);
            this.durationEntryPanel.TabIndex = 1;
            // 
            // durationEntrySecondsLabel
            // 
            this.durationEntrySecondsLabel.AutoSize = true;
            this.durationEntrySecondsLabel.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationEntrySecondsLabel.Location = new System.Drawing.Point(224, 17);
            this.durationEntrySecondsLabel.Name = "durationEntrySecondsLabel";
            this.durationEntrySecondsLabel.Size = new System.Drawing.Size(55, 15);
            this.durationEntrySecondsLabel.TabIndex = 8;
            this.durationEntrySecondsLabel.Text = "Seconds:";
            // 
            // durationEntryMinutesLabel
            // 
            this.durationEntryMinutesLabel.AutoSize = true;
            this.durationEntryMinutesLabel.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationEntryMinutesLabel.Location = new System.Drawing.Point(164, 17);
            this.durationEntryMinutesLabel.Name = "durationEntryMinutesLabel";
            this.durationEntryMinutesLabel.Size = new System.Drawing.Size(52, 15);
            this.durationEntryMinutesLabel.TabIndex = 7;
            this.durationEntryMinutesLabel.Text = "Minutes:";
            // 
            // durationEntryHoursLabel
            // 
            this.durationEntryHoursLabel.AutoSize = true;
            this.durationEntryHoursLabel.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationEntryHoursLabel.Location = new System.Drawing.Point(104, 17);
            this.durationEntryHoursLabel.Name = "durationEntryHoursLabel";
            this.durationEntryHoursLabel.Size = new System.Drawing.Size(43, 15);
            this.durationEntryHoursLabel.TabIndex = 6;
            this.durationEntryHoursLabel.Text = "Hours:";
            // 
            // durationEntrySecondsTextBox
            // 
            this.durationEntrySecondsTextBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationEntrySecondsTextBox.Location = new System.Drawing.Point(226, 39);
            this.durationEntrySecondsTextBox.Name = "durationEntrySecondsTextBox";
            this.durationEntrySecondsTextBox.Size = new System.Drawing.Size(55, 20);
            this.durationEntrySecondsTextBox.TabIndex = 4;
            this.durationEntrySecondsTextBox.Text = "0";
            this.durationEntrySecondsTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // durationEntryMinutesTextBox
            // 
            this.durationEntryMinutesTextBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationEntryMinutesTextBox.Location = new System.Drawing.Point(166, 39);
            this.durationEntryMinutesTextBox.Name = "durationEntryMinutesTextBox";
            this.durationEntryMinutesTextBox.Size = new System.Drawing.Size(55, 20);
            this.durationEntryMinutesTextBox.TabIndex = 3;
            this.durationEntryMinutesTextBox.Text = "0";
            this.durationEntryMinutesTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // durationEntryHoursTextBox
            // 
            this.durationEntryHoursTextBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationEntryHoursTextBox.Location = new System.Drawing.Point(106, 39);
            this.durationEntryHoursTextBox.Name = "durationEntryHoursTextBox";
            this.durationEntryHoursTextBox.Size = new System.Drawing.Size(55, 20);
            this.durationEntryHoursTextBox.TabIndex = 2;
            this.durationEntryHoursTextBox.Text = "0";
            this.durationEntryHoursTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // durationEntryLabel
            // 
            this.durationEntryLabel.AutoSize = true;
            this.durationEntryLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationEntryLabel.Location = new System.Drawing.Point(10, 6);
            this.durationEntryLabel.Name = "durationEntryLabel";
            this.durationEntryLabel.Size = new System.Drawing.Size(92, 13);
            this.durationEntryLabel.TabIndex = 5;
            this.durationEntryLabel.Text = "Enter Duration:";
            // 
            // durationEntryMillisecondsLabel
            // 
            this.durationEntryMillisecondsLabel.AutoSize = true;
            this.durationEntryMillisecondsLabel.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationEntryMillisecondsLabel.Location = new System.Drawing.Point(282, 17);
            this.durationEntryMillisecondsLabel.Name = "durationEntryMillisecondsLabel";
            this.durationEntryMillisecondsLabel.Size = new System.Drawing.Size(76, 15);
            this.durationEntryMillisecondsLabel.TabIndex = 9;
            this.durationEntryMillisecondsLabel.Text = "Milliseconds:";
            // 
            // durationEntryMillisecondsTextBox
            // 
            this.durationEntryMillisecondsTextBox.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.durationEntryMillisecondsTextBox.Location = new System.Drawing.Point(287, 39);
            this.durationEntryMillisecondsTextBox.Name = "durationEntryMillisecondsTextBox";
            this.durationEntryMillisecondsTextBox.Size = new System.Drawing.Size(55, 20);
            this.durationEntryMillisecondsTextBox.TabIndex = 10;
            this.durationEntryMillisecondsTextBox.Text = "0";
            this.durationEntryMillisecondsTextBox.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            // 
            // dataGridViewTextBoxColumn1
            // 
            this.dataGridViewTextBoxColumn1.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.dataGridViewTextBoxColumn1.HeaderText = "ThresholdName";
            this.dataGridViewTextBoxColumn1.Name = "dataGridViewTextBoxColumn1";
            // 
            // ThresholdValue
            // 
            this.ThresholdValue.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.ThresholdValue.HeaderText = "ThresholdName";
            this.ThresholdValue.Name = "ThresholdValue";
            this.ThresholdValue.ReadOnly = true;
            // 
            // ClientRuleValue
            // 
            this.AcceptButton = this.clientRuleValueAddButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.CancelButton = this.clientRuleValueCancelButton;
            this.ClientSize = new System.Drawing.Size(394, 168);
            this.Controls.Add(this.durationEntryPanel);
            this.Controls.Add(this.dataGridView_Thresholds);
            this.Controls.Add(this.comboSelectPanel);
            this.Controls.Add(this.valueEntryPanel);
            this.Controls.Add(this.clientRuleValueCancelButton);
            this.Controls.Add(this.clientRuleValueFinishButton);
            this.Controls.Add(this.clientRuleValueAddButton);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ClientRuleValue";
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "Trafodion Database Manager - Threshold Rule Value";
            this.GotFocus += new System.EventHandler(this.clientRuleValue_GotFocus);
            this.valueEntryPanel.ResumeLayout(false);
            this.valueEntryPanel.PerformLayout();
            this.comboSelectPanel.ResumeLayout(false);
            this.comboSelectPanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView_Thresholds)).EndInit();
            this.durationEntryPanel.ResumeLayout(false);
            this.durationEntryPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TextBox valueEntryTextBox;
        private System.Windows.Forms.Button clientRuleValueFinishButton;
        private System.Windows.Forms.Button clientRuleValueCancelButton;
        private System.Windows.Forms.Panel valueEntryPanel;
        private System.Windows.Forms.Label valueEntryLabel;
        private System.Windows.Forms.Panel comboSelectPanel;
        private System.Windows.Forms.Label comboSelectLabel;
        private System.Windows.Forms.ComboBox comboSelectComboBox;
        private System.Windows.Forms.Button clientRuleValueAddButton;
        private System.Windows.Forms.DataGridView dataGridView_Thresholds;
        private System.Windows.Forms.Panel durationEntryPanel;
        private System.Windows.Forms.Label durationEntryLabel;
        private System.Windows.Forms.Label durationEntrySecondsLabel;
        private System.Windows.Forms.Label durationEntryMinutesLabel;
        private System.Windows.Forms.Label durationEntryHoursLabel;
        private System.Windows.Forms.TextBox durationEntrySecondsTextBox;
        private System.Windows.Forms.TextBox durationEntryMinutesTextBox;
        private System.Windows.Forms.TextBox durationEntryHoursTextBox;
        private System.Windows.Forms.DataGridViewTextBoxColumn ThresholdValue;
        private System.Windows.Forms.DataGridViewImageColumn DeleteThreshold;
        private System.Windows.Forms.TextBox durationEntryMillisecondsTextBox;
        private System.Windows.Forms.Label durationEntryMillisecondsLabel;
        private System.Windows.Forms.DataGridViewTextBoxColumn dataGridViewTextBoxColumn1;
        
    }
}