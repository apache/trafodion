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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.OverviewArea.Models;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework.Queries;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class AlertsOptionDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        AlertOptions _alertOptions;
        AlertOptions _initialAlertOptions;

        public AlertOptions AlertOptions
        {
            get { return _alertOptions; }
        }

        public AlertsOptionDialog(AlertOptions alertOptions)
        {
            InitializeComponent();
            _alertOptions = alertOptions;
            _initialAlertOptions = alertOptions;
            InitializeUIWithAlertOptions();
            CenterToScreen();
        }

        private void InitializeUIWithAlertOptions()
        {
            //initialize UI from passed in alert options
            _openAlertsRadioButton.Checked = _alertOptions.FetchOpenAlertsOnly;
            _allAlertsRadioButton.Checked = !_alertOptions.FetchOpenAlertsOnly;

            //Initialize the grid with the existing severity mappings.
            initializeAlertMappingsGrid();

            timeRangeInput1.TimeRangeString = _alertOptions.TimeRangeString;

            if (_alertOptions.TimeRangeString.Equals(TimeRangeInputBase.CUSTOM_RANGE))
            {
                if (_alertOptions.StartTimeParameter != null)
                {
                    ((ReportParameterInput)timeRangeInput1).setParameter(_alertOptions.StartTimeParameter);
                    ((ReportParameterInput)timeRangeInput1).setParameter(_alertOptions.EndTimeParameter);
                }
            }
        }

        private void initializeAlertMappingsGrid()
        {
            this._alertLevelCellStyleDesign.ImageList = AlertOptions.AlertsImageList;
            this._alertLevelDropDownList.ImageList = AlertOptions.AlertsImageList;
        }

        private void _okButton_Click(object sender, System.EventArgs e)
        {
            try
            {
                _alertOptions.TimeRangeString = timeRangeInput1.TimeRangeString;

                string startString = (string)((ReportParameterInput)timeRangeInput1).getParamValue(_alertOptions.StartTimeParameter);
                if (startString != null)
                {
                    DateTime startTime = DateTime.ParseExact(startString, TimeSelector.TimePattern, null);
                    _alertOptions.StartTimeParameter.Value = startTime;
                }
                else
                {
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), "The specified from time is invalid",
                    global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    DialogResult = DialogResult.None;
                    return;
                }

                string endString = (string)((ReportParameterInput)timeRangeInput1).getParamValue(_alertOptions.EndTimeParameter);
                if (endString != null)
                {
                    DateTime endTime = DateTime.ParseExact(endString, TimeSelector.TimePattern, null);
                    _alertOptions.EndTimeParameter.Value = endTime;
                }
                else
                {
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), "The specified to time is invalid",
                    global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    DialogResult = DialogResult.None;
                    return;
                }
            }
            catch(Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), "Error saving time range values : " + ex.Message,
                global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                DialogResult = DialogResult.None;
                return;
            }

            //Save the current mappings into the alert options object
            //The alert options object is accessed by the caller using the public property
            _alertOptions.FetchOpenAlertsOnly = _openAlertsRadioButton.Checked;

            //foreach (iGRow row in _severityMappingsGrid.Rows)
            //{
            //    string severity = row.Cells[0].Value as string;
            //    _alertOptions.SeverityMappings[severity] = (AlertOptions.AlertType)row.Cells[1].Value;
            //}

            DialogResult = DialogResult.OK;
        }

        private void _resetButton_Click(object sender, EventArgs e)
        {
            _alertOptions = new AlertOptions();
            InitializeUIWithAlertOptions();
        }

        private void _cancelButton_Click(object sender, EventArgs e)
        {
            _alertOptions = _initialAlertOptions;
            Close();
        }

        private void _helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.AlertOptions);
        }

    }
}
