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
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class RoleSelectionPanel : UserControl, IsValidateable
    {
        public enum RoleSelectionMode { Default, Additional };
        String _theTitle = "Roles";
        private static readonly string _theDirectoryServersConfigName = "_theRolesWidgetConfig";
        RoleSelectorDataHandler _theDataDisplayHandler = null;
        UniversalWidgetConfig _theWidgetConfig = null;
        GenericUniversalWidget _theWidget = null;
        ConnectionDefinition _theConnectionDefinition;
        TrafodionIGrid _theDSGrid = null;
        Connection _conn = null;
        User.EditMode _Mode = User.EditMode.Create;
        List<string> _additionalRoles = new List<string>();
        RoleSelectionMode _selectionMode = RoleSelectionMode.Default;
        private delegate void HandleEvent(object obj, EventArgs e);  


        public string DefaultRole
        {
            get { return _theDefaultRoleText.Text.Trim().ToUpper(); }
            set { _theDefaultRoleText.Text = value; }
        }

        public List<string> AdditionalRoles
        {
            set 
            {
                _additionalRoles = value;
                _theAdditionalRolesTxt.Text = User.GetAdditionalRoleString(_additionalRoles);
            }
            get 
            {
                loadAdditionalRolesFromText();
                return _additionalRoles; 
            }
        }

        public User.EditMode Mode
        {
            get { return _Mode; }
            set
            {
                _Mode = value;
                this._theDefaultRolePanel.Visible = ((_Mode == User.EditMode.Create) || (_Mode == User.EditMode.CreateLike));
            }
        }

        public RoleSelectionMode SelectionMode
        {
            get { return _selectionMode; }
            set { _selectionMode = value; }
        }

        public void AddAdditionalRole(string aRole)
        {
            string role = aRole.Trim().ToUpper();
            if (_additionalRoles == null)
            {
                _additionalRoles = new List<string>();
            }
            //TODO: deal with case sensitivity
            if (_additionalRoles.Contains(role))
            {
                _additionalRoles.Remove(role);
            }
            _additionalRoles.Add(role);
        }

        public RoleSelectionPanel()
        {
            InitializeComponent();
        }

        public RoleSelectionPanel(ConnectionDefinition aConnectionDefinition) : this()
        {            
            _theConnectionDefinition = aConnectionDefinition;
            ShowWidgets();
        }

        public List<string> IsValid()
        {
            List<string> ret = new List<string>();
            List<string> additionalRoles = AdditionalRoles;

            if ((DefaultRole.Length == 0) && (additionalRoles.Count == 0))
            {
                ret.Add("Either the default role or an additional role has to be provided");
            }

            if (DefaultRole.Length > ConnectionDefinition.ROLE_NAME_MAX_LENGTH)
            {
                ret.Add(string.Format("Default role \"{0}\" cannot be more than {1} characters long", DefaultRole, ConnectionDefinition.ROLE_NAME_MAX_LENGTH));
            }

            
            foreach (string additionalRole in additionalRoles)
            {
                if (additionalRole.Length > ConnectionDefinition.ROLE_NAME_MAX_LENGTH)
                {
                    ret.Add(string.Format("Additional role \"{0}\" cannot be more than {1} characters long", additionalRole, ConnectionDefinition.ROLE_NAME_MAX_LENGTH));
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


        private void loadAdditionalRolesFromText()
        {
            string additionalRoles = _theAdditionalRolesTxt.Text.Trim().ToUpper();
            _additionalRoles.Clear();
            if (additionalRoles.Length > 0)
            {
                string[] splitRoles = additionalRoles.Split(new char[] { ',' });               
                foreach (string role in splitRoles)
                {
                    if (role.Trim().Length > 0)
                    {
                        AddAdditionalRole(role);
                    }
                }
            }
        }

        private void ShowWidgets()
        {
            // Remove all current contents and add the alerts widget
            this._theRolesGroupBox.Controls.Clear();

            _theWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
            _theWidgetConfig.Name = _theTitle;
            _theWidgetConfig.Title = _theTitle;
            _theWidgetConfig.ShowProperties = false;
            _theWidgetConfig.ShowToolBar = true;
            _theWidgetConfig.ShowChart = false;
            _theWidgetConfig.ShowTimerSetupButton = false;
            _theWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            DatabaseDataProviderConfig _dbConfig = _theWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            _dbConfig.SQLText = string.Format("CALL MANAGEABILITY.TRAFODION_SPJ.SecGetRolesAvailable('{0}', '{1}', '{2}')", _theConnectionDefinition.RoleName, "New" + DateTime.Now.Ticks, "ADD");
            //_dbConfig.SQLText = "select ROLENAME as role_name FROM MANAGEABILITY.TRAFODION_SECURITY.ROLE_INFO ORDER BY ROLENAME ASC";
            _theWidgetConfig.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;

            _theWidget = new GenericUniversalWidget();
            _theWidget.DataProvider = new DatabaseDataProvider(_dbConfig);

            //Set the display properties of the widget and add it to the control
            _theDataDisplayHandler = new RoleSelectorDataHandler(this);
            _theWidget.DataDisplayControl.DataDisplayHandler = _theDataDisplayHandler;
            _theWidget.UniversalWidgetConfiguration = _theWidgetConfig;
            _theWidget.Dock = DockStyle.Fill;
            this._theRolesGroupBox.Controls.Add(_theWidget);

            _theDSGrid = ((TabularDataDisplayControl)_theWidget.DataDisplayControl).DataGrid;
            _theDSGrid.DoubleClickHandler = RoleSelection_Handler;
            _theDSGrid.RowMode = true;

            //Disable the export buttons so it does not show up within the universal widget panel
            ((TabularDataDisplayControl)_theWidget.DataDisplayControl).ShowExportButtons = false;

            //Add event handlers to deal with data provider events
            AddHandlers();

            //Start it.
            _theWidget.StartDataProvider();
            this.Disposed += new EventHandler(RolesControl_Disposed);
        }

        void RolesControl_Disposed(object sender, EventArgs e)
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
        
        private void RoleSelection_Handler(int rowIndex)
        {
            if (_selectionMode == RoleSelectionMode.Default)
            {
                setDefaultRole();
            }
            else if (_selectionMode == RoleSelectionMode.Additional)
            {
                setAdditionalRoles();
            }
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
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

        private void _theDefaultRoleBtn_Click(object sender, EventArgs e)
        {
            setDefaultRole();
        }

        private void setDefaultRole()
        {
            string[] selectedRoles = getSelectedRoles();
            if (selectedRoles.Length > 0)
            {
                if (selectedRoles.Length > 1)
                {
                    DialogResult result = MessageBox.Show(string.Format("You can have only one default role. Hence '{0}' will be used.\n Do you wish to continue?", selectedRoles[0].Trim()),
                        "Multiple roles selected for default Role", MessageBoxButtons.OKCancel, MessageBoxIcon.Question);
                    if (result == DialogResult.OK)
                    {
                        _theDefaultRoleText.Text = selectedRoles[0].Trim().ToUpper();
                    }
                    else
                    {
                        return;
                    }
                }
                else
                {
                    _theDefaultRoleText.Text = selectedRoles[0].Trim();
                }
            }            

        }

        private void _theAdditionalRoleBtn_Click(object sender, EventArgs e)
        {
            setAdditionalRoles();
        }

        private void setAdditionalRoles()
        {
            string[] selectedRoles = getSelectedRoles();

            if (selectedRoles.Length > 0)
            {
                //Load the roles from text first. This is to account any changes that the 
                //user might have typed in
                loadAdditionalRolesFromText();

                foreach (string role in selectedRoles)
                {
                    AddAdditionalRole(role);
                }
                _theAdditionalRolesTxt.Text = User.GetAdditionalRoleString(_additionalRoles);
            }

        }

        private string[] getSelectedRoles()
        {
            int rowCount  = _theDSGrid.SelectedRowIndexes.Count;
            string[] ret = new string[rowCount];
            for (int i = 0; i < rowCount; i++)
            {
                ret[i] = _theDSGrid.Rows[_theDSGrid.SelectedRowIndexes[i]].Cells[0].Value as string;
            }
            return ret;
        }

        private void _theTypedRoleName_TextChanged(object sender, EventArgs e)
        {
            string text = _theTypedRoleName.Text.Trim();
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

        private void sanitizeRoles()
        {
            _theDefaultRoleText.Text = _theDefaultRoleText.Text.Trim().ToUpper();
            string defaultRole = DefaultRole;
            loadAdditionalRolesFromText();
            if (_additionalRoles.Contains(defaultRole))
            {
                _additionalRoles.Remove(defaultRole);
            }
            _theAdditionalRolesTxt.Text = User.GetAdditionalRoleString(_additionalRoles);
        }

        private void _theDefaultRoleText_Leave(object sender, EventArgs e)
        {
            sanitizeRoles();
        }

        private void _theAdditionalRolesTxt_Leave(object sender, EventArgs e)
        {
            sanitizeRoles();
        }
    }

    #region RoleSelectorDataHandler Class

    public class RoleSelectorDataHandler : TabularDataDisplayHandler
    {
        #region Fields
        private RoleSelectionPanel _theRoleSelectionPanel;
        string[] columns = new string[] { "Role Name"};
        enum ColumnPos { RoleName = 0};
        #endregion Fields


        #region Properties


        #endregion Properties


        #region Constructors

        public RoleSelectorDataHandler(RoleSelectionPanel aRoleSelectionPanel)
        {
            _theRoleSelectionPanel = aRoleSelectionPanel;
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

            //the data table has a single row and 1 column. All of the role names are 
            //separated by new line
            if ((aDataTable.Rows.Count == 1) && (aDataTable.Columns.Count == 1))
            {
                List<string> roleNames = getRoleNames((string)aDataTable.Rows[0][0]);
                foreach (string role in roleNames)
                {
                    DataRow dr = dataTable.NewRow();
                    dr[columns[0]] = role;
                    dataTable.Rows.Add(dr);
                }
            }
            else
            {
                //This code path should no get executed if we use the SPJ to get the role names.
                //However if we use the select statement to get the role names, this path will get 
                //executed.
                foreach (DataRow row in aDataTable.Rows)
                {
                    DataRow dr = dataTable.NewRow();
                    dr[columns[0]] = row[0];
                    dataTable.Rows.Add(dr);
                }
            }
            base.DoPopulate(aConfig, dataTable, aDataGrid);
            aDataGrid.AutoResizeCols = true;
            aDataGrid.GridLines.Mode = iGGridLinesMode.None;
        }

        private List<string> getRoleNames( string roleNameString)
        {
            List<string> ret = new List<string>();
            char[] delimiters = new char[] { '\r', '\n' };
            string[] parts = roleNameString.Split(delimiters, StringSplitOptions.RemoveEmptyEntries);
            for (int i = 0; i < parts.Length; i++)
            {
                ret.Add(parts[i]);
            }
            return ret;
        }
        #endregion Private methods
    }

    #endregion RoleSelectorDataHandler Class
}
