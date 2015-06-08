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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class AlertsConfigDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {

        AlertOptionsModel _alertOptions;
        AlertOptionsModel _initialAlertOptions;
        int _initializationState;
        ConnectionDefinition _theConn;

        public AlertOptionsModel AlertOptions
        {
            get { return _alertOptions; }
        }

        /// <summary>
        /// Property: Message - the diagonse message for the dialog
        /// </summary>
        public string Message
        {
            get { return _theMessagePanel.Text.Trim(); }
            set
            {
                _theMessagePanel.Text = value;
                _theMessagePanel.Visible = !string.IsNullOrEmpty(value);
                _okButton.Enabled = string.IsNullOrEmpty(value);
            }
        }

        public AlertsConfigDialog(AlertOptionsModel alertOptions, int initializationState, ConnectionDefinition conn)
        {
            InitializeComponent();
            _alertOptions = alertOptions;
            _initialAlertOptions = alertOptions.Clone();
            _initializationState = initializationState;
            _theConn = conn;
            InitializeUIWithAlertOptions(initializationState);
            CenterToScreen();
        }

        private void InitializeUIWithAlertOptions(int initializationState)
        {
            //initialize UI from passed in alert options
            _openAlertsRadioButton.Checked = _alertOptions.FetchOpenAlertsOnly;
            _allAlertsRadioButton.Checked = !_alertOptions.FetchOpenAlertsOnly;

            Message = "";
            if (initializationState == 4)
            {                
                InitializeTimeControls();
                //set time filters

                _theTimeRangeCombo.SelectedItem = new TimeRangesHandler(_alertOptions.TimeRange);
                _theIncludeLiveFeedCheckbox.Checked = _alertOptions.IncludeLiveFeed;
                try
                {
                    if (_alertOptions.TheStartTime == TimeRangesHandler.MinCustomStartTime) 
                    {                        
                        _theStartTime.Value = GetServerLCTTime(DateTime.Now).AddDays(-30);
                        _theStartTime.Value = GetServerLCTTime(DateTime.Now).AddDays(-30);
                        _theStartTime.Checked = false;
                        
                        _theEndTime.Value = _alertOptions.TheEndTime;
                        _theEndTime.Checked = true;
                    }
                    else if (_alertOptions.TheEndTime == TimeRangesHandler.MaxCustomEndTime)
                    {
                        //_theEndTime.Value = GetServerLCTTime(DateTime.Now);
                        _theEndTime.Checked = false;

                        _theStartTime.Value = _alertOptions.TheStartTime;
                        _theStartTime.Checked = true;
                    }
                    else 
                    {
                        _theStartTime.Checked = _theEndTime.Checked = true;
                        _theStartTime.Value = _alertOptions.TheStartTime;
                        _theEndTime.Value = _alertOptions.TheEndTime;
                    }
                }
                catch (Exception)
                {
                    _theStartTime.Value = TimeRangesHandler.MinCustomStartTime;
                    _theEndTime.Value = TimeRangesHandler.MaxCustomEndTime;
                }
            }
            else 
            {
                _theEventTimeGroupBox.Enabled = false;
            }
        }
        

        private void InitializeTimeControls()
        {
            _theEventTimeGroupBox.Enabled = true;
            _theIncludeLiveFeedCheckbox.Checked = true;

            this._theTimeRangeCombo.Items.Clear();
            for (int i = 0; i < _alertOptions.TimeRanges.Count; i++)
            {
                this._theTimeRangeCombo.Items.Add(_alertOptions.TimeRanges[i]);
                if ((int)_alertOptions.TimeRanges[i].TheRange == (int)TimeRangesHandler.Range.Last30Days) 
                {
                    this._theTimeRangeCombo.SelectedIndex = i;                   
                }                    
            }
        }        

        private void _okButton_Click(object sender, System.EventArgs e)
        {
            if (_initializationState == 4) 
            {
                try
                {
                    _alertOptions.TimeRange = ((TimeRangesHandler)_theTimeRangeCombo.SelectedItem).TheRange;
                    //if (((TimeRangesHandler)_theTimeRangeCombo.SelectedItem).TheRange == TimeRangesHandler.Range.CustomRange)
                    //{
                    //    ProcessStartTime();
                    //    ProcessEndTime();
                    //}
                    _alertOptions.IncludeLiveFeed = _theIncludeLiveFeedCheckbox.Checked;
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), "Error saving time range values : " + ex.Message,
                    global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    DialogResult = DialogResult.None;
                    return;
                }
            }    

            //Save the current mappings into the alert options object
            //The alert options object is accessed by the caller using the public property
            _alertOptions.FetchOpenAlertsOnly = _openAlertsRadioButton.Checked;

            if (!_initialAlertOptions.Equals(_alertOptions))
            {
                DialogResult = DialogResult.OK;
            }
            else 
            {
                Close();
            }
            
        }

        private void ValidateStartTimeAndEndTime() 
        {            
            if(((TimeRangesHandler)_theTimeRangeCombo.SelectedItem).TheRange == TimeRangesHandler.Range.CustomRange)
            {
                if (!_theStartTime.Checked && !_theEndTime.Checked)
                {
                    Message = "You must specify the Start Time or End Time.";
                }
                else if (_theStartTime.Checked && _theEndTime.Checked)
                {
                    if (_theStartTime.Value >= GetServerLCTTime(DateTime.Now))
                    {
                        Message = "The Start Time must be smaller than the current system time.";
                        return;
                    }
                    else 
                    {
                        Message = "";
                    }

                    if (_theStartTime.Value >= _theEndTime.Value)
                    {
                        Message = "The Start Time must be smaller than the End Time.";
                    }
                    else 
                    {
                        Message = "";
                    }
                }
                else if (_theStartTime.Checked)
                {
                    if (_theStartTime.Value >= GetServerLCTTime(DateTime.Now))
                    {
                        Message = "The Start Time must be smaller than the current system time.";
                    }
                    else 
                    {
                        Message = "";
                    }
                }
                else 
                {
                    Message = "";
                }
            }           
        
        }

        //What stored in Alert Option is the Server LCT Time, not User Local Time
        private void ProcessEndTime() 
        {
            if (((TimeRangesHandler)_theTimeRangeCombo.SelectedItem).TheRange == TimeRangesHandler.Range.CustomRange) 
            {
                if (_theEndTime.Checked)
                {
                    if (_theEndTime.Value > GetServerLCTTime(DateTime.Now))
                    {
                        _alertOptions.IncludeLiveFeed = _theIncludeLiveFeedCheckbox.Checked = true;
                    }
                    else
                    {
                        _alertOptions.IncludeLiveFeed = _theIncludeLiveFeedCheckbox.Checked = false;
                    }
                    _alertOptions.TheEndTime = _theEndTime.Value;
                }
                else
                {
                    _alertOptions.TheEndTime = TimeRangesHandler.MaxCustomEndTime;
                    _alertOptions.IncludeLiveFeed = _theIncludeLiveFeedCheckbox.Checked = true;
                    //We don't need to store the end time now since it's not checked;
                } 
            }            
        }

        private void ProcessStartTime() 
        {
            if (((TimeRangesHandler)_theTimeRangeCombo.SelectedItem).TheRange == TimeRangesHandler.Range.CustomRange)
            {
                if (_theStartTime.Checked)
                {
                    _alertOptions.TheStartTime = _theStartTime.Value;
                }
                else
                {
                    _alertOptions.TheStartTime = TimeRangesHandler.MinCustomStartTime;
                }
            }
        }



        public DateTime GetServerLCTTime(DateTime userLocalTime) 
        {
            TimeSpan timeSpan = this._theConn.ServerGMTOffset;
            DateTime serverLctTime = userLocalTime.ToUniversalTime() + timeSpan;
            return serverLctTime;            
        }

        public DateTime ConvertServerLCTToUserLocalTime(DateTime serverLCTTime) 
        {
            TimeSpan timeSpan = TimeZone.CurrentTimeZone.GetUtcOffset(DateTime.Now);
            DateTime localTime = serverLCTTime.ToUniversalTime() + timeSpan;
            return localTime;
        }


        private void _resetButton_Click(object sender, EventArgs e)
        {
            _alertOptions = new AlertOptionsModel();
            InitializeUIWithAlertOptions(_initializationState);
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

        private void _theTimeRangeCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            TimeRangesHandler timeHandler = _theTimeRangeCombo.SelectedItem as TimeRangesHandler;
            if (timeHandler != null)
            {
                switch (timeHandler.TheRange)
                {
                    case TimeRangesHandler.Range.AllTimes:
                    case TimeRangesHandler.Range.Today:
                    case TimeRangesHandler.Range.LiveFeedOnly:
                        {
                            _theStartTime.Enabled = false;
                            _theEndTime.Enabled = false;
                            _theIncludeLiveFeedCheckbox.Checked = true;
                            _theIncludeLiveFeedCheckbox.Enabled = false;
                            Message = "";
                            break;
                        }                     
                    case TimeRangesHandler.Range.Last10Minutes:
                    case TimeRangesHandler.Range.Last20Minutes:
                    case TimeRangesHandler.Range.Last30Minutes:
                    case TimeRangesHandler.Range.Last1Hour:
                    case TimeRangesHandler.Range.Last24Hours:
                    case TimeRangesHandler.Range.Last7Days:
                    case TimeRangesHandler.Range.Last14Days:
                    case TimeRangesHandler.Range.Last30Days:
                        {
                            _theStartTime.Enabled = false;
                            _theEndTime.Enabled = false;
                            _theIncludeLiveFeedCheckbox.Checked = true;
                            _theIncludeLiveFeedCheckbox.Enabled = true;
                            Message = "";
                            break; 
                        }
                    case TimeRangesHandler.Range.CustomRange:
                        {
                            _theStartTime.Enabled = true;
                            _theEndTime.Enabled = true;
                            _theStartTime.Checked = true;                            
                            _theEndTime.Checked = false;
                            CustomFormatChange();
                            _theIncludeLiveFeedCheckbox.Enabled = false;                            
                            break;
                        }
                }                
            }
        }

        private void _theIncludeLiveFeedCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            //_theEndTime.Enabled = (!_theIncludeLiveFeedCheckbox.Checked);
            _alertOptions.IncludeLiveFeed = _theIncludeLiveFeedCheckbox.Checked;
        }

        private void _theStartTime_ValueChanged(object sender, EventArgs e)
        {            
            CustomFormatChange();
            ValidateStartTimeAndEndTime();
            if(Message.Equals(""))
            {
                ProcessStartTime();
            }
            
        }

        private void _theEndTime_ValueChanged(object sender, EventArgs e)
        {
            CustomFormatChange();
            ValidateStartTimeAndEndTime();
            if (Message.Equals(""))
            {
                ProcessEndTime();
            }
        }

        private void _theStartTime_KeyPress(object sender, KeyPressEventArgs e)
        {
            ValidateStartTimeAndEndTime();
        }

        private void _theEndTime_KeyPress(object sender, KeyPressEventArgs e)
        {
            ValidateStartTimeAndEndTime();
        }

      
        private void CustomFormatChange() 
        {            
            DateTime oldStartTime = _theStartTime.Value;
            if (_theEndTime.Checked == true)
            {
                this._theEndTime.CustomFormat = "yyyy-MM-dd HH:mm:ss tt";                
            }
            else
            {
                this._theEndTime.CustomFormat = " ";                
            }

            if (_theStartTime.Checked == true)
            {
                this._theStartTime.CustomFormat = "yyyy-MM-dd HH:mm:ss tt";
            }
            else
            {
                this._theStartTime.CustomFormat = " ";
            }
            _theStartTime.Value = oldStartTime;            
        }
    }
}
