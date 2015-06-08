#region Copyright info
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
    /// the audit log data.
    /// </summary>
    public partial class AuditLogFilterPanel : UserControl
    {
        #region Member Variables
        public delegate void OnUpdateButtonsHandle();
        public event OnUpdateButtonsHandle OnUpdateButtonsImpl;

        public delegate void OnFilterChanged(AuditLogFilterModel aFilterModel);
        private OnFilterChanged _OnFilterChangedImpl = null;
        private TrafodionChangeTracker _theChangeTracker = null;
        AuditLogFilterModel _theFilterModel = null;
        ConnectionDefinition _theConnectionDefinition;
        private bool _theButtonsEnabled = true;
        private bool _modelChanged = false;
        AuditLogDetails _theAuditLogDetails;
        private bool _forLiveEvents = false;
        #endregion

        #region Constructor
        /// <summary>
        /// Default constructor
        /// </summary>
        public AuditLogFilterPanel()
        {
            InitializeComponent();
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
                    _theAuditLoggingTimeGroupBox.Text = "Audit Logging Time (Server Local Time in 24-Hour Format)";               
                }
            }
        }

        public AuditLogDetails TheAuditLogDetails
        {
            get { return _theAuditLogDetails; }
            set { _theAuditLogDetails = value; }
        }

        public AuditLogFilterModel FilterModel
        {
            get { return _theFilterModel; }
            set { _theFilterModel = value; }
        }


        public bool TheModelChanged
        {
            get { return _modelChanged; }
           
        }
        public bool TheButtonsEnabled
        {
            get { return _theButtonsEnabled; }           
        }

        #endregion


        #region Public Methods

        public OnFilterChanged OnFilterChangedImpl
        {
            set { _OnFilterChangedImpl = value; }
        }

        public string GetFilterDisplayString(AuditLogsDataHandler anAuditLogsDataHandler)
        {
            return _theFilterModel.GetFormattedFilterString(anAuditLogsDataHandler);
        }



        public void SetFilterValues(string columnName, object value)
        {
            AuditLogFilterModel filterModel = populateFilterFromUI();
            if (columnName.Equals(AuditLogFilterModel.AUDITTYPE))
            {
            }
            else if (columnName.Equals(AuditLogFilterModel.USER_ID))
            {
                filterModel.AddUserId(value + "");
            }
            else if (columnName.Equals(AuditLogFilterModel.EXTERNALUSERNAME))
            {
                filterModel.AddExternalUserNames(value + "");
            }
            else if (columnName.Equals(AuditLogFilterModel.INTERNALUSERNAME))
            {
                filterModel.AddInternalUserNames(value + "");
            }
            else if (columnName.Equals(AuditLogFilterModel.OUTCOME))
            {
                
            }
            else if (columnName.Equals(AuditLogFilterModel.SESSION_ID))
            {
                filterModel.AddSessionIds(value + "");
            }
            else if (columnName.Equals(AuditLogFilterModel.TRANSACTION_ID))
            {
                filterModel.AddTransactionIds(value + "");
            }
            else if (columnName.Equals(AuditLogFilterModel.SQLCODE))
            {
                filterModel.AddSQLCodes(value + "");
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

            _theFilterModel = getDefaultAuditLogFilterModel(aPersistenceKey);
            AddFilterControls();
            populateUIFromModel(_theFilterModel);
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
            AuditLogFilterModel modelFromUI = populateFilterFromUI();
            UpdateButtons(!_theFilterModel.Equals(modelFromUI));
        }
        private void UpdateButtons(bool modelChanged)
        {
            _modelChanged = modelChanged;
            if (OnUpdateButtonsImpl != null)
            {
                OnUpdateButtonsImpl();
            }
        }

        private void UpdateCheckBoxes(object sender)
        {
            if (sender is CheckedListBox)
            {
                CheckedListBox aListBox = sender as CheckedListBox;
                if (aListBox == _theAuditTypeCheckedListBox)
                {
                    _theSelectAllAuditType.CheckState = (aListBox.CheckedItems.Count == aListBox.Items.Count) ? 
                        CheckState.Checked : (aListBox.CheckedItems.Count > 0) ? CheckState.Indeterminate : CheckState.Unchecked;
                }
                else if (aListBox == _theOutcomeCheckListBox)
                {
                   
                }
            }
        }

        private AuditLogFilterModel getDefaultAuditLogFilterModel(string aPersistenceKey)
        {
            AuditLogFilterModel filterModel = null;
            filterModel = Persistence.Get(aPersistenceKey) as AuditLogFilterModel;
            if (filterModel == null)
            {
                filterModel = getNewAuditLogFilterModel();
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

        private AuditLogFilterModel getNewAuditLogFilterModel()
        {
            AuditLogFilterModel filterModel = new AuditLogFilterModel();
            filterModel.AuditTypeFilters = new List<NameValuePair>();
            //set default audit types as all of audit types.
            foreach (NameValuePair nvp in _theAuditLogDetails.AuditTypes)
            {
                filterModel.AuditTypeFilters.Add(new NameValuePair(nvp.Name, nvp.Value));
            }
            filterModel.UserIds = "";
            filterModel.ExternalUserNames = "";
            filterModel.InternalUserNames = "";
            filterModel.TransactionIds = "";            
            filterModel.SessionIds = "";
            filterModel.MessaegeFilter = "";
            filterModel.OutcomesFilters = new List<NameValuePair>();
            filterModel.TimeRange = TimeRangeHandler.Range.Last1Hour;
            filterModel.CurrentTime = true;
            filterModel.TheEndTime = DateTime.Now;
            filterModel.TheStartTime = DateTime.Today;
            filterModel.CaseSensitive = false;
            filterModel.MessageFilterCondition = new NameValuePair(_theAuditLogDetails.MessageFilters[0], _theAuditLogDetails.MessageFilters[0]);
            return filterModel;
        }

        //Helper method to populate the UI from the  model
        private void populateUIFromModel(AuditLogFilterModel anAuditLogFilterModel)
        {
            AuditLogFilterModel filterModel = anAuditLogFilterModel;


            for (int i = 0; i < _theAuditTypeCheckedListBox.Items.Count; i++)
            {
                _theAuditTypeCheckedListBox.SetItemChecked(i, false);
            }
            _theSelectAllAuditType.Checked = false;
            foreach (NameValuePair nvp in filterModel.AuditTypeFilters)
            {
                for (int i = 0; i < _theAuditTypeCheckedListBox.Items.Count; i++)
                {
                    if (_theAuditTypeCheckedListBox.Items[i].Equals(nvp))
                    {
                        _theAuditTypeCheckedListBox.SetItemChecked(i, true);
                        break;
                    }
                }
            }
            _theSelectAllAuditType.CheckState = (_theAuditTypeCheckedListBox.CheckedItems.Count == _theAuditTypeCheckedListBox.Items.Count)
                ? CheckState.Checked
                : (_theAuditTypeCheckedListBox.CheckedItems.Count > 0) ? CheckState.Indeterminate : CheckState.Unchecked;

            //set userids
            //_theUserIdText.Text = filterModel.UserIds.Trim();

            //set external user name
            _theExternalUserNameText.Text = filterModel.ExternalUserNames.Trim();

            _theInternalUserNameText.Text = filterModel.InternalUserNames.Trim();

            _theSessionIDText.Text = filterModel.SessionIds.Trim();

            _theTransactionIDText.Text = filterModel.TransactionIds.Trim();

            _theSQLCodeText.Text = filterModel.SQLCodeIds.Trim();

            //set Outcome
  
            for (int i = 0; i < _theOutcomeCheckListBox.Items.Count; i++)
            {
                _theOutcomeCheckListBox.SetItemChecked(i, false);
            }

            foreach (NameValuePair nvp in filterModel.OutcomesFilters)
            {
                for (int i = 0; i < _theOutcomeCheckListBox.Items.Count; i++)
                {
                    if (_theOutcomeCheckListBox.Items[i].Equals(nvp))
                    {
                        _theOutcomeCheckListBox.SetItemChecked(i, true);
                        break;
                    }
                }
            }
            //set time filters
            _theTimeRangeCombo.SelectedItem = new TimeRangeHandler(filterModel.TimeRange);
            _theEndTime.Value = filterModel.TheEndTime;
            _theStartTime.Value = filterModel.TheStartTime;
            _theCurrentTimeCheckbox.Checked = filterModel.CurrentTime;

            //Message filters
            _theAuditLogMessageText.Text = filterModel.MessaegeFilter.Trim();
            _theInCaseSensitiveCheckBox.Checked = !filterModel.CaseSensitive;
            _theMessageFilterConditionCombo.SelectedItem = filterModel.MessageFilterCondition;
         
        }


        //Helper method to populate the model from the UI
        private AuditLogFilterModel populateFilterFromUI()
        {
            AuditLogFilterModel filterModel = new AuditLogFilterModel();

            //get audit type filters
            foreach (object obj in _theAuditTypeCheckedListBox.CheckedItems)
            {
                filterModel.AuditTypeFilters.Add(obj as NameValuePair);
            }

            filterModel.AuditTypeAllChecked = (_theAuditTypeCheckedListBox.Items.Count == _theAuditTypeCheckedListBox.CheckedItems.Count);

            //get user ids
            filterModel.UserIds = ""; //_theUserIdText.Text.Trim();

            filterModel.ExternalUserNames = _theExternalUserNameText.Text.Trim().ToUpper();

            filterModel.InternalUserNames = _theInternalUserNameText.Text.Trim().ToUpper();

            filterModel.SessionIds = _theSessionIDText.Text.Trim();

            filterModel.TransactionIds = _theTransactionIDText.Text.Trim();

            filterModel.SQLCodeIds = _theSQLCodeText.Text.Trim();

            //get outcome filters
            foreach (object obj in _theOutcomeCheckListBox.CheckedItems)
            {
                filterModel.OutcomesFilters.Add(obj as NameValuePair);
            }

            //get time filters
            filterModel.TimeRange = ((TimeRangeHandler)_theTimeRangeCombo.SelectedItem).TheRange;
            filterModel.TheEndTime = _theEndTime.Value;
            filterModel.TheStartTime = _theStartTime.Value;
            filterModel.CurrentTime = _theCurrentTimeCheckbox.Checked;

            //Message filters
            filterModel.MessaegeFilter = _theAuditLogMessageText.Text.Trim();
            filterModel.CaseSensitive = !_theInCaseSensitiveCheckBox.Checked;
            filterModel.MessageFilterCondition = (NameValuePair)_theMessageFilterConditionCombo.SelectedItem;

            return filterModel;

        }
        /// <summary>
        /// Initializes the contol with the default values 
        /// </summary>
        private void AddFilterControls()
        {
            //this._theSubsystemsListbox.Items.Clear();
            AuditLogDetails details = _theAuditLogDetails;

            //Set the audit type
            this._theAuditTypeCheckedListBox.Items.Clear();
            ArrayList auditTypes = details.AuditTypes;
            if (auditTypes != null)
            {
                foreach (NameValuePair nvp in auditTypes)
                {
                    this._theAuditTypeCheckedListBox.Items.Add(new NameValuePair(nvp.Name, nvp.Value));
                }
            }
            else
            {
                auditTypes = new ArrayList();
            }

            //Set the outcome
            this._theOutcomeCheckListBox.Items.Clear();
            ArrayList outcomes = details.Outcomes;
            if (outcomes != null)
            {
                foreach (NameValuePair nvp in outcomes)
                {
                    this._theOutcomeCheckListBox.Items.Add(new NameValuePair(nvp.Name, nvp.Value));
                }
            }
            else
            {
                outcomes = new ArrayList();
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
                _theCurrentTimeCheckbox.Enabled = (timeHandler.IsCustomRange);
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
            _theEndTime.Enabled = (!_theCurrentTimeCheckbox.Checked);
        }

        private void _theSelectAllAuditType_CheckedChanged(object sender, EventArgs e)
        {

            if ((((CheckBox)sender).CheckState == CheckState.Checked) || (((CheckBox)sender).CheckState == CheckState.Unchecked))
            {
                CheckListBox(_theAuditTypeCheckedListBox, _theSelectAllAuditType.Checked);
            }

        }

        private void AuditLogFilterPanel_SizeChanged(object sender, EventArgs e)
        {
            if (this.Width > 220)
            {
                _theAuditTypeCheckedListBox.ColumnWidth = 140;
                _theAuditTypeCheckedListBox.MultiColumn = true;
                _theAuditTypeCheckedListBox.HorizontalScrollbar = false;
            }
            else
            {
                _theAuditTypeCheckedListBox.MultiColumn = true;
                _theAuditTypeCheckedListBox.HorizontalScrollbar = true;
            }

        }
        private void CheckListBox(CheckedListBox aListbox, bool isChecked)
        {
            for (int i = 0; i < aListbox.Items.Count; i++)
            {
                aListbox.SetItemChecked(i, isChecked);
            }
        }

        public void Reset()
        {
            AuditLogFilterModel modelFromUI = populateFilterFromUI();
            _theFilterModel = getNewAuditLogFilterModel();
            populateUIFromModel(_theFilterModel);
            UpdateButtons(!modelFromUI.Equals(_theFilterModel));
        }

        public void ApplyFilter()
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
                MessageBox.Show(errorList[0].ToString(), "Invalid input",MessageBoxButtons.OK,MessageBoxIcon.Exclamation);
            }
        }

        #endregion



 

    }

   
}
