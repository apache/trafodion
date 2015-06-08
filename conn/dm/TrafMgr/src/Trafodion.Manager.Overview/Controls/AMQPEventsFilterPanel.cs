#region Copyright info
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
#endregion Copyright info

using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.Serialization;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.OverviewArea.Models;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// This class shall be used to display the filters used for filtering
    /// the event data.
    /// </summary>
    public partial class AMQPEventsFilterPanel : UserControl
    {
        #region Member Variables
        public delegate void OnFilterChanged(EventFilterModel aFilterModel);
        private OnFilterChanged _OnFilterChangedImpl = null;
        private TrafodionChangeTracker _theChangeTracker = null;
        EventFilterModel _theFilterModel = null;
        ConnectionDefinition _theConnectionDefinition;
        private bool _theButtonsEnabled = true;
        EventDetails _theEventDetails;
        private bool _forLiveEvents = false;
        #endregion

        #region Constructor

        /// <summary>
        /// Default constructor
        /// </summary>
        public AMQPEventsFilterPanel()
        {
            InitializeComponent();
            _theToolTip.IsBalloon = false;
        }

        #endregion

        #region Properties
        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set
            {
                _theConnectionDefinition = value;
                if (_theConnectionDefinition != null)
                {
                    _theEventTimeGroupBox.Text = "Event Time (Server Local Time in 24-Hour format)";
                    //if (string.IsNullOrEmpty(_theConnectionDefinition.ServerTimeZoneName))
                    //{
                    //    _theEventTimeGroupBox.Text = "Event Time (Server Local Time in 24-Hour format)";
                    //}
                    //else
                    //{
                    //    _theEventTimeGroupBox.Text = string.Format("Event Time (Server Local Time in {0} 24 hour format)", _theConnectionDefinition.ServerTimeZoneName);
                    //}
                    //_theFilterModel = getDefaultEventFilterModel();
                    //AddFilterControls();
                    //populateUIFromModel(_theFilterModel);
                    //_theApplyFilterButton.Enabled = false;
                    //AddChangeTracker();
                }
            }
        }

        public EventDetails TheEventDetails
        {
            get { return _theEventDetails; }
            set { _theEventDetails = value; }
        }

        public EventFilterModel FilterModel
        {
            get { return _theFilterModel; }
            set { _theFilterModel = value; }
        }

        public bool ForLiveEvents
        {
            get { return _forLiveEvents; }
            set 
            { 
                _forLiveEvents = value;
                _theEventTimeGroupBox.Enabled = _theEventTimeGroupBox.Visible = !_forLiveEvents;
                if (_forLiveEvents)
                {
                    this._theTopPanel.Controls.Remove(_theEventTimeGroupBox);
                    this._theTopPanel.Controls.Remove(_theMessageGroupBox);
                    this._theTopPanel.Controls.Add(_theMessageGroupBox, 0, 4);
                }
            }
        }

        #endregion

        #region Public Methods

        public OnFilterChanged OnFilterChangedImpl
        {
            set { _OnFilterChangedImpl = value; }
        }

        public string GetFilterDisplayString(TextEventsDataHandler aTextEventsDataHandler)
        {
            return _theFilterModel.GetFormattedFilterString(aTextEventsDataHandler);
        }

        /*public string GetLiveFeedFilterDisplayString(LiveFeedEventsDataHandler aLiveFeedEventsDataHandler)
        {
            return _theFilterModel.GetLiveFeedFormattedFilterString(aLiveFeedEventsDataHandler);
        }*/

        public void SetFilterValues(string columnName, object value)
        {
            EventFilterModel filterModel = populateFilterFromUI();
            if (columnName.Equals(EventFilterModel.EVENT_ID))
            {
                filterModel.AddEventId(value + "");
            }
            else if (columnName.Equals(EventFilterModel.SEVERITY))
            {
            }
            else if (columnName.Equals(EventFilterModel.SUBSYSTEM))
            {
            }
            else if (columnName.Equals(EventFilterModel.PROCESS_ID))
            {
                filterModel.AddProcessId(value + "");
            }
            else if (columnName.Equals(EventFilterModel.PROCESS_NAME))
            {
                filterModel.AddProcessName(value + "");
            }
            else if (columnName.Equals(EventFilterModel.MESSAGE))
            {
            }
            populateUIFromModel(filterModel);
            UpdateButtons();

        }

        public void EnableDisableButtons(bool enable)
        {
            _theButtonsEnabled = enable;
            UpdateButtons();
        }

        public void PopulateUIFromPersistence(string aPersistenceKey)
        {

            _theFilterModel = getDefaultEventFilterModel(aPersistenceKey);
            AddFilterControls();
            populateUIFromModel(_theFilterModel);
            _theApplyFilterButton.Enabled = false;
            AddChangeTracker();
        }

        #endregion

        #region Private methods
        /// <summary>
        /// Monitoring changes.
        /// </summary>
        private void AddChangeTracker()
        {
            if (_theChangeTracker != null)
            {
                _theChangeTracker.RemoveChangeHandlers();
            }
            _theChangeTracker = new TrafodionChangeTracker(_theTopPanel);
            _theChangeTracker.OnChangeDetected += new TrafodionChangeTracker.ChangeDetected(ChangeTracker_OnChangeDetected);
            _theChangeTracker.EnableChangeEvents = true;
        }

        /// <summary>
        /// Event handler for change detected.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ChangeTracker_OnChangeDetected(object sender, System.EventArgs e)
        {
            UpdateButtons();
            UpdateCheckBoxes(sender);
        }

        private void UpdateButtons()
        {
            EventFilterModel modelFromUI = populateFilterFromUI();
            UpdateButtons(!_theFilterModel.Equals(modelFromUI));
        }
        private void UpdateButtons(bool modelChanged)
        {
            _theApplyFilterButton.Enabled = (modelChanged && _theButtonsEnabled);
            _theResestButton.Enabled = _theButtonsEnabled;
        }

        private void UpdateCheckBoxes(object sender)
        {
            if (sender is CheckedListBox)
            {
                CheckedListBox aListBox = sender as CheckedListBox;
                if (aListBox == _theSubsystemsListbox)
                {
                    _theSubSystemSelectAllCheckBox.CheckState = (aListBox.CheckedItems.Count == aListBox.Items.Count) ? CheckState.Checked : (aListBox.CheckedItems.Count > 0) ? CheckState.Indeterminate : CheckState.Unchecked;
                }
                else if (aListBox == _theSeverityListBox)
                {
                    _theSeveritySelectAllCheckBox.CheckState = (aListBox.CheckedItems.Count == aListBox.Items.Count) ? CheckState.Checked : (aListBox.CheckedItems.Count > 0) ? CheckState.Indeterminate : CheckState.Unchecked;
                }
            }
        }

        private EventFilterModel getDefaultEventFilterModel(string aPersistenceKey)
        {
            EventFilterModel filterModel = null;
            filterModel = Persistence.Get(aPersistenceKey) as EventFilterModel;
            if (filterModel == null)
            {
                filterModel = getNewEventFilterModel();
            }
            else
            {
                // Sanitize the timestamp
                if ((filterModel.TheStartTime.CompareTo(DateTime.MinValue) == 0) ||
                    (filterModel.TheStartTime.CompareTo(DateTime.MaxValue) == 0))
                {
                    filterModel.TheStartTime = DateTime.Today;
                }

                if ((filterModel.TheEndTime.CompareTo(DateTime.MinValue) == 0) ||
                    (filterModel.TheEndTime.CompareTo(DateTime.MaxValue) == 0))
                {
                    filterModel.TheEndTime = DateTime.Now;
                }
            }
            return filterModel;
        }

        private EventFilterModel getNewEventFilterModel()
        {
            EventFilterModel filterModel =  new EventFilterModel();
            filterModel.SeverityFilters = new List<NameValuePair>();
            filterModel.SubSystemFilters = new List<NameValuePair>();
            filterModel.EventIds = "";
            filterModel.ProcessIds = "";
            filterModel.ProcessNames = "";
            filterModel.TimeRange = TimeRangeHandler.Range.Last1Hour;
            filterModel.CurrentTime = true;
            filterModel.TheEndTime = DateTime.Now;
            filterModel.TheStartTime = DateTime.Today;
            filterModel.MessaegeFilter = "";
            filterModel.CaseSensitive = false;
            filterModel.MessageFilterCondition = new NameValuePair(_theEventDetails.MessageFilters[0], _theEventDetails.MessageFilters[0]);
            return filterModel;
        }

        //Helper method to populate the UI from the  model
        private void populateUIFromModel(EventFilterModel aEventFilterModel)
        {
            EventFilterModel filterModel = aEventFilterModel;

            //set sub-system filters
            //Reset all of the check boxes to start with
            for(int i = 0; i < _theSubsystemsListbox.Items.Count; i++)
            {
                _theSubsystemsListbox.SetItemChecked(i, false);
            }
            _theSubSystemSelectAllCheckBox.Checked = false;
            foreach (NameValuePair nvp in filterModel.SubSystemFilters)
            {
                for (int i = 0; i < _theSubsystemsListbox.Items.Count; i++)
                {
                    if (_theSubsystemsListbox.Items[i].Equals(nvp))
                    {
                        _theSubsystemsListbox.SetItemChecked(i, true);
                        break;
                    }
                }
            }
            _theSubSystemSelectAllCheckBox.CheckState = (_theSubsystemsListbox.CheckedItems.Count == _theSubsystemsListbox.Items.Count) 
                ? CheckState.Checked 
                : (_theSubsystemsListbox.CheckedItems.Count > 0) ? CheckState.Indeterminate : CheckState.Unchecked;


            //set severity filters
            for (int i = 0; i < _theSeverityListBox.Items.Count; i++)
            {
                _theSeverityListBox.SetItemChecked(i, false);
            }
            _theSeveritySelectAllCheckBox.Checked = false;
            foreach (NameValuePair nvp in filterModel.SeverityFilters)
            {
                for (int i = 0; i < _theSeverityListBox.Items.Count; i++)
                {
                    if (_theSeverityListBox.Items[i].Equals(nvp))
                    {
                        _theSeverityListBox.SetItemChecked(i, true);
                        break;
                    }
                }
            }
            _theSeveritySelectAllCheckBox.CheckState = (_theSeverityListBox.CheckedItems.Count == _theSeverityListBox.Items.Count)
                ? CheckState.Checked
                : (_theSeverityListBox.CheckedItems.Count > 0) ? CheckState.Indeterminate : CheckState.Unchecked;


            //set eventids
            _theEventIdText.Text = filterModel.EventIds.Trim();

            //set process ids
            setProcessidText(filterModel.ProcessIds);

            //set process names
            _theProcessNameText.Text = filterModel.ProcessNames.Trim();

            //set time filters
            _theTimeRangeCombo.SelectedItem = new TimeRangeHandler(filterModel.TimeRange);
            _theEndTime.Value = filterModel.TheEndTime;
            _theStartTime.Value = filterModel.TheStartTime;
            _theCurrentTimeCheckbox.Checked = filterModel.CurrentTime;

            //Message filters
            _theEventText.Text = filterModel.MessaegeFilter.Trim();
            _theInCaseSensitiveCheckBox.Checked = !filterModel.CaseSensitive;
            _theMessageFilterConditionCombo.SelectedItem = filterModel.MessageFilterCondition;
        }

        private void setProcessidText(string text)
        {
            _theProcessIdText.Text = text.Trim();
            _theToolTip.SetToolTip(_theProcessIdText, _theProcessIdText.Text);
        }
        //Helper method to populate the model from the UI
        private EventFilterModel populateFilterFromUI()
        {
            EventFilterModel filterModel = new EventFilterModel();
            //get sub-system filters
            foreach (object obj in _theSubsystemsListbox.CheckedItems)
            {
                filterModel.SubSystemFilters.Add(obj as NameValuePair);
            }

            filterModel.SubSystemAllChecked = (_theSubsystemsListbox.Items.Count == _theSubsystemsListbox.CheckedItems.Count);
            ////modify the case when all are checked
            //if (_theSubsystemsListbox.Items.Count == _theSubsystemsListbox.CheckedItems.Count)
            //{
            //    //clear the filtering since all checked is the same as none checked.
            //    filterModel.SubSystemFilters.Clear();
            //}

            //get severity filters
            foreach (object obj in _theSeverityListBox.CheckedItems)
            {
                filterModel.SeverityFilters.Add(obj as NameValuePair);
            }

            ////modify the case when all are checked
            //if (_theSubsystemsListbox.Items.Count == _theSubsystemsListbox.CheckedItems.Count)
            //{
            //    filterModel.SeverityFilters.Clear();
            //}

            filterModel.SeverityAllChecked = (_theSeverityListBox.Items.Count == _theSeverityListBox.CheckedItems.Count);

            //get eventids
            filterModel.EventIds = _theEventIdText.Text.Trim();

            //get process ids
            filterModel.ProcessIds = _theProcessIdText.Text.Trim();

            //get process names
            filterModel.ProcessNames = _theProcessNameText.Text.Trim();
            
            //get time filters
            filterModel.TimeRange = ((TimeRangeHandler)_theTimeRangeCombo.SelectedItem).TheRange;
            filterModel.TheEndTime = _theEndTime.Value;
            filterModel.TheStartTime = _theStartTime.Value;
            filterModel.CurrentTime = _theCurrentTimeCheckbox.Checked;

            //Message filters
            filterModel.MessaegeFilter = _theEventText.Text.Trim();
            filterModel.CaseSensitive = !_theInCaseSensitiveCheckBox.Checked;
            filterModel.MessageFilterCondition = (NameValuePair)_theMessageFilterConditionCombo.SelectedItem;

            return filterModel;
        }
        /// <summary>
        /// Initializes the contol with the default values 
        /// </summary>
        private void AddFilterControls()
        {
            this._theSubsystemsListbox.Items.Clear();
            EventDetails details = _theEventDetails;
            
            //Set the sub-systems. 
            this._theSubsystemsListbox.Items.Clear();
            ArrayList subSystems = details.SubSystems;
            if (subSystems != null)
            {
                foreach (ComponentModel cm in subSystems)
                {
                    this._theSubsystemsListbox.Items.Add(new NameValuePair(cm.ComponentName, (uint)cm.Component));
                }
            }
            else
            {
                subSystems = new ArrayList();
            }

            //Set the severity
            this._theSeverityListBox.Items.Clear();
            ArrayList severities = details.Severities;
            if (severities != null)
            {
                foreach (SeverityModel sm in severities)
                {
                    this._theSeverityListBox.Items.Add(new NameValuePair(sm.SeverityName, sm.Severity));
                }
            }
            else
            {
                severities = new ArrayList();
            }

            //Set the time params
            _theEndTime.Value = DateTime.Now;
            _theStartTime.Value = DateTime.Today;
            _theCurrentTimeCheckbox.Checked = true;

            this._theTimeRangeCombo.Items.Clear();
            for (int i = 0; i < details.TimeRanges.Length; i++)
            {
                this._theTimeRangeCombo.Items.Add(details.TimeRanges[i]);
            }
            this._theTimeRangeCombo.SelectedIndex = 0;

            //Set the message Filter
            _theMessageFilterConditionCombo.Items.Clear();
            for (int i = 0; i < details.MessageFilters.Length; i++)
            {
                this._theMessageFilterConditionCombo.Items.Add(new NameValuePair(details.MessageFilters[i], details.MessageFilters[i]));
            }
            this._theMessageFilterConditionCombo.SelectedIndex = 0;
        }


        private void _theTimeRangeCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            TimeRangeHandler timeHandler = _theTimeRangeCombo.SelectedItem as TimeRangeHandler;
            if (timeHandler != null)
            {
                _theEndTime.Enabled = timeHandler.IsCustomRange && (!_theCurrentTimeCheckbox.Checked);
                _theStartTime.Enabled = timeHandler.IsCustomRange;
                _theCurrentTimeCheckbox.Enabled = (timeHandler.IsCustomRange) ;
                //if not custom range, reset the values from model
                if ((!timeHandler.IsCustomRange) && (_theFilterModel != null))
                {
                    _theEndTime.Value = _theFilterModel.TheEndTime;
                    _theStartTime.Value = _theFilterModel.TheStartTime;
                    _theCurrentTimeCheckbox.Checked = _theFilterModel.CurrentTime;
                }
            }            
        }

        private void _theCurrentTimeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            TimeRangeHandler timeHandler = _theTimeRangeCombo.SelectedItem as TimeRangeHandler;
            if (timeHandler != null)
            {
                _theEndTime.Enabled = timeHandler.IsCustomRange && (!_theCurrentTimeCheckbox.Checked);
            }
        }

        private void _theSeveritySelectAllCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if ((((CheckBox)sender).CheckState == CheckState.Checked) || (((CheckBox)sender).CheckState == CheckState.Unchecked))
            {
                CheckListBox(_theSeverityListBox, _theSeveritySelectAllCheckBox.Checked);
            }
        }

        private void _theSubSystemSelectAllCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if ((((CheckBox)sender).CheckState == CheckState.Checked) || (((CheckBox)sender).CheckState == CheckState.Unchecked))
            {
                CheckListBox(_theSubsystemsListbox, _theSubSystemSelectAllCheckBox.Checked);
            }
        }

        private void CheckListBox(CheckedListBox aListbox, bool isChecked)
        {
            for (int i = 0; i < aListbox.Items.Count; i++)
            {
                aListbox.SetItemChecked(i, isChecked);
            }
        }
        private void _theResestButton_Click(object sender, EventArgs e)
        {
            EventFilterModel modelFromUI = populateFilterFromUI();
            _theFilterModel = getNewEventFilterModel();
            populateUIFromModel(_theFilterModel);
            UpdateButtons(!modelFromUI.Equals(_theFilterModel));
        }



        private void _theApplyFilterButton_Click(object sender, EventArgs e)
        {
            _theFilterModel = populateFilterFromUI();
            ArrayList errorList = _theFilterModel.IsValid();
            if (errorList.Count == 0)
            {
                UpdateButtons();
                //this will clean up the junk input
                populateUIFromModel(_theFilterModel);
                _OnFilterChangedImpl(_theFilterModel);
            }
            else
            {
                MessageBox.Show(errorList[0].ToString(), "Invalid input");
            }
        }



        private void validateInts(KeyPressEventArgs e)
        {
            if (Char.IsDigit(e.KeyChar))
            {
                // Digits are OK
            }

            else if (e.KeyChar == '\b')
            {
                // Backspace key is OK
            }
            else if (e.KeyChar == ',')
            {
                // comma key is OK
            }
            else if ((ModifierKeys & (Keys.Control | Keys.Alt)) != 0)
            {
                // Let the edit control handle control and alt key combinations
            }
            else
            {
                // Consume this invalid key and beep
                e.Handled = true;
                // MessageBeep();
            }

        }
        #endregion

        private void _theProcessIdText_KeyPress(object sender, KeyPressEventArgs e)
        {
            validateInts(e);
        }

        private void _theEventIdText_KeyPress(object sender, KeyPressEventArgs e)
        {
            validateInts(e);
        }

        private void _theProcessIdText_TextChanged(object sender, EventArgs e)
        {
            
        }

        private void _theProcessIdText_MouseHover(object sender, EventArgs e)
        {
            _theToolTip.Show(_theProcessIdText.Text, _theProcessIdText);
        }

        private void _theProcessNameText_MouseHover(object sender, EventArgs e)
        {
            _theToolTip.Show(_theProcessNameText.Text, _theProcessNameText);
        }

        private void _theEventIdText_MouseHover(object sender, EventArgs e)
        {
            _theToolTip.Show(_theEventIdText.Text, _theEventIdText);
        }
    }
}
