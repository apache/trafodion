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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class AlarmStatusUpdateDialog : TrafodionForm
    {
        private AlarmNotesUserControl _alarmNotesUserControl;

        public string AlarmStatus
        {
            get { return _statusComboBox.SelectedItem.ToString(); }
        }
        public string AlarmNotes
        {
            get { return _alarmNotesUserControl.AlarmNotes; }
        }
        
        public AlarmStatusUpdateDialog(ConnectionDefinition aConnectionDefinition, string existingNotes)
        {
            InitializeComponent();
            _alarmNotesUserControl = new AlarmNotesUserControl(aConnectionDefinition, existingNotes);

            int notesPanelSize = notesPanel.Bounds.Height;
            int notesUserControlSize = _alarmNotesUserControl.Bounds.Height;

            _alarmNotesUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            notesPanel.Controls.Add(_alarmNotesUserControl);

            _statusComboBox.Items.Add(SystemAlertsUserControl.ALERT_STATUS_ACK);
            _statusComboBox.Items.Add(SystemAlertsUserControl.ALERT_STATUS_USER_CLOSED);
            _statusComboBox.Items.Add(SystemAlertsUserControl.ALERT_STATUS_OPEN);
            _statusComboBox.SelectedIndex = 0;

            SetBounds(Bounds.X, Bounds.Y, Bounds.Width, Bounds.Height - (notesPanelSize - notesUserControlSize));
            this.CenterToParent();
        }

        private void _okButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void _cancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void _helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UpdateAlertDialog);
        }
    }
}
