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

using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Models;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// The notes user control. Captures notes for alert updates.
    /// </summary>
    public partial class AlarmNotesUserControl : UserControl
    {
        #region private member variables
        #endregion private member variables

        #region Public properties

        /// <summary>
        /// The current alerts notes
        /// </summary>
        public string AlarmNotes
        {
            get
            {
                if (alertResolvedRadioButton.Checked)
                {
                    return alertResolvedRadioButton.Text;
                }
                else
                if (falseAlertRadioButton.Checked)
                {
                    return falseAlertRadioButton.Text;
                }
                else
                if (noActionRadioButton.Checked)
                {
                    return noActionRadioButton.Text;
                }
                if (duplicateAlertRadioButton.Checked)
                {
                    return duplicateAlertRadioButton.Text;
                }
                else
                {
                    return customNotesTextBox.Text;
                }
            }
        }

        #endregion Public properties

        /// <summary>
        /// Constructs the alerts notes control
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="existingNote"></param>
        public AlarmNotesUserControl(ConnectionDefinition aConnectionDefinition, string existingNote)
        {
            InitializeComponent();
          
            customNotesTextBox.MaxLength = AlertsDataProvider.GetAlarmNotesLength(aConnectionDefinition) 
                - (string.IsNullOrEmpty(existingNote) ? 0: existingNote.Length);

            alertResolvedRadioButton.Text = Properties.Resources.AlertResolvedNote;
            falseAlertRadioButton.Text = Properties.Resources.FalseAlertNote;
            duplicateAlertRadioButton.Text = Properties.Resources.DuplicateAlertNote;
            noActionRadioButton.Text = Properties.Resources.AlertsNoActionRequiredNote;

            //Set the existing note into the controls
            if (!string.IsNullOrEmpty(existingNote))
            {
                if (existingNote.Trim().Equals(Properties.Resources.AlertResolvedNote))
                {
                    alertResolvedRadioButton.Checked = true;
                }
                else if (existingNote.Trim().Equals(Properties.Resources.DuplicateAlertNote))
                {
                    duplicateAlertRadioButton.Checked = true;
                }
                else if (existingNote.Trim().Equals(Properties.Resources.AlertsNoActionRequiredNote))
                {
                    noActionRadioButton.Checked = true;
                }
                else if (existingNote.Trim().Equals(Properties.Resources.FalseAlertNote))
                {
                    falseAlertRadioButton.Checked = true;
                }
                else
                {
                    customNoteRadioButton.Checked = true;
                }
                _updateHistoryTextBox.Text = existingNote.Trim();
            }
            else
            {
                customNoteRadioButton.Checked = true;
                existingNotesPanel.Visible = false;
                _updateHistoryLabel.Visible = false;
                _updateHistoryTextBox.Visible = false;
                SetBounds(Bounds.X, Bounds.Y, Bounds.Width, Bounds.Height - existingNotesPanel.Height);
            }
        }

        private void customNoteRadioButton_Click(object sender, System.EventArgs e)
        {

        }

        private void customNotesTextBox_TextChanged(object sender, System.EventArgs e)
        {
            customNoteRadioButton.Checked = true;
        }

    }
}
