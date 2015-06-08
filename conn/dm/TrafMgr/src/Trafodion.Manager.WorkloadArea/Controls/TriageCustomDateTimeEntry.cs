//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Trafodion.Manager.WorkloadArea.Controls
{
	public partial class TriageCustomDateTimeEntry : Form {

		public const String CUSTOM_DATETIME_FORMAT = "dddd MMM  dd,  yyyy  hh:mm:ss tt";


		public TriageCustomDateTimeEntry() {
			InitializeComponent();
			this.startingDateTimePicker.Value = DateTime.Now;
			this.endingDateTimePicker.Value = DateTime.Now;
		}


		public DateTime StartDateTime {
			get {
				if ("".Equals(this.startingDateTimePicker.CustomFormat.Trim()) )
					return DateTime.MinValue;

				return  this.startingDateTimePicker.Value;
			}

			set {
				this.startingDateTimePicker.Value = value;
				if (DateTime.MinValue < value)
					this.startingDateTimePicker.CustomFormat = CUSTOM_DATETIME_FORMAT;

			}
		}


		public DateTime EndDateTime {
			get {
				if ("".Equals(this.endingDateTimePicker.CustomFormat.Trim()))
					return DateTime.MinValue;

				return this.endingDateTimePicker.Value;
			}

			set {
				this.endingDateTimePicker.Value = value;
				if (DateTime.MinValue < value)
					this.endingDateTimePicker.CustomFormat = CUSTOM_DATETIME_FORMAT;

			}
		}


		private void startingDateTimePicker_DropDown(object sender, EventArgs e) {
			checkAndSetDateTimePickerFormat(this.startingDateTimePicker, Keys.Down);
		}


		private void endingDateTimePicker_DropDown(object sender, EventArgs e) {
			checkAndSetDateTimePickerFormat(this.endingDateTimePicker, Keys.Down);
		}


		private void startingDateTimePicker_KeyDown(object sender, KeyEventArgs e) {
			checkAndSetDateTimePickerFormat(this.startingDateTimePicker, e.KeyCode);

		}

		private void endingDateTimePicker_KeyDown(object sender, KeyEventArgs e) {
			checkAndSetDateTimePickerFormat(this.endingDateTimePicker, e.KeyCode);
		}


		private void nccCustomDateTimeEntryOKButton_Click(object sender, EventArgs e) {
			this.DialogResult = DialogResult.OK;
			this.Close();
		}




		private void resetCustomDateTimeFormat(DateTimePicker theControl) {
			theControl.CustomFormat = " ";
		}

		private void setCustomDateTimeFormat(DateTimePicker theControl) {
			theControl.CustomFormat = CUSTOM_DATETIME_FORMAT;
		}


		private void checkAndSetDateTimePickerFormat(DateTimePicker theControl,
													 Keys theKeys) {
			switch (theKeys) {
				case Keys.Delete:
				case Keys.Shift | Keys.Delete:
				case Keys.Back:
					resetCustomDateTimeFormat(theControl);
					break;

				default:
					if (0 >= theControl.CustomFormat.Trim().Length) {
						theControl.Value = DateTime.Now;
						setCustomDateTimeFormat(theControl);
					}

					break;
			}

		}

		private void changeDateTimePickerValue(DateTimePicker theControl, double offsetInHours) {
			try {
				DateTime theTime = DateTime.Now;
				if (0 < theControl.CustomFormat.Trim().Length)
					theTime = theControl.Value;


				TimeSpan tsOffset = TimeSpan.FromHours(offsetInHours);
				theControl.Value = theTime.Add(tsOffset);
				setCustomDateTimeFormat(theControl);

			} catch (Exception) {
			}

		}


		private void startTimeDayMinusToolStripButton_Click(object sender, EventArgs e) {
			changeDateTimePickerValue(startingDateTimePicker, -24.0);
		}

		private void startTimeDayPlusToolStripButton_Click(object sender, EventArgs e) {
			changeDateTimePickerValue(startingDateTimePicker, 24.0);
		}

		private void startTimeHourMinusToolStripButton_Click(object sender, EventArgs e) {
			changeDateTimePickerValue(startingDateTimePicker, -1.0);
		}

		private void startTimeHourPlusToolStripButton_Click(object sender, EventArgs e) {
			changeDateTimePickerValue(startingDateTimePicker, 1.0);
		}

		private void endTimeDayMinusToolStripButton_Click(object sender, EventArgs e) {
			changeDateTimePickerValue(endingDateTimePicker, -24.0);
		}

		private void endTimeDayPlusToolStripButton_Click(object sender, EventArgs e) {
			changeDateTimePickerValue(endingDateTimePicker, 24.0);
		}

		private void endTimeHourMinusToolStripButton_Click(object sender, EventArgs e) {
			changeDateTimePickerValue(endingDateTimePicker, -1.0);
		}

		private void endTimeHourPlusToolStripButton_Click(object sender, EventArgs e) {
			changeDateTimePickerValue(endingDateTimePicker, 1.0);
		}

	}
}
