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
using Trafodion.Manager.UserManagement.Model;
using TenTec.Windows.iGridLib;
using System.Drawing;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class RoleSelectionPanel : UserControl, IsValidateable
    {
        public enum RoleSelectionMode { Default, Additional };
        String _theTitle = "Roles";
        String _COLUMNNAME = "Role Name";
        private static readonly string _theDirectoryServersConfigName = "_theRolesWidgetConfig";
        RoleSelectorDataHandler _theDataDisplayHandler = null;
        UniversalWidgetConfig _theWidgetConfig = null;
        GenericUniversalWidget _theWidget = null;
        ConnectionDefinition _theConnectionDefinition;
        TrafodionIGrid _theDSGrid = null;
        TrafodionIGrid _selectedAdditionalRolesGrid = new TrafodionIGrid();
        Connection _conn = null;
        List<string> _additionalRoles = new List<string>();
        RoleSelectionMode _selectionMode = RoleSelectionMode.Default;

        #region Constructors
        public RoleSelectionPanel()
        {
            InitializeComponent();
            _selectedAdditionalRolesGrid.Clear();
            _selectedAdditionalRolesGrid.Cols.Add(_COLUMNNAME, _COLUMNNAME, 100);
            _selectedAdditionalRolesGrid.SelectionMode = iGSelectionMode.MultiExtended;
            _selectedAdditionalRolesGrid.RowMode = true;
            _selectedAdditionalRolesGrid.AllowColumnFilter = false;
            _selectedAdditionalRolesGrid.KeyUp += new KeyEventHandler(_selectedAdditionalRolesGrid_KeyUp);
            _selectedAdditionalRolesGrid.SelectionChanged += new EventHandler(_selectedAdditionalRolesGrid_SelectionChanged);
            _selectedAdditionalRolesGrid.AutoResizeCols = true;
            _selectedAdditionalRolesGrid.Dock = DockStyle.Fill;
            _userListPanel.Controls.Add(_selectedAdditionalRolesGrid);
        }

        #endregion

        #region Events
        void _selectedAdditionalRolesGrid_SelectionChanged(object sender, EventArgs e)
        {
            updateControls();
        }

        void _selectedAdditionalRolesGrid_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
            {
                DoRemove();
            }
        }

        private void _theDSGrid_DoubleClick(int rowIndex)
        {
            AddAdditionalRole(_theDSGrid.Rows[rowIndex].Cells[0].Value as string);
        }
        #endregion

        #region Properties
        public List<String> AdditionalRoles
        {
            set
            {
                List<String> rows = value;
                _selectedAdditionalRolesGrid.Rows.Clear();
                _theTypedRoleName.Text = string.Empty;
                foreach (String dr in rows)
                {
                    iGRow row = _selectedAdditionalRolesGrid.Rows.Add();
                    row.Cells[0].Value = dr.ToString();
                }
            }
            get
            {
                //loadAdditionalRolesFromText();
                DataTable dt = new DataTable();
                dt.Columns.Add(new DataColumn(_theTitle, System.Type.GetType("System.String")));
                List<String> roles = new List<String>();
                foreach (iGRow row in _selectedAdditionalRolesGrid.Rows)
                {
                    roles.Add(row.Cells[0].Value as string);
                }
                return roles;

            }
        }
        #endregion Properties

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

        public RoleSelectionPanel(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            _theConnectionDefinition = aConnectionDefinition;
            ShowWidgets();
        }

        public List<string> IsValid()
        {
            List<string> ret = new List<string>();
            List<string> additionalRoles = AdditionalRoles;

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
            _theWidgetConfig.ShowExportButtons = false;
            _theWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            DatabaseDataProviderConfig _dbConfig = _theWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;            
            _dbConfig.SQLText = "SELECT ROLE_NAME FROM TRAFODION_INFORMATION_SCHEMA.ROLE_INFO ORDER BY ROLE_NAME ASC";
            _theWidgetConfig.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;

            _theWidget = new GenericUniversalWidget();
            _theWidget.DataProvider = new DatabaseDataProvider(_theWidgetConfig.DataProviderConfig);

            //Set the display properties of the widget and add it to the control
            _theDataDisplayHandler = new RoleSelectorDataHandler(this);
            _theWidget.DataDisplayControl.DataDisplayHandler = _theDataDisplayHandler;
            _theWidget.UniversalWidgetConfiguration = _theWidgetConfig;
            _theWidget.Dock = DockStyle.Fill;
            this._theRolesGroupBox.Controls.Add(_theWidget);

            _theDSGrid = ((TabularDataDisplayControl)_theWidget.DataDisplayControl).DataGrid;
            _theDSGrid.DoubleClickHandler = RoleSelection_Handler;
            _theDSGrid.RowMode = true;
            _theDSGrid.AllowColumnFilter = false;
            //Set selected rows color while losed focus.
            _theDSGrid.SelCellsBackColorNoFocus = System.Drawing.SystemColors.Highlight;
            _theDSGrid.SelCellsForeColorNoFocus = System.Drawing.SystemColors.HighlightText;
            _theDSGrid.SelectionChanged += new EventHandler(_theDSGrid_SelectionChanged);
            //Disable the export buttons so it does not show up within the universal widget panel
            ((TabularDataDisplayControl)_theWidget.DataDisplayControl).ShowExportButtons = false;

            //Add event handlers to deal with data provider events
            AddHandlers();

            //Start it.
            _theWidget.StartDataProvider();
            this.Disposed += new EventHandler(RolesControl_Disposed);

            updateControls();
        }

        void _theDSGrid_SelectionChanged(object sender, EventArgs e)
        {
            updateControls();
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
            if (_selectionMode == RoleSelectionMode.Additional)
            {
                setAdditionalRoles();
            }
        }

        private void AddHandlers()
        {
            if (_theWidget != null && _theWidget.DataProvider != null)
            {
                //Associate the event handlers
                _theWidget.DataProvider.OnErrorEncountered += HandleError;
                _theWidget.DataProvider.OnNewDataArrived += HandleNewDataArrived;
                _theWidget.DataProvider.OnFetchingData += HandleFetchingData;

            }
        }

        private void RemoveHandlers()
        {
            if (_theWidget != null && _theWidget.DataProvider != null)
            {
                //Remove the event handlers
                _theWidget.DataProvider.OnErrorEncountered -= HandleError;
                _theWidget.DataProvider.OnNewDataArrived -= HandleNewDataArrived;
                _theWidget.DataProvider.OnFetchingData -= HandleFetchingData;
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

        private void _theAdditionalRoleBtn_Click(object sender, EventArgs e)
        {
            setAdditionalRoles();
        }

        private void setAdditionalRoles()
        {
            bool isExisted = false;
            foreach (iGRow row in _theDSGrid.SelectedRows)
            {
                isExisted = false;
                foreach (iGRow irow in _selectedAdditionalRolesGrid.Rows)
                {
                    if (irow.Cells[0].Value.ToString().Equals(row.Cells[0].Value.ToString()))
                    {
                        isExisted = true;
                        break;
                    }
                }

                if (!isExisted)
                {
                    iGRow newRow = _selectedAdditionalRolesGrid.Rows.Add();
                    newRow.Cells[0].Value = row.Cells[0].Value;
                }
            }

        }

        private string[] getSelectedRoles()
        {
            int rowCount = _theDSGrid.SelectedRowIndexes.Count;
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
                bool hasMatchedRow = false;
                int rowCount = _theDSGrid.Rows.Count;
                for (int row = 0; row < rowCount; row++)
                {
                    iGRow tempRow = _theDSGrid.Rows[row];
                    if (((string)tempRow.Cells[0].Value).StartsWith(text, StringComparison.InvariantCultureIgnoreCase))
                    {
                        _theDSGrid.PerformAction(iGActions.DeselectAllRows);
                        _theDSGrid.Rows[row].Selected = true;
                        _theDSGrid.SetCurRow(row);
                        hasMatchedRow = true;
                        break;
                    }
                }

                if (!hasMatchedRow)
                {
                    _theDSGrid.PerformAction(iGActions.DeselectAllRows);
                }
            }
        }

        /// <summary>
        /// Update button status for selection dialog.
        /// </summary>
        private void updateControls()
        {
            _theAdditionalRoleBtn.Enabled = _theDSGrid.Rows.Count > 0 && _theDSGrid.SelectedRows.Count > 0;
            _theDelAdditionalRoleBtn.Enabled = _selectedAdditionalRolesGrid.Rows.Count > 0 && _selectedAdditionalRolesGrid.SelectedRows.Count > 0;
        }

        private void _theDelAdditionalRoleBtn_Click(object sender, EventArgs e)
        {
            DoRemove();
        }

        private void DoRemove()
        {
            if (_selectedAdditionalRolesGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in _selectedAdditionalRolesGrid.SelectedRowIndexes)
                {
                    _selectedAdditionalRolesGrid.Rows.RemoveAt(_selectedAdditionalRolesGrid.SelectedRowIndexes[0]);
                }
                updateControls();
            }
        }

    }

    #region RoleSelectorDataHandler Class

    public class RoleSelectorDataHandler : TabularDataDisplayHandler
    {
        #region Fields
        private RoleSelectionPanel _theRoleSelectionPanel;
        string[] columns = new string[] { "Role Name" };
        enum ColumnPos { RoleName = 0 };
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

        private List<string> getRoleNames(string roleNameString)
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
