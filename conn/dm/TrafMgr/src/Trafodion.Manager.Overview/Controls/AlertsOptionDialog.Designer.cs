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
    partial class AlertsOptionDialog
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
            this.components = new System.ComponentModel.Container();
            this._alertLevelCellStyleDesign = new TenTec.Windows.iGridLib.iGCellStyleDesign();
            this._alertLevelDropDownList = new TenTec.Windows.iGridLib.iGDropDownList();
            this._alertsImageList = new System.Windows.Forms.ImageList(this.components);
            this.iGColHdrStyleDesign1 = new TenTec.Windows.iGridLib.iGColHdrStyleDesign();
            this._fetchOptionsGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.oneGuiGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this._openAlertsRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this._allAlertsRadioButton = new Trafodion.Manager.Framework.Controls.TrafodionRadioButton();
            this.timeRangeInput1 = new Trafodion.Manager.DatabaseArea.Queries.Controls.TimeRangeInput();
            this._cancelButton = new System.Windows.Forms.Button();
            this._okButton = new System.Windows.Forms.Button();
            this._resetButton = new System.Windows.Forms.Button();
            this.oneGuiPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._helpButton = new System.Windows.Forms.Button();
            this._fetchOptionsGroupBox.SuspendLayout();
            this.oneGuiGroupBox1.SuspendLayout();
            this.oneGuiPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // _alertLevelCellStyleDesign
            // 
            this._alertLevelCellStyleDesign.ContentIndent = new TenTec.Windows.iGridLib.iGIndent(1, 1, 1, 1);
            this._alertLevelCellStyleDesign.CustomDrawFlags = TenTec.Windows.iGridLib.iGCustomDrawFlags.None;
            this._alertLevelCellStyleDesign.DropDownControl = this._alertLevelDropDownList;
            this._alertLevelCellStyleDesign.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            this._alertLevelCellStyleDesign.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            this._alertLevelCellStyleDesign.Flags = ((TenTec.Windows.iGridLib.iGCellFlags)((TenTec.Windows.iGridLib.iGCellFlags.DisplayText | TenTec.Windows.iGridLib.iGCellFlags.DisplayImage)));
            this._alertLevelCellStyleDesign.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.MiddleLeft;
            this._alertLevelCellStyleDesign.ImageList = this._alertsImageList;
            this._alertLevelCellStyleDesign.ReadOnly = TenTec.Windows.iGridLib.iGBool.False;
            this._alertLevelCellStyleDesign.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            this._alertLevelCellStyleDesign.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.MiddleLeft;
            this._alertLevelCellStyleDesign.TextFormatFlags = TenTec.Windows.iGridLib.iGStringFormatFlags.None;
            this._alertLevelCellStyleDesign.TextPosToImage = TenTec.Windows.iGridLib.iGTextPosToImage.Horizontally;
            this._alertLevelCellStyleDesign.TextTrimming = TenTec.Windows.iGridLib.iGStringTrimming.EllipsisCharacter;
            this._alertLevelCellStyleDesign.Type = TenTec.Windows.iGridLib.iGCellType.Text;
            this._alertLevelCellStyleDesign.TypeFlags = TenTec.Windows.iGridLib.iGCellTypeFlags.NoTextEdit;
            // 
            // _alertLevelDropDownList
            // 
            this._alertLevelDropDownList.BackColor = System.Drawing.Color.Empty;
            this._alertLevelDropDownList.ForeColor = System.Drawing.Color.Empty;
            this._alertLevelDropDownList.ImageList = this._alertsImageList;
            this._alertLevelDropDownList.SelItemBackColor = System.Drawing.Color.Empty;
            this._alertLevelDropDownList.SelItemForeColor = System.Drawing.Color.Empty;
            // 
            // _alertsImageList
            // 
            this._alertsImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
            this._alertsImageList.ImageSize = new System.Drawing.Size(16, 16);
            this._alertsImageList.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // iGColHdrStyleDesign1
            // 
            this.iGColHdrStyleDesign1.ContentIndent = new TenTec.Windows.iGridLib.iGIndent(1, 1, 1, 1);
            this.iGColHdrStyleDesign1.CustomDrawFlags = TenTec.Windows.iGridLib.iGCustomDrawFlags.None;
            this.iGColHdrStyleDesign1.Flags = ((TenTec.Windows.iGridLib.iGColHdrFlags)((TenTec.Windows.iGridLib.iGColHdrFlags.DisplayText | TenTec.Windows.iGridLib.iGColHdrFlags.DisplayImage)));
            this.iGColHdrStyleDesign1.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.MiddleLeft;
            this.iGColHdrStyleDesign1.SortInfoVisible = TenTec.Windows.iGridLib.iGBool.True;
            this.iGColHdrStyleDesign1.TextAlign = TenTec.Windows.iGridLib.iGContentAlignment.TopLeft;
            this.iGColHdrStyleDesign1.TextPosToImage = TenTec.Windows.iGridLib.iGTextPosToImage.Horizontally;
            this.iGColHdrStyleDesign1.TextTrimming = TenTec.Windows.iGridLib.iGStringTrimming.EllipsisCharacter;
            // 
            // _fetchOptionsGroupBox
            // 
            this._fetchOptionsGroupBox.Controls.Add(this.oneGuiGroupBox1);
            this._fetchOptionsGroupBox.Dock = System.Windows.Forms.DockStyle.Top;
            this._fetchOptionsGroupBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._fetchOptionsGroupBox.Location = new System.Drawing.Point(0, 0);
            this._fetchOptionsGroupBox.Name = "_fetchOptionsGroupBox";
            this._fetchOptionsGroupBox.Size = new System.Drawing.Size(523, 64);
            this._fetchOptionsGroupBox.TabIndex = 0;
            this._fetchOptionsGroupBox.TabStop = false;
            this._fetchOptionsGroupBox.Text = "Fetch Options";
            // 
            // oneGuiGroupBox1
            // 
            this.oneGuiGroupBox1.Controls.Add(this._openAlertsRadioButton);
            this.oneGuiGroupBox1.Controls.Add(this._allAlertsRadioButton);
            this.oneGuiGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox1.Location = new System.Drawing.Point(9, 15);
            this.oneGuiGroupBox1.Name = "oneGuiGroupBox1";
            this.oneGuiGroupBox1.Size = new System.Drawing.Size(249, 33);
            this.oneGuiGroupBox1.TabIndex = 4;
            this.oneGuiGroupBox1.TabStop = false;
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
            // timeRangeInput1
            // 
            this.timeRangeInput1.Dock = System.Windows.Forms.DockStyle.Top;
            this.timeRangeInput1.Location = new System.Drawing.Point(0, 64);
            this.timeRangeInput1.Name = "timeRangeInput1";
            this.timeRangeInput1.RangeGroupBoxText = "Time Range (Server Side LCT)";
            this.timeRangeInput1.Size = new System.Drawing.Size(523, 162);
            this.timeRangeInput1.TabIndex = 2;
            this.timeRangeInput1.TimeRangeString = "";
            // 
            // _cancelButton
            // 
            this._cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._cancelButton.Location = new System.Drawing.Point(365, 5);
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
            this._okButton.Location = new System.Drawing.Point(286, 5);
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
            // oneGuiPanel1
            // 
            this.oneGuiPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.oneGuiPanel1.Controls.Add(this._okButton);
            this.oneGuiPanel1.Controls.Add(this._resetButton);
            this.oneGuiPanel1.Controls.Add(this._helpButton);
            this.oneGuiPanel1.Controls.Add(this._cancelButton);
            this.oneGuiPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.oneGuiPanel1.Location = new System.Drawing.Point(0, 238);
            this.oneGuiPanel1.Name = "oneGuiPanel1";
            this.oneGuiPanel1.Size = new System.Drawing.Size(523, 33);
            this.oneGuiPanel1.TabIndex = 4;
            // 
            // _helpButton
            // 
            this._helpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._helpButton.Location = new System.Drawing.Point(444, 5);
            this._helpButton.Name = "_helpButton";
            this._helpButton.Size = new System.Drawing.Size(75, 23);
            this._helpButton.TabIndex = 3;
            this._helpButton.Text = "&Help";
            this._helpButton.UseVisualStyleBackColor = true;
            this._helpButton.Click += new System.EventHandler(this._helpButton_Click);
            // 
            // AlertsOptionDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.WhiteSmoke;
            this.ClientSize = new System.Drawing.Size(523, 271);
            this.Controls.Add(this.oneGuiPanel1);
            this.Controls.Add(this.timeRangeInput1);
            this.Controls.Add(this._fetchOptionsGroupBox);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.HelpButton = true;
            this.Name = "AlertsOptionDialog";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "HP Database Manager - Alerts Options ";
            this._fetchOptionsGroupBox.ResumeLayout(false);
            this.oneGuiGroupBox1.ResumeLayout(false);
            this.oneGuiGroupBox1.PerformLayout();
            this.oneGuiPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox _fetchOptionsGroupBox;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _allAlertsRadioButton;
        private Trafodion.Manager.Framework.Controls.TrafodionRadioButton _openAlertsRadioButton;
        private System.Windows.Forms.Button _cancelButton;
        private System.Windows.Forms.Button _okButton;
        private TenTec.Windows.iGridLib.iGCellStyleDesign _alertLevelCellStyleDesign;
        private TenTec.Windows.iGridLib.iGDropDownList _alertLevelDropDownList;
        private System.Windows.Forms.ImageList _alertsImageList;
        private TenTec.Windows.iGridLib.iGColHdrStyleDesign iGColHdrStyleDesign1;
        private Trafodion.Manager.DatabaseArea.Queries.Controls.TimeRangeInput timeRangeInput1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox1;
        private System.Windows.Forms.Button _resetButton;
        private Framework.Controls.TrafodionPanel oneGuiPanel1;
        private System.Windows.Forms.Button _helpButton;



    }
}