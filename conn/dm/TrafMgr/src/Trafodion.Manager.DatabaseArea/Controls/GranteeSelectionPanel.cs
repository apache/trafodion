//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.DatabaseArea.Controls
{

    public partial class GranteeSelectionPanel : UserControl
    {
        #region Fields
        static readonly string GranteeSelectionPersistenceKey = "ComponentPrivilegesGranteeSelection";
        UniversalWidgetConfig _universalWidgetConfig;
        GenericUniversalWidget _granteesWidget;
        TrafodionIGrid _granteesGrid;
       //TrafodionSystem _sqlMxSystem;
        ConnectionDefinition _connectionDefinition;
        
        TrafodionIGrid _selectedGranteesGrid = new TrafodionIGrid();

        private const string COL_GRANTEETYPE = "Type";
        private const string COL_GRANTEE = "Grantee Name";

        // The string-ified user types.
        private const string OWNER_USERTYPE = "O";
        private const string PUBLIC_USERTYPE = "P";
        private const string SYSTEM_USERTYPE = "S";
        private const string ROLE_USERTYPE = "R";
        private const string USER_USERTYPE = "U";
        List<DataRow> _listGrantedGrantee = new List<DataRow>();
        #endregion

        #region Properties

        /// <summary>
        /// If "Public" is in granted grantees
        /// </summary>
        public bool IsPublicInGrantees
        {
            get;
            set;
        }

        /// <summary>
        /// Interact property for consumer control.
        /// </summary>
        public List<DataRow> SelectedGrantees
        {
            get
            {
                DataTable dt = new DataTable();                
                dt.Columns.Add(new DataColumn(COL_GRANTEE, System.Type.GetType("System.String")));
                dt.Columns.Add(new DataColumn(COL_GRANTEETYPE, System.Type.GetType("System.String")));
                List<DataRow> Grantees = new List<DataRow>();
                foreach (iGRow row in _selectedGranteesGrid.Rows)
                {
                    DataRow dr = dt.NewRow();
                    dr[COL_GRANTEE] = row.Cells[COL_GRANTEE].Value as string;
                    dr[COL_GRANTEETYPE] = row.Cells[COL_GRANTEETYPE].Value as string;
                    Grantees.Add(dr);
                }
                return Grantees;
            }
            set
            {
                //reset user input
                _theTypedGranteeName.Text = string.Empty;
                _theAllRadioButton.Checked = true;

                _selectedGranteesGrid.Rows.Clear();
                _listGrantedGrantee = value;
            }
        }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set
            {
                //When a new connection is set, stop the data provider and reset the data provider to use the new connection
                if (_connectionDefinition != null)
                {
                    _granteesWidget.DataProvider.Stop();
                    _granteesGrid.Rows.Clear();
                }
                _connectionDefinition = value;             
            }
        }
        #endregion

        #region Constructor
        public GranteeSelectionPanel()
        {
            InitializeComponent();
            _selectedGranteesGrid.Clear();            
            _selectedGranteesGrid.Cols.Add(COL_GRANTEE, COL_GRANTEE, 120);
            _selectedGranteesGrid.Cols.Add(COL_GRANTEETYPE, COL_GRANTEETYPE, 120);
            _selectedGranteesGrid.SelectionMode = iGSelectionMode.MultiExtended;
            _selectedGranteesGrid.RowMode = true;
            _selectedGranteesGrid.AllowColumnFilter = false;
            _selectedGranteesGrid.KeyUp += new KeyEventHandler(_selectedGranteesGrid_KeyUp);
            _selectedGranteesGrid.SelectionChanged += new EventHandler(_selectedGranteesGrid_SelectionChanged);
            _selectedGranteesGrid.AutoResizeCols = true;
            _selectedGranteesGrid.Dock = DockStyle.Fill;
            _granteeListPanel.Controls.Add(_selectedGranteesGrid);
        }

        public GranteeSelectionPanel(ConnectionDefinition aConnectionDefinition, bool isPublicInGrantees)
            : this()
        {
            _connectionDefinition = aConnectionDefinition;
            IsPublicInGrantees = isPublicInGrantees;

            //_sqlMxSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);
            SetupGrantees();
        }

        #endregion

        #region Events
        private void _selectedGranteesGrid_SelectionChanged(object sender, EventArgs e)
        {
            updateControls();
        }
        
        private void _selectedGranteesGrid_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
            {
                DoRemove();
            }
        }

        private void _GranteesGrid_SelectionChanged(object sender, EventArgs e)
        {
            updateControls();
        }

        private void _GranteesGrid_DoubleClick(int rowIndex)
        {
            addSelectedGrantees();
        }

        private void _theTypedGranteeName_TextChanged(object sender, EventArgs e)
        {
            string text = _theTypedGranteeName.Text.Trim();
            if ((_granteesGrid != null) && (text.Length > 0))
            {
                int rowCount = _granteesGrid.Rows.Count;
                for (int row = 0; row < rowCount; row++)
                {
                    iGRow tempRow = _granteesGrid.Rows[row];
                    if (((string)tempRow.Cells[0].Value).StartsWith(text, StringComparison.InvariantCultureIgnoreCase))
                    {
                        _granteesGrid.PerformAction(iGActions.DeselectAllRows);
                        _granteesGrid.Rows[row].Selected = true;
                        _granteesGrid.SetCurRow(row);
                        break;
                    }
                }
            }

        }

        private void GranteeSelectionPanel_Load(object sender, EventArgs e)
        {
            if (_granteesWidget != null && _granteesWidget.DataProvider != null)
            {

                _granteesWidget.DataProvider.Start();
            }
        }

        private void _theAddGranteesBtn_Click(object sender, EventArgs e)
        {
            addSelectedGrantees();
        }

        private void _theDelGranteesBtn_Click(object sender, EventArgs e)
        {
            DoRemove();
        }

        private void radioButtonFilter_Click(object sender, EventArgs e)
        {
            if ((sender as RadioButton).Checked)
            {
                _granteesGrid.BeginUpdate();
                try
                {
                    if (_granteesGrid.Rows.Count > 0)
                    {
                        if (_theUserRadioButton.Checked)
                            Filter(Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.User.ToString());
                        else if (_theRolesRadioButton.Checked)
                            Filter(Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.Role.ToString());
                        else if (_thePublicRadioButton.Checked)
                            Filter(Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.Public.ToString());
                        else if (_theAllRadioButton.Checked)
                            UnFilter();
                    }
                }
                finally
                {
                    _granteesGrid.PerformAction(iGActions.DeselectAllRows);
                    _granteesGrid.EndUpdate();
                }
            }
        }


        #endregion

        #region Private Functions

        private void updateControls()
        {
            _theDelGranteesBtn.Enabled = _selectedGranteesGrid.Rows.Count > 0 &&
                _selectedGranteesGrid.SelectedRows.Count > 0;
            _theAddGranteesBtn.Enabled = _granteesGrid.Rows.Count > 0 && _granteesGrid.SelectedRows.Count > 0;
            _theAllRadioButton.Enabled = _granteesGrid.Rows.Count > 0;
            _theUserRadioButton.Enabled = _granteesGrid.Rows.Count > 0;
            _theRolesRadioButton.Enabled = _granteesGrid.Rows.Count > 0;
        }

        private void DoRemove()
        {
            if (_selectedGranteesGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in _selectedGranteesGrid.SelectedRowIndexes)
                {
                    _selectedGranteesGrid.Rows.RemoveAt(_selectedGranteesGrid.SelectedRowIndexes[0]);
                }
            }
            updateControls();
        }


        /// <summary>
        /// Sets up the Grantees widget
        /// </summary>
        private void SetupGrantees()
        {
            //Read the widget config from persistence
            _universalWidgetConfig = WidgetRegistry.GetConfigFromPersistence(GranteeSelectionPersistenceKey);

            if (_universalWidgetConfig == null)
            {
                //Create the Universal widget configuration
                _universalWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _universalWidgetConfig.Name = GranteeSelectionPersistenceKey;
                _universalWidgetConfig.DataProviderConfig.TimerPaused = false;
                _universalWidgetConfig.ShowProviderStatus = false;
                _universalWidgetConfig.ShowProperties = false;
                _universalWidgetConfig.ShowToolBar = true;
                _universalWidgetConfig.ShowChart = false;
                _universalWidgetConfig.ShowTimerSetupButton = false;
                _universalWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
                _universalWidgetConfig.ShowHelpButton = true;
                _universalWidgetConfig.ShowExportButtons = false;
            }

            _universalWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            //_universalWidgetConfig.HelpTopic = HelpTopics.ControlSettings;

            //Create the Grantees Widget
            _granteesWidget = new GenericUniversalWidget();
            ((TabularDataDisplayControl)_granteesWidget.DataDisplayControl).LineCountFormat = "Grantees";
            _granteesWidget.DataProvider = new ComponentPrivilegesGranteeDataProvider();
            _granteesWidget.DataProvider.DataProviderConfig.ConnectionDefinition = _connectionDefinition;

            //Set the widget configuration 
            _granteesWidget.UniversalWidgetConfiguration = _universalWidgetConfig;

            _granteesWidget.Dock = DockStyle.Fill;
            _granteeGroupBox.Controls.Add(_granteesWidget);

            //Associate the custom data display handler for the TabularDisplay panel
            _granteesWidget.DataDisplayControl.DataDisplayHandler = new GranteesDataHandler(this);

            //Initialize the Alerts iGrid
            _granteesGrid = ((TabularDataDisplayControl)_granteesWidget.DataDisplayControl).DataGrid;
            _granteesGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);
            _granteesGrid.AutoResizeCols = true;
            _granteesGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            _granteesGrid.DoubleClickHandler = _GranteesGrid_DoubleClick;
            _granteesGrid.RowMode = true;
            _granteesGrid.AllowColumnFilter = false;
            //Set selected rows color while losed focus.
            _granteesGrid.SelCellsBackColorNoFocus = System.Drawing.SystemColors.Highlight;
            _granteesGrid.SelCellsForeColorNoFocus = System.Drawing.SystemColors.HighlightText;
            _granteesGrid.SelectionChanged += new EventHandler(_GranteesGrid_SelectionChanged);

        }

     
        /// <summary>
        /// Add selected Grantees to selected gridview.
        /// </summary>
        private void addSelectedGrantees()
        {
            bool isExisted = false;
            foreach (iGRow row in _granteesGrid.SelectedRows)
            {
                isExisted = false;
                foreach (iGRow irow in _selectedGranteesGrid.Rows)
                {
                    if (irow.Cells[COL_GRANTEE].Value.ToString().Equals(
                        row.Cells[COL_GRANTEE].Value.ToString()))
                    {
                        isExisted = true;
                        break;
                    }
                }

                if (!isExisted)
                {
                    iGRow newRow = _selectedGranteesGrid.Rows.Insert(0);
                    newRow.Cells[COL_GRANTEE].Value = row.Cells[COL_GRANTEE].Value;
                    newRow.Cells[COL_GRANTEETYPE].Value = row.Cells[COL_GRANTEETYPE].Value;
                }
            }
        }




        private void UnFilter()
        {
            foreach (iGCell myCell in _granteesGrid.Cols[COL_GRANTEETYPE].Cells)
            {
                if (myCell.Row.Type == iGRowType.Normal)
                {
                    myCell.Row.Visible = true;
                }
            }
        }

        private void Filter(string type)
        {
            foreach (iGCell myCell in _granteesGrid.Cols[COL_GRANTEETYPE].Cells)
            {
                if (myCell.Row.Type == iGRowType.Normal)
                {
                    if (!myCell.Value.ToString().Equals(type))
                        myCell.Row.Visible = false;
                    else
                        myCell.Row.Visible = true;
                }
            }
        }



        private bool isExistedInGrantedGranteeList(DataRow row)
        {
            foreach (DataRow dr in _listGrantedGrantee)
            {

                if (dr[COL_GRANTEE].ToString().Equals(row[COL_GRANTEE].ToString()) &&
                    dr[COL_GRANTEETYPE].ToString().Equals(row[COL_GRANTEETYPE].ToString()))
                {
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Translates from a string-ified user type to one of the UserType types.
        /// </summary>
        /// <param name="type">One of the known user types.
        /// <seealso cref="OWNER_USERTYPE"/>
        /// <seealso cref="PUBLIC_USERTYPE"/>
        /// <seealso cref="SYSTEM_USERTYPE"/>
        /// <seealso cref="USER_USERTYPE"/>
        /// </param>
        /// <returns>A UserType. If the usertype is unknown, it returns UserType.User</returns>
        private Trafodion.Manager.DatabaseArea.Model.Privilege.UserType TranslateType(string type)
        {
            Trafodion.Manager.DatabaseArea.Model.Privilege.UserType userType = Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.User;

            if (type.Equals(OWNER_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.Owner;
            }
            else if (type.Equals(PUBLIC_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.Public;
            }
            else if (type.Equals(SYSTEM_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.System;
            }
            else if (type.Equals(ROLE_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.Role;
            }
            else if (type.Equals(USER_USERTYPE, StringComparison.OrdinalIgnoreCase))
            {
                userType = Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.User;
            }
            else
            {
#if DEBUG
                // A debug build will actually throw an exception for an unexpected type.
                throw new ApplicationException("\"" + type + "\" is an unknown user type.");
#endif
            }

            return userType;
        }
        #endregion

        #region Public Methods
        /// <summary>
        /// Loads user and role list to the grantee List
        /// </summary>
        public void LoadData()
        {
            DataTable granteesTable = new DataTable();
            granteesTable.Columns.Add(new DataColumn(COL_GRANTEE, System.Type.GetType("System.String")));
            granteesTable.Columns.Add(new DataColumn(COL_GRANTEETYPE, System.Type.GetType("System.String")));
            TrafodionSystem _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_connectionDefinition);
            _granteesGrid.Rows.Clear();

            /* "Public" is special. It cannot be found in RoleAndUserList.
             * So IsPublicInGrantees indicates if "Public" is already in granted grantees.
             * If "Public" is NOT in the granted grantees, it should be add into grantee selection grid for users to select.
            */
            if (!IsPublicInGrantees)
            {
                //Add PUBLIC to the grantee list.
                DataRow drPublic = granteesTable.NewRow();
                drPublic[COL_GRANTEE] = "PUBLIC";
                drPublic[COL_GRANTEETYPE] = Trafodion.Manager.DatabaseArea.Model.Privilege.UserType.Public;
                granteesTable.Rows.Add(drPublic);
            }

            ArrayList roleAndUserList = _sqlMxSystem.RoleAndUserList;

            if (roleAndUserList.Count > 0)
            {
                if (roleAndUserList[0] != null)
                {
                    List<string[]> listRole = (List<string[]>)roleAndUserList[0];
                    if (listRole != null)
                    {
                        foreach (string[] role in listRole)
                        {
                            DataRow dr = granteesTable.NewRow();
                            dr[COL_GRANTEE] = role[0];
                            dr[COL_GRANTEETYPE] = TranslateType(role[1]);
                            if (!isExistedInGrantedGranteeList(dr))
                            {
                                granteesTable.Rows.Add(dr);
                            }
                        }
                    }
                }
                if (roleAndUserList[1] != null)
                {
                    List<string[]> listUser = (List<string[]>)roleAndUserList[1];
                    if (listUser != null)
                    {
                        foreach (string[] user in listUser)
                        {
                            DataRow dr = granteesTable.NewRow();
                            dr[COL_GRANTEE] = user[0];
                            dr[COL_GRANTEETYPE] = TranslateType(user[1]);
                            if (!isExistedInGrantedGranteeList(dr))
                            {
                                granteesTable.Rows.Add(dr);
                            }
                        }
                    }
                }
            }

            _granteesGrid.FillWithData(granteesTable);
            _granteesGrid.ApplyFilter();

            if (_theRolesRadioButton.Checked)
            {
                _theRolesRadioButton.PerformClick();
            }
            else if(_theUserRadioButton.Checked)
            {
                _theUserRadioButton.PerformClick();
            }
            else if (_theAllRadioButton.Checked)
            {
                _theAllRadioButton.PerformClick();
            }
            else if (_thePublicRadioButton.Checked)
            {
                _thePublicRadioButton.PerformClick();
            }
            updateControls();
        }

        #endregion

    }


    /// <summary>
    /// Custom handler to load the components and grantee information
    /// </summary>
    public class GranteesDataHandler : TabularDataDisplayHandler
    {
        private GranteeSelectionPanel _granteeSelectionPanel;

        public GranteesDataHandler(GranteeSelectionPanel granteeSelectionPanel)
        {
            _granteeSelectionPanel = granteeSelectionPanel;
        }

        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            _granteeSelectionPanel.LoadData();
        }

    }
}
