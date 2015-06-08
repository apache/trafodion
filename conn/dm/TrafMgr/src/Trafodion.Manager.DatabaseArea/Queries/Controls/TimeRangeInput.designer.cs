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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    partial class TimeRangeInput
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
            this.groupBox1 = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this._theErrorMsg = new Trafodion.Manager.Framework.Controls.TrafodionLabel();
            this._startTimeGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.startTime = new Trafodion.Manager.DatabaseArea.Queries.Controls.TimeSelector();
            this.timeRangeCombo = new Trafodion.Manager.Framework.Controls.TrafodionComboBox();
            this._endTimeGroupBox = new Trafodion.Manager.Framework.Controls.TrafodionGroupBox();
            this.endTime = new Trafodion.Manager.DatabaseArea.Queries.Controls.TimeSelector();
            this.groupBox1.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this._startTimeGroupBox.SuspendLayout();
            this._endTimeGroupBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.tableLayoutPanel1);
            this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.groupBox1.Font = new System.Drawing.Font("Tahoma", 8F);
            this.groupBox1.Location = new System.Drawing.Point(0, 0);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(554, 178);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Time Range";
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 2;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Controls.Add(this._theErrorMsg, 0, 6);
            this.tableLayoutPanel1.Controls.Add(this._startTimeGroupBox, 0, 1);
            this.tableLayoutPanel1.Controls.Add(this.timeRangeCombo, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this._endTimeGroupBox, 1, 1);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(3, 16);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 7;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 43.75F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 56.25F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(548, 159);
            this.tableLayoutPanel1.TabIndex = 4;
            // 
            // _theErrorMsg
            // 
            this._theErrorMsg.AutoSize = true;
            this.tableLayoutPanel1.SetColumnSpan(this._theErrorMsg, 2);
            this._theErrorMsg.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theErrorMsg.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theErrorMsg.ForeColor = System.Drawing.Color.Red;
            this._theErrorMsg.Location = new System.Drawing.Point(3, 138);
            this._theErrorMsg.Name = "_theErrorMsg";
            this._theErrorMsg.Size = new System.Drawing.Size(542, 21);
            this._theErrorMsg.TabIndex = 4;
            // 
            // _startTimeGroupBox
            // 
            this._startTimeGroupBox.Controls.Add(this.startTime);
            this._startTimeGroupBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._startTimeGroupBox.Location = new System.Drawing.Point(3, 28);
            this._startTimeGroupBox.Name = "_startTimeGroupBox";
            this._startTimeGroupBox.Padding = new System.Windows.Forms.Padding(3, 3, 10, 10);
            this.tableLayoutPanel1.SetRowSpan(this._startTimeGroupBox, 5);
            this._startTimeGroupBox.Size = new System.Drawing.Size(268, 107);
            this._startTimeGroupBox.TabIndex = 2;
            this._startTimeGroupBox.TabStop = false;
            this._startTimeGroupBox.Text = "From Time:";
            // 
            // startTime
            // 
            this.startTime.Dock = System.Windows.Forms.DockStyle.Fill;
            this.startTime.Location = new System.Drawing.Point(3, 16);
            this.startTime.Margin = new System.Windows.Forms.Padding(4);
            this.startTime.Name = "startTime";
            this.startTime.Padding = new System.Windows.Forms.Padding(0, 0, 10, 10);
            this.startTime.Size = new System.Drawing.Size(255, 81);
            this.startTime.TabIndex = 1;
            this.startTime.InputChanged += new System.EventHandler(this.startTime_InputChanged);
            // 
            // timeRangeCombo
            // 
            this.tableLayoutPanel1.SetColumnSpan(this.timeRangeCombo, 2);
            this.timeRangeCombo.Dock = System.Windows.Forms.DockStyle.Top;
            this.timeRangeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.timeRangeCombo.FlatStyle = System.Windows.Forms.FlatStyle.System;
            this.timeRangeCombo.Font = new System.Drawing.Font("Tahoma", 8F);
            this.timeRangeCombo.FormattingEnabled = true;
            this.timeRangeCombo.Location = new System.Drawing.Point(3, 3);
            this.timeRangeCombo.Margin = new System.Windows.Forms.Padding(3, 3, 9, 5);
            this.timeRangeCombo.Name = "timeRangeCombo";
            this.timeRangeCombo.Size = new System.Drawing.Size(536, 21);
            this.timeRangeCombo.TabIndex = 0;
            this.timeRangeCombo.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
            // 
            // _endTimeGroupBox
            // 
            this._endTimeGroupBox.Controls.Add(this.endTime);
            this._endTimeGroupBox.Font = new System.Drawing.Font("Tahoma", 8F);
            this._endTimeGroupBox.Location = new System.Drawing.Point(277, 28);
            this._endTimeGroupBox.Name = "_endTimeGroupBox";
            this._endTimeGroupBox.Padding = new System.Windows.Forms.Padding(3, 3, 10, 10);
            this.tableLayoutPanel1.SetRowSpan(this._endTimeGroupBox, 5);
            this._endTimeGroupBox.Size = new System.Drawing.Size(268, 107);
            this._endTimeGroupBox.TabIndex = 3;
            this._endTimeGroupBox.TabStop = false;
            this._endTimeGroupBox.Text = "To Time:";
            // 
            // endTime
            // 
            this.endTime.Dock = System.Windows.Forms.DockStyle.Fill;
            this.endTime.Location = new System.Drawing.Point(3, 16);
            this.endTime.Margin = new System.Windows.Forms.Padding(4);
            this.endTime.Name = "endTime";
            this.endTime.Padding = new System.Windows.Forms.Padding(0, 0, 10, 10);
            this.endTime.Size = new System.Drawing.Size(255, 81);
            this.endTime.TabIndex = 1;
            this.endTime.InputChanged += new System.EventHandler(this.endTime_InputChanged);
            this.endTime.Load += new System.EventHandler(this.endTime_Load);
            // 
            // TimeRangeInput
            // 
            this.Controls.Add(this.groupBox1);
            this.Name = "TimeRangeInput";
            this.Size = new System.Drawing.Size(554, 178);
            this.groupBox1.ResumeLayout(false);
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this._startTimeGroupBox.ResumeLayout(false);
            this._endTimeGroupBox.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion

        private TrafodionGroupBox groupBox1;
        private TimeSelector startTime;
        private TrafodionComboBox timeRangeCombo;
        private TimeSelector endTime;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private TrafodionGroupBox _startTimeGroupBox;
        private TrafodionGroupBox _endTimeGroupBox;
        private TrafodionLabel _theErrorMsg;
    }
}
