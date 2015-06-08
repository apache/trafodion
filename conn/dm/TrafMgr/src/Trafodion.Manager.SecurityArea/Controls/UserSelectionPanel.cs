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
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class UserSelectionPanel : UserControl, IsValidateable
    {
        String _theTitle = "Users";
        UserSelectorDataHandler _theDataDisplayHandler = null;
        UniversalWidgetConfig _theWidgetConfig = null;
        GenericUniversalWidget _theWidget = null;
        ConnectionDefinition _theConnectionDefinition;
        TrafodionIGrid _theDSGrid = null;
        string _roleName;
        TrafodionIGrid _usersListGrid = new TrafodionIGrid();
        private delegate void HandleEvent(object obj, EventArgs e);  

        public List<string> AdditionalUsers
        {
            get 
            {
                List<string> userNames = new List<string>();
                foreach (iGRow row in _usersListGrid.Rows)
                {
                    userNames.Add(row.Cells[0].Value as string);
                }
                return userNames; 
            }
        }

        public void AddAdditionalUser(string aUser)
        {
            string userName = aUser.Trim().ToUpper();

            //TODO: deal with case sensitivity
            if (!AdditionalUsers.Contains(userName))
            {
                iGRow row = _usersListGrid.Rows.Add();
                row.Cells[0].Value = userName;
                _usersListGrid.PerformAction(iGActions.DeselectAllRows);
                row.Selected = true;
            }
        }

        public UserSelectionPanel()
        {
            InitializeComponent();
            _roleName = "";

            _usersListGrid.Cols.Add("User Name");
            _usersListGrid.SelectionMode = iGSelectionMode.MultiExtended;
            _usersListGrid.RowMode = true;
            _usersListGrid.KeyUp += new KeyEventHandler(_usersListGrid_KeyUp);
            _usersListGrid.AutoResizeCols = true;
            _usersListGrid.Dock = DockStyle.Fill;
            _userListPanel.Controls.Add(_usersListGrid);
        }

        void _usersListGrid_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
            {
                DataTable dataTable = new DataTable();
                dataTable.Columns.Add(_usersListGrid.Cols[0].Text.ToString());

                List<string> selectedusers = new List<string>(_usersListGrid.SelectedRows.Count);
                foreach(iGRow row in _usersListGrid.SelectedRows)
                {
                    selectedusers.Add(row.Cells[0].Value as string);
                }

                foreach (iGRow row in _usersListGrid.Rows)
                {
                    if(!selectedusers.Contains(row.Cells[0].Value as string))
                    {
                        dataTable.Rows.Add(new object[] {row.Cells[0].Value as string});
                    }
                }
                _usersListGrid.Clear();
                _usersListGrid.FillWithData(dataTable);
            }
        }

        public UserSelectionPanel(String roleName, ConnectionDefinition aConnectionDefinition)
            : this()
        {
            _roleName = roleName;
            _theConnectionDefinition = aConnectionDefinition;
            ShowWidgets();
        }

        public List<string> IsValid()
        {
            List<string> ret = new List<string>();
            List<string> additionalUsers = AdditionalUsers;

            if (additionalUsers.Count == 0)
            {
                ret.Add("Atleast one user must be added");
            }
            else
            {

                foreach (string additionalUser in additionalUsers)
                {
                    if (additionalUser.Length > ConnectionDefinition.USER_NAME_MAX_LENGTH)
                    {
                        ret.Add(string.Format("Additional user \"{0}\" cannot be more than {1} characters long", additionalUser, ConnectionDefinition.USER_NAME_MAX_LENGTH));
                    }
                }
            }

            return ret;
        }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set
            {
                _theConnectionDefinition = value;
                if (_theWidgetConfig != null)
                {
                    _theWidgetConfig.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;
                }
            }
        }


        private void ShowWidgets()
        {
            // Remove all current contents and add the alerts widget
            this._theUsersGroupBox.Controls.Clear();

            _theWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
            _theWidgetConfig.Name = _theTitle;
            _theWidgetConfig.Title = _theTitle;
            _theWidgetConfig.ShowProperties = false;
            _theWidgetConfig.ShowToolBar = true;
            _theWidgetConfig.ShowChart = false;
            _theWidgetConfig.ShowTimerSetupButton = false;
            _theWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            DatabaseDataProviderConfig _dbConfig = _theWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            //_dbConfig.SQLText = string.Format("CALL MANAGEABILITY.TRAFODION_SPJ.SecGetUsersAvailable('{0}', '{1}', '{2}')", _theConnectionDefinition.UserName, "New" + DateTime.Now.Ticks, "ADD");
            _dbConfig.SQLText = string.Format("SELECT DISTINCT TRIM(USERNAME) as USER_NAME FROM MANAGEABILITY.TRAFODION_SECURITY.USER_INFO " +
                                                "WHERE USERTYPE != 2 AND USERNAME NOT IN "+
                                                "( SELECT USERNAME FROM MANAGEABILITY.TRAFODION_SECURITY.USER_INFO " +
                                                "WHERE ROLE = '{0}' ) ORDER BY 1 ASC", _roleName.Trim());

            _theWidgetConfig.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;

            _theWidget = new GenericUniversalWidget();
            _theWidget.DataProvider = new DatabaseDataProvider(_dbConfig);

            //Set the display properties of the widget and add it to the control
            _theDataDisplayHandler = new UserSelectorDataHandler(this);
            _theWidget.DataDisplayControl.DataDisplayHandler = _theDataDisplayHandler;
            _theWidget.UniversalWidgetConfiguration = _theWidgetConfig;
            _theWidget.Dock = DockStyle.Fill;
            this._theUsersGroupBox.Controls.Add(_theWidget);

            _theDSGrid = ((TabularDataDisplayControl)_theWidget.DataDisplayControl).DataGrid;
            _theDSGrid.DoubleClickHandler = UserSelection_Handler;
            _theDSGrid.RowMode = true;

            //Disable the export buttons so it does not show up within the universal widget panel
            ((TabularDataDisplayControl)_theWidget.DataDisplayControl).ShowExportButtons = false;

            //Add event handlers to deal with data provider events
            AddHandlers();

            //Start it.
            _theWidget.StartDataProvider();
            this.Disposed += new EventHandler(UsersControl_Disposed);
        }

        void UsersControl_Disposed(object sender, EventArgs e)
        {

        }
        /// <summary>
        /// Cleanup
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Remove the event handlers
                this.RemoveHandlers();
            }
        }
        
        private void UserSelection_Handler(int rowIndex)
        {
            setAdditionalUsers();
        }

        private void AddHandlers()
        {
            if (_theWidget != null && _theWidget.DataProvider != null)
            {
                //Associate the event handlers
                _theWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                _theWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _theWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;

            }
        }

        private void RemoveHandlers()
        {
            if (_theWidget != null && _theWidget.DataProvider != null)
            {
                //Remove the event handlers
                _theWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                _theWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
            }
        }

        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleError), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security, ex.StackTrace);
            }
        }

        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
        }

        private void _theAdditionalUsersBtn_Click(object sender, EventArgs e)
        {
            setAdditionalUsers();
        }

        private void setAdditionalUsers()
        {
            string[] selectedUsers = getSelectedUsers();

            if (selectedUsers.Length > 0)
            {
                foreach (string user in selectedUsers)
                {
                    AddAdditionalUser(user);
                }
            }
        }

        private string[] getSelectedUsers()
        {
            string[] ret = new string[_theDSGrid.SelectedRowIndexes.Count];
            for(int i=0;i < _theDSGrid.SelectedRowIndexes.Count;i++)
            {
                ret[i] = _theDSGrid.Rows[_theDSGrid.SelectedRowIndexes[i]].Cells[0].Value as string;
            }
            return ret;
        }

        private void _theTypedUserName_TextChanged(object sender, EventArgs e)
        {
            string text = _theTypedUserName.Text.Trim();
            if ((_theDSGrid != null) && (text.Length > 0))
            {
                int rowCount = _theDSGrid.Rows.Count;
                for (int row = 0; row < rowCount; row++)
                {
                    iGRow tempRow = _theDSGrid.Rows[row];
                    if (((string)tempRow.Cells[0].Value).StartsWith(text, StringComparison.InvariantCultureIgnoreCase))
                    {
                        _theDSGrid.PerformAction(iGActions.DeselectAllRows);
                        _theDSGrid.Rows[row].Selected = true;
                        _theDSGrid.SetCurRow(row);
                        break;
                    }
                }
            }
        }
    }

    #region UserSelectorDataHandler Class

    public class UserSelectorDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private UserSelectionPanel _theUserSelectionPanel;
        string[] columns = new string[] { "User Name"};
        enum ColumnPos { UserName = 0};

        #endregion Fields


        #region Properties


        #endregion Properties


        #region Constructors

        public UserSelectorDataHandler(UserSelectionPanel aUserSelectionPanel)
        {
            _theUserSelectionPanel = aUserSelectionPanel;
        }

        #endregion Constructors


        #region Public methods

        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            lock (this)
            {
                populate(aConfig, aDataTable, aDataGrid);
            }
        }

        #endregion Public methods


        #region Private methods

        //Popultes the datatable using the data returned by the data provider and displays it in the grid
        private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            if (null == aDataTable)
            {
                return;
            }

            DataTable dataTable = new DataTable();
            foreach (string columnName in columns)
            {
                dataTable.Columns.Add(columnName, typeof(string));
            }

            foreach (DataRow row in aDataTable.Rows)
            {
                DataRow dr = dataTable.NewRow();
                dr[columns[0]] = row[0];
                dataTable.Rows.Add(dr);
            }

            base.DoPopulate(aConfig, dataTable, aDataGrid);
            aDataGrid.AutoResizeCols = true;
            aDataGrid.GridLines.Mode = iGGridLinesMode.None;
        }
        #endregion Private methods
    }

    #endregion UserSelectorDataHandler Class
}
