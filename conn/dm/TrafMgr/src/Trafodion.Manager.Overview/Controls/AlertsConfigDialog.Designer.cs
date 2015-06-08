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
    partial class AlertsConfigDialog
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
            this.iGColHdrStyleDesign1 = new TenTec.Windows.iGridLib.iGColHdrStyleDesign();
            this._cancelButton = new System.Windows.Forms.Button();
            this._okButton = new System.Windows.Forms.Button();
            this._resetButton = new System.Windows.Forms.Button();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._helpButton = new System.Windows.Forms.Button();
            this._fetchOptionsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.TrafodionGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._openAlertsRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._allAlertsRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._theEventTimeGroupBox = new System.Windows.Forms.GroupBox();
            this._theMessagePanel = new Trafodion.Manager.Framework.Controls.TrafodionMessagePanel();
            this.TrafodionPanel2 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionLabel6 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theEndTime = new System.Windows.Forms.DateTimePicker();
            this._theStartTime = new System.Windows.Forms.DateTimePicker();
            this.TrafodionLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.TrafodionPanel3 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.TrafodionLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._theTimeRangeCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._theIncludeLiveFeedCheckbox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.TrafodionPanel1.SuspendLayout();
            this._fetchOptionsGroupBox.SuspendLayout();
            this.TrafodionGroupBox1.SuspendLayout();
            this._theEventTimeGroupBox.SuspendLayout();
            this.TrafodionPanel2.SuspendLayout();
            this.TrafodionPanel3.SuspendLayout();
            this.SuspendLayout();
            // 
            // iGColHdrStyleDesign1
            // 
            this.iGColHdrStyleDesign1.CustomDrawFlags = TenTec.Windows.iGridLib.iGCustomDrawFlags.None;
            this.iGColHdrStyleDesign1.Flags = ((TenTec.Windows.iGridLib.iGColHdrFlags)((TenTec.Windows.iGridLib.iGColHdrFlags.DisplayText | TenTec.Windows.iGridLib.iGColHdrFlags.DisplayImage)));
            this.iGColHdrStyleDesign1.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.MiddleLeft;
            this.iGColHdrStyleDesign1.SortInfoVisible = TenTec.Windows.iGridLib.iGBool.True;
            this.iGColHdrStyleDesign1.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGColHdrStyleDesign1.TextPosToImage = TenTec.Windows.iGridLib.iGTextPosToImage.Horizontally;
            this.iGColHdrStyleDesign1.TextTrimming = TenTec.Windows.iGridLib.iGStringTrimming.EllipsisCharacter;
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Location = new System.Drawing.Point(242, 5);
            this._cancelButton.Name = "_cancelButton";
            this._cancelButton.Size = new System.Drawing.Size(75, 23);
            this._cancelButton.TabIndex = 3;
            this._cancelButton.Text = "&Cancel";
            this._cancelButton.UseVisualStyleBackColor = true;
            this._cancelButton.Click += new System.EventHandler(this._cancelButton_Click);
            // 
            // _okButton
            // 
            this._okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._okButton.Location = new System.Drawing.Point(163, 5);
            this._okButton.Name = "_okButton";
            this._okButton.Size = new System.Drawing.Size(75, 23);
            this._okButton.TabIndex = 3;
            this._okButton.Text = "&OK";
            this._okButton.UseVisualStyleBackColor = true;
            this._okButton.Click += new System.EventHandler(this._okButton_Click);
            // 
            // _resetButton
            // 
            this._resetButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this._resetButton.Location = new System.Drawing.Point(5, 5);
            this._resetButton.Name = "_resetButton";
            this._resetButton.Size = new System.Drawing.Size(111, 23);
            this._resetButton.TabIndex = 3;
            this._resetButton.Text = "&Reset to Defaults";
            this._resetButton.UseVisualStyleBackColor = true;
            this._resetButton.Click += new System.EventHandler(this._resetButton_Click);
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._okButton);
            this.TrafodionPanel1.Controls.Add(this._resetButton);
            this.TrafodionPanel1.Controls.Add(this._helpButton);
            this.TrafodionPanel1.Controls.Add(this._cancelButton);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 255);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(400, 33);
            this.TrafodionPanel1.TabIndex = 4;
            // 
            // _helpButton
            // 
            this._helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._helpButton.Location = new System.Drawing.Point(321, 5);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(75, 23);
            this._helpButton.TabIndex = 3;
            this._helpButton.Text = "&Help";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // _fetchOptionsGroupBox
            // 
            this._fetchOptionsGroupBox.Controls.Add(this.TrafodionGroupBox1);
            this._fetchOptionsGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._fetchOptionsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._fetchOptionsGroupBox.Location = new System.Drawing.Point(0, 0);
            this._fetchOptionsGroupBox.Name = "_fetchOptionsGroupBox";
            this._fetchOptionsGroupBox.Size = new System.Drawing.Size(400, 64);
            this._fetchOptionsGroupBox.TabIndex = 6;
            this._fetchOptionsGroupBox.TabStop = false;
            this._fetchOptionsGroupBox.Text = "Fetch Options";
            // 
            // TrafodionGroupBox1
            // 
            this.TrafodionGroupBox1.Controls.Add(this._openAlertsRadioButton);
            this.TrafodionGroupBox1.Controls.Add(this._allAlertsRadioButton);
            this.TrafodionGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionGroupBox1.Location = new System.Drawing.Point(9, 15);
            this.TrafodionGroupBox1.Name = "TrafodionGroupBox1";
            this.TrafodionGroupBox1.Size = new System.Drawing.Size(249, 33);
            this.TrafodionGroupBox1.TabIndex = 4;
            this.TrafodionGroupBox1.TabStop = false;
            // 
            // _openAlertsRadioButton
            // 
            this._openAlertsRadioButton.AutoSize = true;
            this._openAlertsRadioButton.Checked = true;
            this._openAlertsRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._openAlertsRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this._openAlertsRadioButton.Location = new System.Drawing.Point(6, 9);
            this._openAlertsRadioButton.Name = "_openAlertsRadioButton";
            this._openAlertsRadioButton.Size = new System.Drawing.Size(113, 18);
            this._openAlertsRadioButton.TabIndex = 1;
            this._openAlertsRadioButton.TabStop = true;
            this._openAlertsRadioButton.Text = "Open Alerts Only";
            this._openAlertsRadioButton.UseVisualStyleBackColor = true;
            // 
            // _allAlertsRadioButton
            // 
            this._allAlertsRadioButton.AutoSize = true;
            this._allAlertsRadioButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._allAlertsRadioButton.Font = new System.Drawing.Font("Tahoma", 8F);
            this._allAlertsRadioButton.Location = new System.Drawing.Point(149, 10);
            this._allAlertsRadioButton.Name = "_allAlertsRadioButton";
            this._allAlertsRadioButton.Size = new System.Drawing.Size(73, 18);
            this._allAlertsRadioButton.TabIndex = 1;
            this._allAlertsRadioButton.Text = "All Alerts";
            this._allAlertsRadioButton.UseVisualStyleBackColor = true;
            // 
            // _theEventTimeGroupBox
            // 
            this._theEventTimeGroupBox.Controls.Add(this._theMessagePanel);
            this._theEventTimeGroupBox.Controls.Add(this.TrafodionPanel2);
            this._theEventTimeGroupBox.Controls.Add(this.TrafodionPanel3);
            this._theEventTimeGroupBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theEventTimeGroupBox.Location = new System.Drawing.Point(0, 64);
            this._theEventTimeGroupBox.Name = "_theEventTimeGroupBox";
            this._theEventTimeGroupBox.Size = new System.Drawing.Size(400, 191);
            this._theEventTimeGroupBox.TabIndex = 7;
            this._theEventTimeGroupBox.TabStop = false;
            this._theEventTimeGroupBox.Text = "Alert Time ( Server Local Time in 24-Hour format )";
            // 
            // _theMessagePanel
            // 
            this._theMessagePanel.Dock = System.Windows.Forms.DockStyle.Bottom;
            this._theMessagePanel.Location = new System.Drawing.Point(3, 166);
            this._theMessagePanel.Name = "_theMessagePanel";
            this._theMessagePanel.Size = new System.Drawing.Size(394, 22);
            this._theMessagePanel.TabIndex = 12;
            this._theMessagePanel.TabStop = false;
            // 
            // TrafodionPanel2
            // 
            this.TrafodionPanel2.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel6);
            this.TrafodionPanel2.Controls.Add(this._theEndTime);
            this.TrafodionPanel2.Controls.Add(this._theStartTime);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel3);
            this.TrafodionPanel2.Controls.Add(this.TrafodionLabel2);
            this.TrafodionPanel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel2.Location = new System.Drawing.Point(3, 92);
            this.TrafodionPanel2.Name = "TrafodionPanel2";
            this.TrafodionPanel2.Size = new System.Drawing.Size(394, 73);
            this.TrafodionPanel2.TabIndex = 10;
            // 
            // TrafodionLabel6
            // 
            this.TrafodionLabel6.AutoSize = true;
            this.TrafodionLabel6.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel6.Location = new System.Drawing.Point(4, -1);
            this.TrafodionLabel6.Name = "TrafodionLabel6";
            this.TrafodionLabel6.Size = new System.Drawing.Size(134, 13);
            this.TrafodionLabel6.TabIndex = 10;
            this.TrafodionLabel6.Text = "OR  Select a custom range";
            // 
            // _theEndTime
            // 
            this._theEndTime.Checked = false;
            this._theEndTime.CustomFormat = "yyyy-MM-dd HH:mm:ss tt";
            this._theEndTime.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
            this._theEndTime.Location = new System.Drawing.Point(109, 48);
            this._theEndTime.Name = "_theEndTime";
            this._theEndTime.ShowCheckBox = true;
            this._theEndTime.Size = new System.Drawing.Size(182, 20);
            this._theEndTime.TabIndex = 5;
            this._theEndTime.ValueChanged += new System.EventHandler(this._theEndTime_ValueChanged);
            this._theEndTime.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this._theEndTime_KeyPress);
            // 
            // _theStartTime
            // 
            this._theStartTime.CustomFormat = "yyyy-MM-dd HH:mm:ss tt";
            this._theStartTime.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
            this._theStartTime.Location = new System.Drawing.Point(109, 22);
            this._theStartTime.Name = "_theStartTime";
            this._theStartTime.ShowCheckBox = true;
            this._theStartTime.Size = new System.Drawing.Size(182, 20);
            this._theStartTime.TabIndex = 6;
            this._theStartTime.ValueChanged += new System.EventHandler(this._theStartTime_ValueChanged);
            this._theStartTime.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this._theStartTime_KeyPress);
            // 
            // TrafodionLabel3
            // 
            this.TrafodionLabel3.AutoSize = true;
            this.TrafodionLabel3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel3.Location = new System.Drawing.Point(12, 26);
            this.TrafodionLabel3.Name = "TrafodionLabel3";
            this.TrafodionLabel3.Size = new System.Drawing.Size(56, 13);
            this.TrafodionLabel3.TabIndex = 8;
            this.TrafodionLabel3.Text = "Start Time";
            // 
            // TrafodionLabel2
            // 
            this.TrafodionLabel2.AutoSize = true;
            this.TrafodionLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel2.Location = new System.Drawing.Point(12, 52);
            this.TrafodionLabel2.Name = "TrafodionLabel2";
            this.TrafodionLabel2.Size = new System.Drawing.Size(50, 13);
            this.TrafodionLabel2.TabIndex = 7;
            this.TrafodionLabel2.Text = "End Time";
            // 
            // TrafodionPanel3
            // 
            this.TrafodionPanel3.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel3.Controls.Add(this.TrafodionLabel4);
            this.TrafodionPanel3.Controls.Add(this._theTimeRangeCombo);
            this.TrafodionPanel3.Controls.Add(this._theIncludeLiveFeedCheckbox);
            this.TrafodionPanel3.Dock = System.Windows.Forms.DockStyle.Top;
            this.TrafodionPanel3.Location = new System.Drawing.Point(3, 16);
            this.TrafodionPanel3.Name = "TrafodionPanel3";
            this.TrafodionPanel3.Size = new System.Drawing.Size(394, 76);
            this.TrafodionPanel3.TabIndex = 11;
            // 
            // TrafodionLabel4
            // 
            this.TrafodionLabel4.AutoSize = true;
            this.TrafodionLabel4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.TrafodionLabel4.Location = new System.Drawing.Point(12, 21);
            this.TrafodionLabel4.Name = "TrafodionLabel4";
            this.TrafodionLabel4.Size = new System.Drawing.Size(63, 13);
            this.TrafodionLabel4.TabIndex = 1;
            this.TrafodionLabel4.Text = "Time Range";
            // 
            // _theTimeRangeCombo
            // 
            this._theTimeRangeCombo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this._theTimeRangeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this._theTimeRangeCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theTimeRangeCombo.FormattingEnabled = true;
            this._theTimeRangeCombo.Items.AddRange(new object[] {
            "Last 10 Minutes",
            "Last 20 Minutes",
            "Last 30 Minutes",
            "Last 1 Hour",
            "Today",
            "Last 24 Hours",
            "This Week",
            "Last 7 Days"});
            this._theTimeRangeCombo.Location = new System.Drawing.Point(109, 17);
            this._theTimeRangeCombo.Name = "_theTimeRangeCombo";
            this._theTimeRangeCombo.Size = new System.Drawing.Size(182, 21);
            this._theTimeRangeCombo.TabIndex = 0;
            this._theTimeRangeCombo.SelectedIndexChanged += new System.EventHandler(this._theTimeRangeCombo_SelectedIndexChanged);
            // 
            // _theIncludeLiveFeedCheckbox
            // 
            this._theIncludeLiveFeedCheckbox.AutoSize = true;
            this._theIncludeLiveFeedCheckbox.CheckAlign = System.Drawing.ContentAlignment.MiddleRight;
            this._theIncludeLiveFeedCheckbox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._theIncludeLiveFeedCheckbox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theIncludeLiveFeedCheckbox.Location = new System.Drawing.Point(6, 44);
            this._theIncludeLiveFeedCheckbox.Name = "_theIncludeLiveFeedCheckbox";
            this._theIncludeLiveFeedCheckbox.Size = new System.Drawing.Size(116, 18);
            this._theIncludeLiveFeedCheckbox.TabIndex = 9;
            this._theIncludeLiveFeedCheckbox.Text = "Include Live Feed";
            this._theIncludeLiveFeedCheckbox.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this._theIncludeLiveFeedCheckbox.UseVisualStyleBackColor = true;
            // 
            // AlertsOptionDialog2
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ClientSize = new System.Drawing.Size(400, 288);
            this.Controls.Add(this._theEventTimeGroupBox);
            this.Controls.Add(this._fetchOptionsGroupBox);
            this.Controls.Add(this.TrafodionPanel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.HelpButton = true;
            this.Name = "AlertsOptionDialog2";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "HP Database Manager - Alerts Options ";
            this.TrafodionPanel1.ResumeLayout(false);
            this._fetchOptionsGroupBox.ResumeLayout(false);
            this.TrafodionGroupBox1.ResumeLayout(false);
            this.TrafodionGroupBox1.PerformLayout();
            this._theEventTimeGroupBox.ResumeLayout(false);
            this.TrafodionPanel2.ResumeLayout(false);
            this.TrafodionPanel2.PerformLayout();
            this.TrafodionPanel3.ResumeLayout(false);
            this.TrafodionPanel3.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button _cancelButton;
        private System.Windows.Forms.Button _okButton;
        //private TenTec.Windows.iGridLib.iGCellStyleDesign _alertLevelCellStyleDesign;
        //private TenTec.Windows.iGridLib.iGDropDownList _alertLevelDropDownList;
        //private System.Windows.Forms.ImageList _alertsImageList;
        private TenTec.Windows.iGridLib.iGColHdrStyleDesign iGColHdrStyleDesign1;
        private System.Windows.Forms.Button _resetButton;
        private Framework.Controls.TrafodionPanel TrafodionPanel1;
        private System.Windows.Forms.Button _helpButton;
        private Framework.Controls.TrafodionGroupBox _fetchOptionsGroupBox;
        private Framework.Controls.TrafodionGroupBox TrafodionGroupBox1;
        private Framework.Controls.TrafodionRadioButton _openAlertsRadioButton;
        private Framework.Controls.TrafodionRadioButton _allAlertsRadioButton;
        private System.Windows.Forms.GroupBox _theEventTimeGroupBox;
        private Framework.Controls.TrafodionPanel TrafodionPanel2;
        private Framework.Controls.TrafodionLabel TrafodionLabel6;
        private System.Windows.Forms.DateTimePicker _theEndTime;
        private System.Windows.Forms.DateTimePicker _theStartTime;
        private Framework.Controls.TrafodionLabel TrafodionLabel3;
        private Framework.Controls.TrafodionLabel TrafodionLabel2;
        private Framework.Controls.TrafodionPanel TrafodionPanel3;
        private Framework.Controls.TrafodionLabel TrafodionLabel4;
        private Framework.Controls.TrafodionComboBox _theTimeRangeCombo;
        private Framework.Controls.TrafodionCheckBox _theIncludeLiveFeedCheckbox;
        private Framework.Controls.TrafodionMessagePanel _theMessagePanel;



    }
}