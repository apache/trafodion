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
namespace Trafodion.Manager.WorkloadArea.Controls
{
	partial class TriageCustomDateTimeEntry {
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing) {
			if (disposing && (components != null)) {
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent() {
			this.components = new System.ComponentModel.Container();
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TriageCustomDateTimeEntry));
			this.startingDateTimePicker = new System.Windows.Forms.DateTimePicker();
			this.endingDateTimePicker = new System.Windows.Forms.DateTimePicker();
			this.customDateAndTimeGroupBox = new System.Windows.Forms.GroupBox();
			this.startingTimeButtonPanel = new System.Windows.Forms.Panel();
			this.startingTimeToolStrip = new System.Windows.Forms.ToolStrip();
			this.startTimeDayMinusToolStripButton = new System.Windows.Forms.ToolStripButton();
			this.startTimeDayToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
			this.startTimeDayPlusToolStripButton = new System.Windows.Forms.ToolStripButton();
			this.startTimeDayPadderToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
			this.startTimePadderToolStripLabel = new System.Windows.Forms.ToolStripLabel();
			this.startTimePadderHourToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
			this.startTimeHourMinusToolStripButton = new System.Windows.Forms.ToolStripButton();
			this.startTimeHourToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
			this.startTimeHourPlusToolStripButton = new System.Windows.Forms.ToolStripButton();
			this.endingTimeButtonPanel = new System.Windows.Forms.Panel();
			this.endingTimeToolStrip = new System.Windows.Forms.ToolStrip();
			this.endTimeDayMinusToolStripButton = new System.Windows.Forms.ToolStripButton();
			this.endTimeDayToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
			this.endTimeDayPlusToolStripButton = new System.Windows.Forms.ToolStripButton();
			this.endTimeDayPadderToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
			this.endTimePadderToolStripLabel = new System.Windows.Forms.ToolStripLabel();
			this.endTimePadderHourToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
			this.endTimeHourMinusToolStripButton = new System.Windows.Forms.ToolStripButton();
			this.endTimeHourToolStripSeparator = new System.Windows.Forms.ToolStripSeparator();
			this.endTimeHourPlusToolStripButton = new System.Windows.Forms.ToolStripButton();
			this.startingTimeLabel = new System.Windows.Forms.Label();
			this.endingTimeLabel = new System.Windows.Forms.Label();
			this.nccCustomDateTimeEntryOKButton = new System.Windows.Forms.Button();
			this.nccCustomDateTimeEntryCancelButton = new System.Windows.Forms.Button();
			this.nccCustomDateTimeEntryToolTip = new System.Windows.Forms.ToolTip(this.components);
			this.customDateAndTimeGroupBox.SuspendLayout();
			this.startingTimeButtonPanel.SuspendLayout();
			this.startingTimeToolStrip.SuspendLayout();
			this.endingTimeButtonPanel.SuspendLayout();
			this.endingTimeToolStrip.SuspendLayout();
			this.SuspendLayout();
			// 
			// startingDateTimePicker
			// 
			this.startingDateTimePicker.CustomFormat = " ";
			this.startingDateTimePicker.Font = new System.Drawing.Font("Arial", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.startingDateTimePicker.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
			this.startingDateTimePicker.Location = new System.Drawing.Point(150, 35);
			this.startingDateTimePicker.Name = "startingDateTimePicker";
			this.startingDateTimePicker.Size = new System.Drawing.Size(325, 21);
			this.startingDateTimePicker.TabIndex = 0;
			this.nccCustomDateTimeEntryToolTip.SetToolTip(this.startingDateTimePicker, "Select or enter the starting date and time in \r\n     mmm dd, yyyy  hh:mm:ss AM|PM" +
					" format.\r\nE.g. Jan 01, 2008 11:35 AM\r\n\r\nUse the backspace or delete key to clear" +
					" the current value.");
			this.startingDateTimePicker.Value = new System.DateTime(2007, 11, 1, 0, 0, 0, 0);
			this.startingDateTimePicker.DropDown += new System.EventHandler(this.startingDateTimePicker_DropDown);
			this.startingDateTimePicker.KeyDown += new System.Windows.Forms.KeyEventHandler(this.startingDateTimePicker_KeyDown);
			// 
			// endingDateTimePicker
			// 
			this.endingDateTimePicker.CustomFormat = " ";
			this.endingDateTimePicker.Font = new System.Drawing.Font("Arial", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.endingDateTimePicker.Format = System.Windows.Forms.DateTimePickerFormat.Custom;
			this.endingDateTimePicker.Location = new System.Drawing.Point(150, 120);
			this.endingDateTimePicker.Name = "endingDateTimePicker";
			this.endingDateTimePicker.Size = new System.Drawing.Size(325, 21);
			this.endingDateTimePicker.TabIndex = 1;
			this.nccCustomDateTimeEntryToolTip.SetToolTip(this.endingDateTimePicker, "Select or enter the ending date and time in \r\n     mmm dd, yyyy  hh:mm:ss AM|PM f" +
					"ormat.\r\nE.g. Jan 01, 2008 2:00 PM\r\n\r\nUse the backspace or delete key to clear th" +
					"e current value.");
			this.endingDateTimePicker.Value = new System.DateTime(2007, 11, 1, 1, 15, 0, 0);
			this.endingDateTimePicker.DropDown += new System.EventHandler(this.endingDateTimePicker_DropDown);
			this.endingDateTimePicker.KeyDown += new System.Windows.Forms.KeyEventHandler(this.endingDateTimePicker_KeyDown);
			// 
			// customDateAndTimeGroupBox
			// 
			this.customDateAndTimeGroupBox.Controls.Add(this.startingTimeButtonPanel);
			this.customDateAndTimeGroupBox.Controls.Add(this.endingTimeButtonPanel);
			this.customDateAndTimeGroupBox.Controls.Add(this.startingTimeLabel);
			this.customDateAndTimeGroupBox.Controls.Add(this.startingDateTimePicker);
			this.customDateAndTimeGroupBox.Controls.Add(this.endingTimeLabel);
			this.customDateAndTimeGroupBox.Controls.Add(this.endingDateTimePicker);
			this.customDateAndTimeGroupBox.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.customDateAndTimeGroupBox.Location = new System.Drawing.Point(20, 20);
			this.customDateAndTimeGroupBox.Name = "customDateAndTimeGroupBox";
			this.customDateAndTimeGroupBox.Size = new System.Drawing.Size(500, 200);
			this.customDateAndTimeGroupBox.TabIndex = 101;
			this.customDateAndTimeGroupBox.TabStop = false;
			this.customDateAndTimeGroupBox.Text = "Custom Date and Time";
			// 
			// startingTimeButtonPanel
			// 
			this.startingTimeButtonPanel.Controls.Add(this.startingTimeToolStrip);
			this.startingTimeButtonPanel.Location = new System.Drawing.Point(150, 65);
			this.startingTimeButtonPanel.Name = "startingTimeButtonPanel";
			this.startingTimeButtonPanel.Size = new System.Drawing.Size(325, 25);
			this.startingTimeButtonPanel.TabIndex = 5;
			// 
			// startingTimeToolStrip
			// 
			this.startingTimeToolStrip.Dock = System.Windows.Forms.DockStyle.Fill;
			this.startingTimeToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.startTimeDayMinusToolStripButton,
            this.startTimeDayToolStripSeparator,
            this.startTimeDayPlusToolStripButton,
            this.startTimeDayPadderToolStripSeparator,
            this.startTimePadderToolStripLabel,
            this.startTimePadderHourToolStripSeparator,
            this.startTimeHourMinusToolStripButton,
            this.startTimeHourToolStripSeparator,
            this.startTimeHourPlusToolStripButton});
			this.startingTimeToolStrip.Location = new System.Drawing.Point(0, 0);
			this.startingTimeToolStrip.Name = "startingTimeToolStrip";
			this.startingTimeToolStrip.Size = new System.Drawing.Size(325, 25);
			this.startingTimeToolStrip.TabIndex = 4;
			this.startingTimeToolStrip.Text = "toolStrip1";
			// 
			// startTimeDayMinusToolStripButton
			// 
			this.startTimeDayMinusToolStripButton.BackColor = System.Drawing.Color.AliceBlue;
			this.startTimeDayMinusToolStripButton.Image = Properties.Resources._1leftarrow;
			this.startTimeDayMinusToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.startTimeDayMinusToolStripButton.Name = "startTimeDayMinusToolStripButton";
			this.startTimeDayMinusToolStripButton.Size = new System.Drawing.Size(65, 22);
			this.startTimeDayMinusToolStripButton.Text = "  Day -  ";
			this.startTimeDayMinusToolStripButton.ToolTipText = "Previous Day";
			this.startTimeDayMinusToolStripButton.Click += new System.EventHandler(this.startTimeDayMinusToolStripButton_Click);
			// 
			// startTimeDayToolStripSeparator
			// 
			this.startTimeDayToolStripSeparator.Name = "startTimeDayToolStripSeparator";
			this.startTimeDayToolStripSeparator.Size = new System.Drawing.Size(6, 25);
			// 
			// startTimeDayPlusToolStripButton
			// 
			this.startTimeDayPlusToolStripButton.Image = Properties.Resources._1rightarrow;
			this.startTimeDayPlusToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.startTimeDayPlusToolStripButton.Name = "startTimeDayPlusToolStripButton";
			this.startTimeDayPlusToolStripButton.Size = new System.Drawing.Size(66, 22);
			this.startTimeDayPlusToolStripButton.Text = "  Day + ";
			this.startTimeDayPlusToolStripButton.ToolTipText = "Next Day";
			this.startTimeDayPlusToolStripButton.Click += new System.EventHandler(this.startTimeDayPlusToolStripButton_Click);
			// 
			// startTimeDayPadderToolStripSeparator
			// 
			this.startTimeDayPadderToolStripSeparator.Name = "startTimeDayPadderToolStripSeparator";
			this.startTimeDayPadderToolStripSeparator.Size = new System.Drawing.Size(6, 25);
			// 
			// startTimePadderToolStripLabel
			// 
			this.startTimePadderToolStripLabel.Name = "startTimePadderToolStripLabel";
			this.startTimePadderToolStripLabel.Size = new System.Drawing.Size(19, 22);
			this.startTimePadderToolStripLabel.Text = "    ";
			// 
			// startTimePadderHourToolStripSeparator
			// 
			this.startTimePadderHourToolStripSeparator.Name = "startTimePadderHourToolStripSeparator";
			this.startTimePadderHourToolStripSeparator.Size = new System.Drawing.Size(6, 25);
			// 
			// startTimeHourMinusToolStripButton
			// 
			this.startTimeHourMinusToolStripButton.Image = Properties.Resources._1downarrow;
			this.startTimeHourMinusToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.startTimeHourMinusToolStripButton.Name = "startTimeHourMinusToolStripButton";
			this.startTimeHourMinusToolStripButton.Size = new System.Drawing.Size(69, 22);
			this.startTimeHourMinusToolStripButton.Text = "  Hour -  ";
			this.startTimeHourMinusToolStripButton.ToolTipText = "Go back 1 hour";
			this.startTimeHourMinusToolStripButton.Click += new System.EventHandler(this.startTimeHourMinusToolStripButton_Click);
			// 
			// startTimeHourToolStripSeparator
			// 
			this.startTimeHourToolStripSeparator.Name = "startTimeHourToolStripSeparator";
			this.startTimeHourToolStripSeparator.Size = new System.Drawing.Size(6, 25);
			// 
			// startTimeHourPlusToolStripButton
			// 
			this.startTimeHourPlusToolStripButton.Image = Properties.Resources._1uparrow;
			this.startTimeHourPlusToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.startTimeHourPlusToolStripButton.Name = "startTimeHourPlusToolStripButton";
			this.startTimeHourPlusToolStripButton.Size = new System.Drawing.Size(70, 22);
			this.startTimeHourPlusToolStripButton.Text = "  Hour + ";
			this.startTimeHourPlusToolStripButton.ToolTipText = "Go ahead 1 hour";
			this.startTimeHourPlusToolStripButton.Click += new System.EventHandler(this.startTimeHourPlusToolStripButton_Click);
			// 
			// endingTimeButtonPanel
			// 
			this.endingTimeButtonPanel.Controls.Add(this.endingTimeToolStrip);
			this.endingTimeButtonPanel.Location = new System.Drawing.Point(150, 150);
			this.endingTimeButtonPanel.Name = "endingTimeButtonPanel";
			this.endingTimeButtonPanel.Size = new System.Drawing.Size(325, 25);
			this.endingTimeButtonPanel.TabIndex = 5;
			// 
			// endingTimeToolStrip
			// 
			this.endingTimeToolStrip.Dock = System.Windows.Forms.DockStyle.Fill;
			this.endingTimeToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.endTimeDayMinusToolStripButton,
            this.endTimeDayToolStripSeparator,
            this.endTimeDayPlusToolStripButton,
            this.endTimeDayPadderToolStripSeparator,
            this.endTimePadderToolStripLabel,
            this.endTimePadderHourToolStripSeparator,
            this.endTimeHourMinusToolStripButton,
            this.endTimeHourToolStripSeparator,
            this.endTimeHourPlusToolStripButton});
			this.endingTimeToolStrip.Location = new System.Drawing.Point(0, 0);
			this.endingTimeToolStrip.Name = "endingTimeToolStrip";
			this.endingTimeToolStrip.Size = new System.Drawing.Size(325, 25);
			this.endingTimeToolStrip.TabIndex = 4;
			this.endingTimeToolStrip.Text = "toolStrip1";
			// 
			// endTimeDayMinusToolStripButton
			// 
			this.endTimeDayMinusToolStripButton.BackColor = System.Drawing.Color.AliceBlue;
			this.endTimeDayMinusToolStripButton.Image = Properties.Resources._1leftarrow;
			this.endTimeDayMinusToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.endTimeDayMinusToolStripButton.Name = "endTimeDayMinusToolStripButton";
			this.endTimeDayMinusToolStripButton.Size = new System.Drawing.Size(65, 22);
			this.endTimeDayMinusToolStripButton.Text = "  Day -  ";
			this.endTimeDayMinusToolStripButton.ToolTipText = "Previous Day";
			this.endTimeDayMinusToolStripButton.Click += new System.EventHandler(this.endTimeDayMinusToolStripButton_Click);
			// 
			// endTimeDayToolStripSeparator
			// 
			this.endTimeDayToolStripSeparator.Name = "endTimeDayToolStripSeparator";
			this.endTimeDayToolStripSeparator.Size = new System.Drawing.Size(6, 25);
			// 
			// endTimeDayPlusToolStripButton
			// 
			this.endTimeDayPlusToolStripButton.Image = Properties.Resources._1rightarrow;
			this.endTimeDayPlusToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.endTimeDayPlusToolStripButton.Name = "endTimeDayPlusToolStripButton";
			this.endTimeDayPlusToolStripButton.Size = new System.Drawing.Size(66, 22);
			this.endTimeDayPlusToolStripButton.Text = "  Day + ";
			this.endTimeDayPlusToolStripButton.ToolTipText = "Next Day";
			this.endTimeDayPlusToolStripButton.Click += new System.EventHandler(this.endTimeDayPlusToolStripButton_Click);
			// 
			// endTimeDayPadderToolStripSeparator
			// 
			this.endTimeDayPadderToolStripSeparator.Name = "endTimeDayPadderToolStripSeparator";
			this.endTimeDayPadderToolStripSeparator.Size = new System.Drawing.Size(6, 25);
			// 
			// endTimePadderToolStripLabel
			// 
			this.endTimePadderToolStripLabel.Name = "endTimePadderToolStripLabel";
			this.endTimePadderToolStripLabel.Size = new System.Drawing.Size(19, 22);
			this.endTimePadderToolStripLabel.Text = "    ";
			// 
			// endTimePadderHourToolStripSeparator
			// 
			this.endTimePadderHourToolStripSeparator.Name = "endTimePadderHourToolStripSeparator";
			this.endTimePadderHourToolStripSeparator.Size = new System.Drawing.Size(6, 25);
			// 
			// endTimeHourMinusToolStripButton
			// 
			this.endTimeHourMinusToolStripButton.Image = Properties.Resources._1downarrow;
			this.endTimeHourMinusToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.endTimeHourMinusToolStripButton.Name = "endTimeHourMinusToolStripButton";
			this.endTimeHourMinusToolStripButton.Size = new System.Drawing.Size(69, 22);
			this.endTimeHourMinusToolStripButton.Text = "  Hour -  ";
			this.endTimeHourMinusToolStripButton.ToolTipText = "Go back 1 hour";
			this.endTimeHourMinusToolStripButton.Click += new System.EventHandler(this.endTimeHourMinusToolStripButton_Click);
			// 
			// endTimeHourToolStripSeparator
			// 
			this.endTimeHourToolStripSeparator.Name = "endTimeHourToolStripSeparator";
			this.endTimeHourToolStripSeparator.Size = new System.Drawing.Size(6, 25);
			// 
			// endTimeHourPlusToolStripButton
			// 
			this.endTimeHourPlusToolStripButton.Image = Properties.Resources._1uparrow;
			this.endTimeHourPlusToolStripButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			this.endTimeHourPlusToolStripButton.Name = "endTimeHourPlusToolStripButton";
			this.endTimeHourPlusToolStripButton.Size = new System.Drawing.Size(70, 22);
			this.endTimeHourPlusToolStripButton.Text = "  Hour + ";
			this.endTimeHourPlusToolStripButton.ToolTipText = "Go ahead 1 hour";
			this.endTimeHourPlusToolStripButton.Click += new System.EventHandler(this.endTimeHourPlusToolStripButton_Click);
			// 
			// startingTimeLabel
			// 
			this.startingTimeLabel.AutoSize = true;
			this.startingTimeLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.startingTimeLabel.Location = new System.Drawing.Point(20, 40);
			this.startingTimeLabel.Name = "startingTimeLabel";
			this.startingTimeLabel.Size = new System.Drawing.Size(108, 16);
			this.startingTimeLabel.TabIndex = 0;
			this.startingTimeLabel.Text = "Start Date and Time";
			this.nccCustomDateTimeEntryToolTip.SetToolTip(this.startingTimeLabel, "The starting date and time in \r\n     mmm dd, yyyy  hh:mm:ss AM|PM format.\r\nE.g. J" +
					"an 01, 2008 11:35 AM");
			// 
			// endingTimeLabel
			// 
			this.endingTimeLabel.AutoSize = true;
			this.endingTimeLabel.Font = new System.Drawing.Font("Trebuchet MS", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
			this.endingTimeLabel.Location = new System.Drawing.Point(28, 123);
			this.endingTimeLabel.Name = "endingTimeLabel";
			this.endingTimeLabel.Size = new System.Drawing.Size(100, 16);
			this.endingTimeLabel.TabIndex = 2;
			this.endingTimeLabel.Text = "End Date and Time";
			this.nccCustomDateTimeEntryToolTip.SetToolTip(this.endingTimeLabel, "The ending date and time in \r\n     mmm dd, yyyy  hh:mm:ss AM|PM format.\r\nE.g. Jan" +
					" 01, 2008 2:00 PM");
			// 
			// nccCustomDateTimeEntryOKButton
			// 
			this.nccCustomDateTimeEntryOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.nccCustomDateTimeEntryOKButton.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccCustomDateTimeEntryOKButton.Image = Properties.Resources.ApplyIcon;
			this.nccCustomDateTimeEntryOKButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.nccCustomDateTimeEntryOKButton.Location = new System.Drawing.Point(325, 255);
			this.nccCustomDateTimeEntryOKButton.Name = "nccCustomDateTimeEntryOKButton";
			this.nccCustomDateTimeEntryOKButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
			this.nccCustomDateTimeEntryOKButton.Size = new System.Drawing.Size(80, 25);
			this.nccCustomDateTimeEntryOKButton.TabIndex = 99;
			this.nccCustomDateTimeEntryOKButton.Text = "&OK";
			this.nccCustomDateTimeEntryToolTip.SetToolTip(this.nccCustomDateTimeEntryOKButton, "Use the custom date and times");
			this.nccCustomDateTimeEntryOKButton.UseVisualStyleBackColor = true;
			this.nccCustomDateTimeEntryOKButton.Click += new System.EventHandler(this.nccCustomDateTimeEntryOKButton_Click);
			// 
			// nccCustomDateTimeEntryCancelButton
			// 
			this.nccCustomDateTimeEntryCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.nccCustomDateTimeEntryCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.nccCustomDateTimeEntryCancelButton.Font = new System.Drawing.Font("Lucida Sans Unicode", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.nccCustomDateTimeEntryCancelButton.Image = Properties.Resources.cancelImage;
			this.nccCustomDateTimeEntryCancelButton.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
			this.nccCustomDateTimeEntryCancelButton.Location = new System.Drawing.Point(430, 255);
			this.nccCustomDateTimeEntryCancelButton.Name = "nccCustomDateTimeEntryCancelButton";
			this.nccCustomDateTimeEntryCancelButton.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
			this.nccCustomDateTimeEntryCancelButton.Size = new System.Drawing.Size(90, 25);
			this.nccCustomDateTimeEntryCancelButton.TabIndex = 100;
			this.nccCustomDateTimeEntryCancelButton.Text = "    &Cancel";
			this.nccCustomDateTimeEntryToolTip.SetToolTip(this.nccCustomDateTimeEntryCancelButton, "Discard changes");
			this.nccCustomDateTimeEntryCancelButton.UseVisualStyleBackColor = true;
			// 
			// NCCCustomDateTimeEntry
			// 
			this.AcceptButton = this.nccCustomDateTimeEntryOKButton;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.BackColor = System.Drawing.Color.AliceBlue;
			this.CancelButton = this.nccCustomDateTimeEntryCancelButton;
			this.ClientSize = new System.Drawing.Size(544, 293);
			this.Controls.Add(this.customDateAndTimeGroupBox);
			this.Controls.Add(this.nccCustomDateTimeEntryOKButton);
			this.Controls.Add(this.nccCustomDateTimeEntryCancelButton);
			this.DoubleBuffered = true;
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "NCCCustomDateTimeEntry";
			this.ShowIcon = false;
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Workload Filter Start and End Time Ranges";
			this.customDateAndTimeGroupBox.ResumeLayout(false);
			this.customDateAndTimeGroupBox.PerformLayout();
			this.startingTimeButtonPanel.ResumeLayout(false);
			this.startingTimeButtonPanel.PerformLayout();
			this.startingTimeToolStrip.ResumeLayout(false);
			this.startingTimeToolStrip.PerformLayout();
			this.endingTimeButtonPanel.ResumeLayout(false);
			this.endingTimeButtonPanel.PerformLayout();
			this.endingTimeToolStrip.ResumeLayout(false);
			this.endingTimeToolStrip.PerformLayout();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.DateTimePicker startingDateTimePicker;
		private System.Windows.Forms.DateTimePicker endingDateTimePicker;
		private System.Windows.Forms.GroupBox customDateAndTimeGroupBox;
		private System.Windows.Forms.Label startingTimeLabel;
		private System.Windows.Forms.Label endingTimeLabel;
		private System.Windows.Forms.Button nccCustomDateTimeEntryOKButton;
		private System.Windows.Forms.Button nccCustomDateTimeEntryCancelButton;
		private System.Windows.Forms.ToolTip nccCustomDateTimeEntryToolTip;
		private System.Windows.Forms.ToolStrip endingTimeToolStrip;
		private System.Windows.Forms.ToolStripButton endTimeDayMinusToolStripButton;
		private System.Windows.Forms.ToolStripButton endTimeDayPlusToolStripButton;
		private System.Windows.Forms.ToolStripSeparator endTimeDayPadderToolStripSeparator;
		private System.Windows.Forms.ToolStripButton endTimeHourMinusToolStripButton;
		private System.Windows.Forms.ToolStripButton endTimeHourPlusToolStripButton;
		private System.Windows.Forms.ToolStripSeparator endTimeDayToolStripSeparator;
		private System.Windows.Forms.ToolStripSeparator endTimeHourToolStripSeparator;
		private System.Windows.Forms.Panel endingTimeButtonPanel;
		private System.Windows.Forms.ToolStripLabel endTimePadderToolStripLabel;
		private System.Windows.Forms.ToolStripSeparator endTimePadderHourToolStripSeparator;
		private System.Windows.Forms.Panel startingTimeButtonPanel;
		private System.Windows.Forms.ToolStrip startingTimeToolStrip;
		private System.Windows.Forms.ToolStripButton startTimeDayMinusToolStripButton;
		private System.Windows.Forms.ToolStripSeparator startTimeDayToolStripSeparator;
		private System.Windows.Forms.ToolStripButton startTimeDayPlusToolStripButton;
		private System.Windows.Forms.ToolStripSeparator startTimeDayPadderToolStripSeparator;
		private System.Windows.Forms.ToolStripLabel startTimePadderToolStripLabel;
		private System.Windows.Forms.ToolStripSeparator startTimePadderHourToolStripSeparator;
		private System.Windows.Forms.ToolStripButton startTimeHourMinusToolStripButton;
		private System.Windows.Forms.ToolStripSeparator startTimeHourToolStripSeparator;
		private System.Windows.Forms.ToolStripButton startTimeHourPlusToolStripButton;
	}
}