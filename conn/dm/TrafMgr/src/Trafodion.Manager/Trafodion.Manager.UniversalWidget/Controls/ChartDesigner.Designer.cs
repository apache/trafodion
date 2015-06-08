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
﻿namespace Trafodion.Manager.UniversalWidget.Controls
{
    partial class ChartDesigner
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
            this._theMainPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theGraphListPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theGraphsList = new Trafodion.Manager.Framework.Controls.TrafodionListBox();
            this._theChartAttributePanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.oneGuiGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._theSeriesColumnDataTypeCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theSeriesColumnDataType = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theXAxisDataTypeCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theXAxisDataType = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theXAxisCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theXAxis = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theLegend = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theLegendText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theSeriesColumnCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theSeriesColumn = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theChartTypeCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theChartType = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theChartNameText = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this._theChartName = new Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel();
            this._theButtonPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this._theCancelButton = new System.Windows.Forms.Button();
            this._thePreviousButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theNextButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._removeButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theMainPanel.SuspendLayout();
            this._theGraphListPanel.SuspendLayout();
            this.oneGuiGroupBox1.SuspendLayout();
            this._theChartAttributePanel.SuspendLayout();
            this.oneGuiGroupBox2.SuspendLayout();
            this._theButtonPanel.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _theMainPanel
            // 
            this._theMainPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theMainPanel.Controls.Add(this._theGraphListPanel);
            this._theMainPanel.Controls.Add(this._theChartAttributePanel);
            this._theMainPanel.Controls.Add(this._theButtonPanel);
            this._theMainPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMainPanel.Location = new System.Drawing.Point(0, 0);
            this._theMainPanel.Name = "_theMainPanel";
            this._theMainPanel.Size = new System.Drawing.Size(391, 512);
            this._theMainPanel.TabIndex = 0;
            // 
            // _theGraphListPanel
            // 
            this._theGraphListPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theGraphListPanel.Controls.Add(this.oneGuiGroupBox1);
            this._theGraphListPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theGraphListPanel.Location = new System.Drawing.Point(0, 0);
            this._theGraphListPanel.Name = "_theGraphListPanel";
            this._theGraphListPanel.Size = new System.Drawing.Size(391, 196);
            this._theGraphListPanel.TabIndex = 0;
            // 
            // oneGuiGroupBox1
            // 
            this.oneGuiGroupBox1.Controls.Add(this._theGraphsList);
            this.oneGuiGroupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox1.Location = new System.Drawing.Point(0, 0);
            this.oneGuiGroupBox1.Name = "oneGuiGroupBox1";
            this.oneGuiGroupBox1.Size = new System.Drawing.Size(391, 196);
            this.oneGuiGroupBox1.TabIndex = 0;
            this.oneGuiGroupBox1.TabStop = false;
            this.oneGuiGroupBox1.Text = "Select New Graph or Choose Existing";
            // 
            // _theGraphsList
            // 
            this._theGraphsList.Dock = System.Windows.Forms.DockStyle.Top;
            this._theGraphsList.FormattingEnabled = true;
            this._theGraphsList.Location = new System.Drawing.Point(3, 17);
            this._theGraphsList.Name = "_theGraphsList";
            this._theGraphsList.Size = new System.Drawing.Size(385, 173);
            this._theGraphsList.TabIndex = 0;
            this._theGraphsList.SelectedValueChanged += new System.EventHandler(this._theGraphsList_SelectedValueChanged);
            // 
            // _theChartAttributePanel
            // 
            this._theChartAttributePanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theChartAttributePanel.Controls.Add(this.oneGuiGroupBox2);
            this._theChartAttributePanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theChartAttributePanel.Location = new System.Drawing.Point(0, 196);
            this._theChartAttributePanel.Name = "_theChartAttributePanel";
            this._theChartAttributePanel.Size = new System.Drawing.Size(391, 287);
            this._theChartAttributePanel.TabIndex = 1;
            // 
            // oneGuiGroupBox2
            // 
            this.oneGuiGroupBox2.Controls.Add(this._theSeriesColumnDataTypeCombo);
            this.oneGuiGroupBox2.Controls.Add(this._theSeriesColumnDataType);
            this.oneGuiGroupBox2.Controls.Add(this._theXAxisDataTypeCombo);
            this.oneGuiGroupBox2.Controls.Add(this._theXAxisDataType);
            this.oneGuiGroupBox2.Controls.Add(this._theXAxisCombo);
            this.oneGuiGroupBox2.Controls.Add(this._theXAxis);
            this.oneGuiGroupBox2.Controls.Add(this._theLegend);
            this.oneGuiGroupBox2.Controls.Add(this._theLegendText);
            this.oneGuiGroupBox2.Controls.Add(this._theSeriesColumnCombo);
            this.oneGuiGroupBox2.Controls.Add(this._theSeriesColumn);
            this.oneGuiGroupBox2.Controls.Add(this._theChartTypeCombo);
            this.oneGuiGroupBox2.Controls.Add(this._theChartType);
            this.oneGuiGroupBox2.Controls.Add(this._theChartNameText);
            this.oneGuiGroupBox2.Controls.Add(this._theChartName);
            this.oneGuiGroupBox2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.oneGuiGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox2.Location = new System.Drawing.Point(0, 0);
            this.oneGuiGroupBox2.Name = "oneGuiGroupBox2";
            this.oneGuiGroupBox2.Size = new System.Drawing.Size(391, 287);
            this.oneGuiGroupBox2.TabIndex = 0;
            this.oneGuiGroupBox2.TabStop = false;
            this.oneGuiGroupBox2.Text = "Chart Attributes";
            // 
            // _theSeriesColumnDataTypeCombo
            // 
            this._theSeriesColumnDataTypeCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSeriesColumnDataTypeCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSeriesColumnDataTypeCombo.FormattingEnabled = true;
            this._theSeriesColumnDataTypeCombo.Location = new System.Drawing.Point(139, 89);
            this._theSeriesColumnDataTypeCombo.Name = "_theSeriesColumnDataTypeCombo";
            this._theSeriesColumnDataTypeCombo.Size = new System.Drawing.Size(243, 21);
            this._theSeriesColumnDataTypeCombo.TabIndex = 14;
            // 
            // _theSeriesColumnDataType
            // 
            this._theSeriesColumnDataType.Location = new System.Drawing.Point(0, 92);
            this._theSeriesColumnDataType.Name = "_theSeriesColumnDataType";
            this._theSeriesColumnDataType.ShowRequired = true;
            this._theSeriesColumnDataType.Size = new System.Drawing.Size(133, 18);
            this._theSeriesColumnDataType.TabIndex = 13;
            // 
            // _theXAxisDataTypeCombo
            // 
            this._theXAxisDataTypeCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theXAxisDataTypeCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theXAxisDataTypeCombo.FormattingEnabled = true;
            this._theXAxisDataTypeCombo.Location = new System.Drawing.Point(139, 163);
            this._theXAxisDataTypeCombo.Name = "_theXAxisDataTypeCombo";
            this._theXAxisDataTypeCombo.Size = new System.Drawing.Size(243, 21);
            this._theXAxisDataTypeCombo.TabIndex = 12;
            // 
            // _theXAxisDataType
            // 
            this._theXAxisDataType.Location = new System.Drawing.Point(3, 166);
            this._theXAxisDataType.Name = "_theXAxisDataType";
            this._theXAxisDataType.ShowRequired = true;
            this._theXAxisDataType.Size = new System.Drawing.Size(130, 18);
            this._theXAxisDataType.TabIndex = 11;
            // 
            // _theXAxisCombo
            // 
            this._theXAxisCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theXAxisCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theXAxisCombo.FormattingEnabled = true;
            this._theXAxisCombo.Location = new System.Drawing.Point(139, 139);
            this._theXAxisCombo.Name = "_theXAxisCombo";
            this._theXAxisCombo.Size = new System.Drawing.Size(243, 21);
            this._theXAxisCombo.TabIndex = 10;
            // 
            // _theXAxis
            // 
            this._theXAxis.Location = new System.Drawing.Point(3, 142);
            this._theXAxis.Name = "_theXAxis";
            this._theXAxis.ShowRequired = true;
            this._theXAxis.Size = new System.Drawing.Size(130, 18);
            this._theXAxis.TabIndex = 9;
            // 
            // _theLegend
            // 
            this._theLegend.Location = new System.Drawing.Point(3, 118);
            this._theLegend.Name = "_theLegend";
            this._theLegend.ShowRequired = true;
            this._theLegend.Size = new System.Drawing.Size(130, 18);
            this._theLegend.TabIndex = 8;
            // 
            // _theLegendText
            // 
            this._theLegendText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theLegendText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theLegendText.Location = new System.Drawing.Point(139, 115);
            this._theLegendText.Name = "_theLegendText";
            this._theLegendText.Size = new System.Drawing.Size(243, 21);
            this._theLegendText.TabIndex = 7;
            // 
            // _theSeriesColumnCombo
            // 
            this._theSeriesColumnCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theSeriesColumnCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theSeriesColumnCombo.FormattingEnabled = true;
            this._theSeriesColumnCombo.Location = new System.Drawing.Point(139, 65);
            this._theSeriesColumnCombo.Name = "_theSeriesColumnCombo";
            this._theSeriesColumnCombo.Size = new System.Drawing.Size(243, 21);
            this._theSeriesColumnCombo.TabIndex = 5;
            // 
            // _theSeriesColumn
            // 
            this._theSeriesColumn.Location = new System.Drawing.Point(0, 68);
            this._theSeriesColumn.Name = "_theSeriesColumn";
            this._theSeriesColumn.ShowRequired = true;
            this._theSeriesColumn.Size = new System.Drawing.Size(133, 18);
            this._theSeriesColumn.TabIndex = 4;
            // 
            // _theChartTypeCombo
            // 
            this._theChartTypeCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theChartTypeCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theChartTypeCombo.FormattingEnabled = true;
            this._theChartTypeCombo.Location = new System.Drawing.Point(139, 41);
            this._theChartTypeCombo.Name = "_theChartTypeCombo";
            this._theChartTypeCombo.Size = new System.Drawing.Size(243, 21);
            this._theChartTypeCombo.TabIndex = 3;
            // 
            // _theChartType
            // 
            this._theChartType.Location = new System.Drawing.Point(6, 44);
            this._theChartType.Name = "_theChartType";
            this._theChartType.ShowRequired = true;
            this._theChartType.Size = new System.Drawing.Size(127, 18);
            this._theChartType.TabIndex = 2;
            // 
            // _theChartNameText
            // 
            this._theChartNameText.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theChartNameText.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theChartNameText.Location = new System.Drawing.Point(139, 17);
            this._theChartNameText.Name = "_theChartNameText";
            this._theChartNameText.Size = new System.Drawing.Size(243, 21);
            this._theChartNameText.TabIndex = 1;
            // 
            // _theChartName
            // 
            this._theChartName.Location = new System.Drawing.Point(6, 20);
            this._theChartName.Name = "_theChartName";
            this._theChartName.ShowRequired = true;
            this._theChartName.Size = new System.Drawing.Size(127, 18);
            this._theChartName.TabIndex = 0;
            // 
            // _theButtonPanel
            // 
            this._theButtonPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this._theButtonPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theButtonPanel.Controls.Add(this.flowLayoutPanel1);
            this._theButtonPanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theButtonPanel.Location = new System.Drawing.Point(0, 483);
            this._theButtonPanel.Name = "_theButtonPanel";
            this._theButtonPanel.Size = new System.Drawing.Size(391, 29);
            this._theButtonPanel.TabIndex = 2;
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.AutoSize = true;
            this.flowLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.flowLayoutPanel1.Controls.Add(this._theCancelButton);
            this.flowLayoutPanel1.Controls.Add(this._thePreviousButton);
            this.flowLayoutPanel1.Controls.Add(this._theNextButton);
            this.flowLayoutPanel1.Controls.Add(this._removeButton);
            this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.flowLayoutPanel1.FlowDirection = System.Windows.Forms.FlowDirection.RightToLeft;
            this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            this.flowLayoutPanel1.Size = new System.Drawing.Size(391, 29);
            this.flowLayoutPanel1.TabIndex = 0;
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Location = new System.Drawing.Point(313, 3);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(75, 23);
            this._theCancelButton.TabIndex = 0;
            this._theCancelButton.Text = "Cancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this._theCancelButton_Click);
            // 
            // _thePreviousButton
            // 
            this._thePreviousButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._thePreviousButton.Location = new System.Drawing.Point(232, 3);
            this._thePreviousButton.Name = "_thePreviousButton";
            this._thePreviousButton.Size = new System.Drawing.Size(75, 23);
            this._thePreviousButton.TabIndex = 2;
            this._thePreviousButton.Text = "<< Previous";
            this._thePreviousButton.UseVisualStyleBackColor = true;
            this._thePreviousButton.Visible = false;
            this._thePreviousButton.Click += new System.EventHandler(this._thePreviousButton_Click);
            // 
            // _theNextButton
            // 
            this._theNextButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theNextButton.Location = new System.Drawing.Point(151, 3);
            this._theNextButton.Name = "_theNextButton";
            this._theNextButton.Size = new System.Drawing.Size(75, 23);
            this._theNextButton.TabIndex = 1;
            this._theNextButton.Text = "Next>>";
            this._theNextButton.UseVisualStyleBackColor = true;
            this._theNextButton.Click += new System.EventHandler(this._theNextButton_Click);
            // 
            // _removeButton
            // 
            this._removeButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._removeButton.Location = new System.Drawing.Point(70, 3);
            this._removeButton.Name = "_removeButton";
            this._removeButton.Size = new System.Drawing.Size(75, 23);
            this._removeButton.TabIndex = 3;
            this._removeButton.Text = "&Remove";
            this._removeButton.UseVisualStyleBackColor = true;
            this._removeButton.Click += new System.EventHandler(this._removeButton_Click);
            // 
            // ChartDesigner
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.ClientSize = new System.Drawing.Size(391, 512);
            this.Controls.Add(this._theMainPanel);
            this.Name = "ChartDesigner";
            this._theMainPanel.ResumeLayout(false);
            this._theGraphListPanel.ResumeLayout(false);
            this.oneGuiGroupBox1.ResumeLayout(false);
            this._theChartAttributePanel.ResumeLayout(false);
            this.oneGuiGroupBox2.ResumeLayout(false);
            this.oneGuiGroupBox2.PerformLayout();
            this._theButtonPanel.ResumeLayout(false);
            this._theButtonPanel.PerformLayout();
            this.flowLayoutPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theMainPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theGraphListPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theChartAttributePanel;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _theButtonPanel;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.Button _theCancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _theNextButton;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _theChartTypeCombo;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theChartType;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theChartNameText;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theChartName;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theXAxis;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theLegend;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox _theLegendText;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _theSeriesColumnCombo;
        private Trafodion.Manager.Framework.Controls.TrafodionRequiredFieldLabel _theSeriesColumn;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _theXAxisCombo;
        private Trafodion.Manager.Framework.Controls.TrafodionListBox _theGraphsList;
        private Trafodion.Manager.Framework.Controls.TrafodionButton _thePreviousButton;
        private Framework.Controls.TrafodionButton _removeButton;
        private Framework.Controls.TrafodionComboBox _theSeriesColumnDataTypeCombo;
        private Framework.Controls.TrafodionRequiredFieldLabel _theSeriesColumnDataType;
        private Framework.Controls.TrafodionComboBox _theXAxisDataTypeCombo;
        private Framework.Controls.TrafodionRequiredFieldLabel _theXAxisDataType;
    }
}
