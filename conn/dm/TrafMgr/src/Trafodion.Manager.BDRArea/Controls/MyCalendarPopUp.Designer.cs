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
﻿namespace Trafodion.Manager.BDRArea.Controls
{
    partial class MyCalendarPopUp
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
            this.monthCalendar1 = new System.Windows.Forms.MonthCalendar();
            this._chooseFromTimeComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.oneGuiLabel1 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiGroupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.dateFiltersListComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.oneGuiGroupBox2 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.oneGuiGroupBox4 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.monthCalendar2 = new System.Windows.Forms.MonthCalendar();
            this._chooseToTimeComboBox = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this.oneGuiLabel4 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiGroupBox3 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.fromTSTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.toTSTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.oneGuiLabel2 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.oneGuiLabel3 = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this.okButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this.cancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._utcCheckBox = new Trafodion.Manager.Framework.Controls.TrafodionCheckBox();
            this.oneGuiGroupBox5 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.oneGuiGroupBox1.SuspendLayout();
            this.oneGuiGroupBox2.SuspendLayout();
            this.oneGuiGroupBox4.SuspendLayout();
            this.oneGuiGroupBox3.SuspendLayout();
            this.oneGuiGroupBox5.SuspendLayout();
            this.SuspendLayout();
            // 
            // monthCalendar1
            // 
            this.monthCalendar1.Location = new System.Drawing.Point(12, 19);
            this.monthCalendar1.Name = "monthCalendar1";
            this.monthCalendar1.TabIndex = 0;
            this.monthCalendar1.DateChanged += new System.Windows.Forms.DateRangeEventHandler(this.monthCalendar1_DateChanged);
            this.monthCalendar1.DateSelected += new System.Windows.Forms.DateRangeEventHandler(this.monthCalendar1_DateChanged);
            // 
            // _chooseFromTimeComboBox
            // 
            this._chooseFromTimeComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._chooseFromTimeComboBox.FormattingEnabled = true;
            this._chooseFromTimeComboBox.Location = new System.Drawing.Point(264, 45);
            this._chooseFromTimeComboBox.Name = "_chooseFromTimeComboBox";
            this._chooseFromTimeComboBox.Size = new System.Drawing.Size(81, 21);
            this._chooseFromTimeComboBox.Sorted = true;
            this._chooseFromTimeComboBox.TabIndex = 1;
            this._chooseFromTimeComboBox.SelectedIndexChanged += new System.EventHandler(this._chooseFromTimeComboBox_SelectedIndexChanged);
            // 
            // oneGuiLabel1
            // 
            this.oneGuiLabel1.AutoSize = true;
            this.oneGuiLabel1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel1.Location = new System.Drawing.Point(261, 19);
            this.oneGuiLabel1.Name = "oneGuiLabel1";
            this.oneGuiLabel1.Size = new System.Drawing.Size(64, 13);
            this.oneGuiLabel1.TabIndex = 2;
            this.oneGuiLabel1.Text = "Time (UTC):";
            // 
            // oneGuiGroupBox1
            // 
            this.oneGuiGroupBox1.Controls.Add(this.dateFiltersListComboBox);
            this.oneGuiGroupBox1.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox1.Location = new System.Drawing.Point(12, 75);
            this.oneGuiGroupBox1.Name = "oneGuiGroupBox1";
            this.oneGuiGroupBox1.Size = new System.Drawing.Size(215, 61);
            this.oneGuiGroupBox1.TabIndex = 3;
            this.oneGuiGroupBox1.TabStop = false;
            this.oneGuiGroupBox1.Text = "Predefined Time Filters (UTC)";
            // 
            // dateFiltersListComboBox
            // 
            this.dateFiltersListComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.dateFiltersListComboBox.FormattingEnabled = true;
            this.dateFiltersListComboBox.Items.AddRange(new object[] {
            "Previous 24 Hr",
            "Previous 1 Hr",
            "Previous 2 Hr",
            "Previous 3 Hr",
            "Previous 4 Hr",
            "Previous 1 Day",
            "Previous 2 Days",
            "Previous 3 Days",
            "Previous 4 Days",
            "Previous 5 Days",
            "Previous 6 Days",
            "Previous 1 Week",
            "Previous 2 Weeks",
            "Previous 3 Weeks",
            "Previous 1 Month",
            "Previous 2 Months",
            "Previous 3 Months"});
            this.dateFiltersListComboBox.Location = new System.Drawing.Point(23, 20);
            this.dateFiltersListComboBox.Name = "dateFiltersListComboBox";
            this.dateFiltersListComboBox.Size = new System.Drawing.Size(159, 21);
            this.dateFiltersListComboBox.TabIndex = 0;
            this.dateFiltersListComboBox.SelectedIndexChanged += new System.EventHandler(this.dateFiltersListComboBox_SelectedIndexChanged);
            // 
            // oneGuiGroupBox2
            // 
            this.oneGuiGroupBox2.Controls.Add(this.oneGuiGroupBox4);
            this.oneGuiGroupBox2.Controls.Add(this.oneGuiGroupBox3);
            this.oneGuiGroupBox2.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox2.Location = new System.Drawing.Point(12, 156);
            this.oneGuiGroupBox2.Name = "oneGuiGroupBox2";
            this.oneGuiGroupBox2.Size = new System.Drawing.Size(740, 235);
            this.oneGuiGroupBox2.TabIndex = 4;
            this.oneGuiGroupBox2.TabStop = false;
            this.oneGuiGroupBox2.Text = "Custom Time Filter";
            // 
            // oneGuiGroupBox4
            // 
            this.oneGuiGroupBox4.Controls.Add(this.monthCalendar2);
            this.oneGuiGroupBox4.Controls.Add(this._chooseToTimeComboBox);
            this.oneGuiGroupBox4.Controls.Add(this.oneGuiLabel4);
            this.oneGuiGroupBox4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox4.Location = new System.Drawing.Point(374, 20);
            this.oneGuiGroupBox4.Name = "oneGuiGroupBox4";
            this.oneGuiGroupBox4.Size = new System.Drawing.Size(360, 200);
            this.oneGuiGroupBox4.TabIndex = 11;
            this.oneGuiGroupBox4.TabStop = false;
            this.oneGuiGroupBox4.Text = "To Date (UTC)";
            // 
            // monthCalendar2
            // 
            this.monthCalendar2.Location = new System.Drawing.Point(12, 19);
            this.monthCalendar2.Name = "monthCalendar2";
            this.monthCalendar2.TabIndex = 3;
            this.monthCalendar2.DateChanged += new System.Windows.Forms.DateRangeEventHandler(this.monthCalendar2_DateChanged);
            this.monthCalendar2.DateSelected += new System.Windows.Forms.DateRangeEventHandler(this.monthCalendar2_DateChanged);
            // 
            // _chooseToTimeComboBox
            // 
            this._chooseToTimeComboBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._chooseToTimeComboBox.FormattingEnabled = true;
            this._chooseToTimeComboBox.Location = new System.Drawing.Point(259, 45);
            this._chooseToTimeComboBox.Name = "_chooseToTimeComboBox";
            this._chooseToTimeComboBox.Size = new System.Drawing.Size(81, 21);
            this._chooseToTimeComboBox.Sorted = true;
            this._chooseToTimeComboBox.TabIndex = 4;
            this._chooseToTimeComboBox.SelectedIndexChanged += new System.EventHandler(this._chooseToTimeComboBox_SelectedIndexChanged);
            // 
            // oneGuiLabel4
            // 
            this.oneGuiLabel4.AutoSize = true;
            this.oneGuiLabel4.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiLabel4.Location = new System.Drawing.Point(256, 19);
            this.oneGuiLabel4.Name = "oneGuiLabel4";
            this.oneGuiLabel4.Size = new System.Drawing.Size(64, 13);
            this.oneGuiLabel4.TabIndex = 5;
            this.oneGuiLabel4.Text = "Time (UTC):";
            // 
            // oneGuiGroupBox3
            // 
            this.oneGuiGroupBox3.Controls.Add(this.monthCalendar1);
            this.oneGuiGroupBox3.Controls.Add(this._chooseFromTimeComboBox);
            this.oneGuiGroupBox3.Controls.Add(this.oneGuiLabel1);
            this.oneGuiGroupBox3.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox3.Location = new System.Drawing.Point(6, 20);
            this.oneGuiGroupBox3.Name = "oneGuiGroupBox3";
            this.oneGuiGroupBox3.Size = new System.Drawing.Size(360, 200);
            this.oneGuiGroupBox3.TabIndex = 6;
            this.oneGuiGroupBox3.TabStop = false;
            this.oneGuiGroupBox3.Text = "From Date (UTC)";
            // 
            // fromTSTextBox
            // 
            this.fromTSTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.fromTSTextBox.Location = new System.Drawing.Point(113, 14);
            this.fromTSTextBox.Name = "fromTSTextBox";
            this.fromTSTextBox.Size = new System.Drawing.Size(123, 21);
            this.fromTSTextBox.TabIndex = 5;
            this.fromTSTextBox.Text = "YYYY-MM-DD HH:MM:SS";
            // 
            // toTSTextBox
            // 
            this.toTSTextBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.toTSTextBox.Location = new System.Drawing.Point(349, 14);
            this.toTSTextBox.Name = "toTSTextBox";
            this.toTSTextBox.Size = new System.Drawing.Size(123, 21);
            this.toTSTextBox.TabIndex = 6;
            this.toTSTextBox.Text = "YYYY-MM-DD HH:MM:SS";
            // 
            // oneGuiLabel2
            // 
            this.oneGuiLabel2.AutoSize = true;
            this.oneGuiLabel2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.oneGuiLabel2.Location = new System.Drawing.Point(5, 17);
            this.oneGuiLabel2.Name = "oneGuiLabel2";
            this.oneGuiLabel2.Size = new System.Drawing.Size(100, 13);
            this.oneGuiLabel2.TabIndex = 7;
            this.oneGuiLabel2.Text = "From date/time:";
            // 
            // oneGuiLabel3
            // 
            this.oneGuiLabel3.AutoSize = true;
            this.oneGuiLabel3.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            this.oneGuiLabel3.Location = new System.Drawing.Point(258, 17);
            this.oneGuiLabel3.Name = "oneGuiLabel3";
            this.oneGuiLabel3.Size = new System.Drawing.Size(85, 13);
            this.oneGuiLabel3.TabIndex = 8;
            this.oneGuiLabel3.Text = "To date/time:";
            // 
            // okButton
            // 
            this.okButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.okButton.Location = new System.Drawing.Point(651, 12);
            this.okButton.Name = "okButton";
            this.okButton.Size = new System.Drawing.Size(75, 23);
            this.okButton.TabIndex = 9;
            this.okButton.Text = "OK";
            this.okButton.UseVisualStyleBackColor = true;
            this.okButton.Click += new System.EventHandler(this.okButton_Click);
            // 
            // cancelButton
            // 
            this.cancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.cancelButton.Location = new System.Drawing.Point(651, 54);
            this.cancelButton.Name = "cancelButton";
            this.cancelButton.Size = new System.Drawing.Size(75, 23);
            this.cancelButton.TabIndex = 10;
            this.cancelButton.Text = "Cancel";
            this.cancelButton.UseVisualStyleBackColor = true;
            this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
            // 
            // _utcCheckBox
            // 
            this._utcCheckBox.AutoSize = true;
            this._utcCheckBox.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this._utcCheckBox.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._utcCheckBox.Location = new System.Drawing.Point(490, 15);
            this._utcCheckBox.Name = "_utcCheckBox";
            this._utcCheckBox.Size = new System.Drawing.Size(84, 18);
            this._utcCheckBox.TabIndex = 11;
            this._utcCheckBox.Text = "LCT Adjust";
            this._utcCheckBox.UseVisualStyleBackColor = true;
            this._utcCheckBox.Click += new System.EventHandler(this._utcCheckBox_Click);
            // 
            // oneGuiGroupBox5
            // 
            this.oneGuiGroupBox5.Controls.Add(this.fromTSTextBox);
            this.oneGuiGroupBox5.Controls.Add(this.toTSTextBox);
            this.oneGuiGroupBox5.Controls.Add(this.oneGuiLabel2);
            this.oneGuiGroupBox5.Controls.Add(this._utcCheckBox);
            this.oneGuiGroupBox5.Controls.Add(this.oneGuiLabel3);
            this.oneGuiGroupBox5.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this.oneGuiGroupBox5.Location = new System.Drawing.Point(12, 12);
            this.oneGuiGroupBox5.Name = "oneGuiGroupBox5";
            this.oneGuiGroupBox5.Size = new System.Drawing.Size(583, 47);
            this.oneGuiGroupBox5.TabIndex = 14;
            this.oneGuiGroupBox5.TabStop = false;
            this.oneGuiGroupBox5.Text = "Your Selection";
            // 
            // MyCalendarPopUp
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(772, 411);
            this.Controls.Add(this.oneGuiGroupBox5);
            this.Controls.Add(this.cancelButton);
            this.Controls.Add(this.okButton);
            this.Controls.Add(this.oneGuiGroupBox2);
            this.Controls.Add(this.oneGuiGroupBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "MyCalendarPopUp";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "HP Database Manager - Date/Time Filters";
            this.TopMost = true;
            this.oneGuiGroupBox1.ResumeLayout(false);
            this.oneGuiGroupBox2.ResumeLayout(false);
            this.oneGuiGroupBox4.ResumeLayout(false);
            this.oneGuiGroupBox4.PerformLayout();
            this.oneGuiGroupBox3.ResumeLayout(false);
            this.oneGuiGroupBox3.PerformLayout();
            this.oneGuiGroupBox5.ResumeLayout(false);
            this.oneGuiGroupBox5.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.MonthCalendar monthCalendar1;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _chooseFromTimeComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox1;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox2;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox fromTSTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox toTSTextBox;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox dateFiltersListComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel2;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel3;
        private System.Windows.Forms.MonthCalendar monthCalendar2;
        private Trafodion.Manager.Framework.Controls.TrafodionComboBox _chooseToTimeComboBox;
        private Trafodion.Manager.Framework.Controls.TrafodionLabel oneGuiLabel4;
        private Trafodion.Manager.Framework.Controls.TrafodionButton okButton;
        private Trafodion.Manager.Framework.Controls.TrafodionButton cancelButton;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox4;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox3;
        private Trafodion.Manager.Framework.Controls.TrafodionCheckBox _utcCheckBox;
        private Trafodion.Manager.Framework.Controls.TrafodionGroupBox oneGuiGroupBox5;
    }
}