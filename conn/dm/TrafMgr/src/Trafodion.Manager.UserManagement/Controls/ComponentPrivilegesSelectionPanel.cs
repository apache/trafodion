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
using System.Drawing;
using System.Linq;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class ComponentPrivilegesSelectionPanel : UserControl
    {
        #region Fields
        static readonly string CompPrivPersistenceKey = "ComponentPrivilegesSelection";
        UniversalWidgetConfig _universalWidgetConfig;
        GenericUniversalWidget _granteesWidget;
        TrafodionIGrid _privilegesGrid;
        //TrafodionSystem _sqlMxSystem;
        ConnectionDefinition _connectionDefinition;
        string _granteeName;          
        TrafodionIGrid _selectedPrivilegesGrid = new TrafodionIGrid();

        private const string COL_COMPONENT = "Component";
        private const string COL_PRIVILEGES = "Privilege";
        private const string COL_WITHGRANTOPTION = "With Grant Option";
        private const string COL_DESCRIPTION = "Description";
        #endregion

        #region Properties
        public List<DataRow> SelectedPrivileges
        {
            get
            {
                DataTable dt = new DataTable();
                dt.Columns.Add(new DataColumn(COL_COMPONENT, System.Type.GetType("System.String")));
                dt.Columns.Add(new DataColumn(COL_PRIVILEGES, System.Type.GetType("System.String")));
                dt.Columns.Add(new DataColumn(COL_DESCRIPTION, System.Type.GetType("System.String")));
                List<DataRow> privileges = new List<DataRow>();
                foreach (iGRow row in _selectedPrivilegesGrid.Rows)
                {
                    DataRow dr=dt.NewRow();
                    dr[COL_COMPONENT] = row.Cells[COL_COMPONENT].Value as string;
                    dr[COL_PRIVILEGES] = row.Cells[COL_PRIVILEGES].Value as string;
                    dr[COL_DESCRIPTION] = row.Cells[COL_DESCRIPTION].Value as string;
                    privileges.Add(dr);
                }
                return privileges;
            }
            set
            {
                List<DataRow> rows = value;
                _selectedPrivilegesGrid.Rows.Clear();
                _theTypedPrivilegesName.Text = string.Empty;
                foreach (DataRow dr in rows)
                {
                    iGRow row = _selectedPrivilegesGrid.Rows.Add();
                    row.Cells[COL_COMPONENT].Value = dr[COL_COMPONENT].ToString();
                    row.Cells[COL_PRIVILEGES].Value = dr[COL_PRIVILEGES].ToString();
                    row.Cells[COL_DESCRIPTION].Value = dr[COL_DESCRIPTION].ToString();
                }

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
                    _privilegesGrid.Rows.Clear();
                }
                _connectionDefinition = value;
                if (_connectionDefinition != null)
                {
                    _universalWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
                    _granteesWidget.DataProvider.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
                    _granteesWidget.StartDataProvider();
                }
            }
        }
        #endregion

        #region Constructors
        public ComponentPrivilegesSelectionPanel()
        {
            InitializeComponent();
            _selectedPrivilegesGrid.Clear();
            _selectedPrivilegesGrid.Cols.Add(COL_COMPONENT, COL_COMPONENT, 100);
            _selectedPrivilegesGrid.Cols.Add(COL_PRIVILEGES, COL_PRIVILEGES, 120);
            _selectedPrivilegesGrid.Cols.Add(COL_DESCRIPTION, COL_DESCRIPTION, 100);
            _selectedPrivilegesGrid.Cols[COL_DESCRIPTION].Visible = false;
            _selectedPrivilegesGrid.SelectionMode = iGSelectionMode.MultiExtended;
            _selectedPrivilegesGrid.RowMode = true;
            _selectedPrivilegesGrid.AllowColumnFilter = false;
            _selectedPrivilegesGrid.KeyUp += new KeyEventHandler(_selectedPrivilegesGrid_KeyUp);
            _selectedPrivilegesGrid.SelectionChanged += new EventHandler(_selectedPrivilegesGrid_SelectionChanged);
            _selectedPrivilegesGrid.AutoResizeCols = true;
            _selectedPrivilegesGrid.Dock = DockStyle.Fill;
            _userListPanel.Controls.Add(_selectedPrivilegesGrid);
        }

        public ComponentPrivilegesSelectionPanel(ConnectionDefinition aConnectionDefinition, string granteeName)
            : this()
        {
            _connectionDefinition = aConnectionDefinition;
            _granteeName = granteeName;
            //_sqlMxSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);
            SetupPrivileges();
        }

        #endregion

        #region Events
        void _selectedPrivilegesGrid_SelectionChanged(object sender, EventArgs e)
        {
            updateControls();
        }

      

        void _selectedPrivilegesGrid_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
            {
                DoRemove();
            }
        }

        void _privilegesGrid_SelectionChanged(object sender, EventArgs e)
        {
            updateControls();
        }

        private void _privilegesGrid_DoubleClick(int rowIndex)
        {
            addSelectedPrivileges();
        }

        private void ComponentPrivilegesSelectionPanel_Load(object sender, EventArgs e)
        {
            if (_granteesWidget != null && _granteesWidget.DataProvider != null)
            {

                _granteesWidget.DataProvider.Start();
            }
        }

        private void _theAddPrivilegesBtn_Click(object sender, EventArgs e)
        {
            addSelectedPrivileges();
        }

        private void _theTypedPrivilegesName_TextChanged(object sender, EventArgs e)
        {
            string text = _theTypedPrivilegesName.Text.Trim();
            if ((_privilegesGrid != null) && (text.Length > 0))
            {
                bool hasMatchedRow = false;
                int rowCount = _privilegesGrid.Rows.Count;
                for (int row = 0; row < rowCount; row++)
                {
                    iGRow tempRow = _privilegesGrid.Rows[row];
                    if (((string)tempRow.Cells[1].Value).StartsWith(text, StringComparison.InvariantCultureIgnoreCase))
                    {
                        _privilegesGrid.PerformAction(iGActions.DeselectAllRows);
                        _privilegesGrid.Rows[row].Selected = true;
                        _privilegesGrid.SetCurRow(row);
                        hasMatchedRow = true;
                        break;
                    }
                }

                if (!hasMatchedRow)
                {
                    _privilegesGrid.PerformAction(iGActions.DeselectAllRows);
                }
            }
        }

        private void _theDelPrivilegesBtn_Click(object sender, EventArgs e)
        {
            DoRemove();
        }


        #endregion

        #region Private Functions

        private void updateControls()
        {
            _theDelPrivilegesBtn.Enabled = _selectedPrivilegesGrid.Rows.Count > 0 &&
                _selectedPrivilegesGrid.SelectedRows.Count > 0;
            _theAddPrivilegesBtn.Enabled = _privilegesGrid.Rows.Count > 0 && _privilegesGrid.SelectedRows.Count > 0;
        }

        private void DoRemove()
        {
            if (_selectedPrivilegesGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in _selectedPrivilegesGrid.SelectedRowIndexes)
                {
                    _selectedPrivilegesGrid.Rows.RemoveAt(_selectedPrivilegesGrid.SelectedRowIndexes[0]);
                }
                updateControls();
            }    
        }


        /// <summary>
        /// Sets up the privileges widget
        /// </summary>
        void SetupPrivileges()
        {
            //Read the widget config from persistence
            _universalWidgetConfig = WidgetRegistry.GetConfigFromPersistence(CompPrivPersistenceKey);

            if (_universalWidgetConfig == null)
            {
                //Create the Universal widget configuration
                _universalWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _universalWidgetConfig.Name = CompPrivPersistenceKey;
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
            ((TabularDataDisplayControl)_granteesWidget.DataDisplayControl).LineCountFormat = "Privileges";
            _granteesWidget.DataProvider = new ComponentPrivilegesDataProvider();
            _granteesWidget.DataProvider.DataProviderConfig.ConnectionDefinition = _connectionDefinition;

            //Set the widget configuration 
            _granteesWidget.UniversalWidgetConfiguration = _universalWidgetConfig;

            _granteesWidget.Dock = DockStyle.Fill;
            _privilegesGroupBox.Controls.Add(_granteesWidget);

            //Associate the custom data display handler for the TabularDisplay panel
            _granteesWidget.DataDisplayControl.DataDisplayHandler = new CompPrivDataHandler(this);

            //Initialize the Alerts iGrid
            _privilegesGrid = ((TabularDataDisplayControl)_granteesWidget.DataDisplayControl).DataGrid;
            _privilegesGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);
            _privilegesGrid.AutoResizeCols = true;
            _privilegesGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            _privilegesGrid.DoubleClickHandler = _privilegesGrid_DoubleClick;
            _privilegesGrid.RowMode = true;
            _privilegesGrid.AllowColumnFilter = false;
            //Set selected rows color while losed focus.
            _privilegesGrid.SelCellsBackColorNoFocus = System.Drawing.SystemColors.Highlight;
            _privilegesGrid.SelCellsForeColorNoFocus = System.Drawing.SystemColors.HighlightText;
            _privilegesGrid.SelectionChanged += new EventHandler(_privilegesGrid_SelectionChanged);
        
        }

        /// <summary>
        /// Add selected privileges to selected gridview.
        /// </summary>
        private void addSelectedPrivileges()
        {
            bool isExisted = false;
            foreach (iGRow row in _privilegesGrid.SelectedRows)
            {
                isExisted = false;
                foreach (iGRow irow in _selectedPrivilegesGrid.Rows)
                {
                    if (irow.Cells[COL_COMPONENT].Value.ToString().Equals(
                        row.Cells[COL_COMPONENT].Value.ToString()) &&
                        irow.Cells[COL_PRIVILEGES].Value.ToString().Equals(
                        row.Cells[COL_PRIVILEGES].Value.ToString()))
                    {
                        isExisted = true;
                        break;
                    }
                }

                if (!isExisted)
                {
                    iGRow newRow = _selectedPrivilegesGrid.Rows.Add();
                    newRow.Cells[COL_COMPONENT].Value = row.Cells[COL_COMPONENT].Value;
                    newRow.Cells[COL_PRIVILEGES].Value = row.Cells[COL_PRIVILEGES].Value;
                    newRow.Cells[COL_DESCRIPTION].Value = row.Cells[COL_DESCRIPTION].Value;
                }
            }
        }

        #endregion
           
        #region Public Methods

        /// <summary>
        /// Loads the component privileges into the grantees grid and the component privileges user control 
        /// </summary>
        public void LoadData()
        {
            DataTable privilegesTable = new DataTable();
            privilegesTable.Columns.Add(new DataColumn(COL_COMPONENT, System.Type.GetType("System.String")));
            privilegesTable.Columns.Add(new DataColumn(COL_PRIVILEGES, System.Type.GetType("System.String")));
            privilegesTable.Columns.Add(new DataColumn(COL_DESCRIPTION, System.Type.GetType("System.String")));
            TrafodionSystem _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_connectionDefinition);
            _privilegesGrid.Rows.Clear();
            List<long> selectedComponents = new List<long>();
            List<string> privTypes = new List<string>();
            IEnumerable<ComponentPrivilege> privilegesForUser;
            if (_sqlMxSystem.ComponentPrivilegeUsages.Count > 0)
            {
                selectedComponents.AddRange(
                        from c in _sqlMxSystem.Components
                        select c.ComponentUID);

                privilegesForUser =
                (from priv in _sqlMxSystem.ComponentPrivileges
                 where priv.GranteeName.Equals(_granteeName)
                 select priv).ToArray();
                var q =

                from c in _sqlMxSystem.ComponentPrivilegeUsages.Where<ComponentPrivilegeUsage>(c => selectedComponents.Contains(c.ComponentUID))

                join p in privilegesForUser on c.ComponentUID equals p.ComponentUID into ps

                from pr in ps.Where<ComponentPrivilege>(pr => pr.ComponentUID == c.ComponentUID && pr.PrivType.Equals(c.PrivType)).DefaultIfEmpty()
                select new
                {
                    ComponentID = c.ComponentUID,
                    PrivilegeName = c.PrivName,
                    PrivilgesType=c.PrivType,
                    IsGranted = pr == null ? false : true,
                    WithGrant = pr == null ? false : pr.Grantable,
                    Grantor = pr == null ? "" : pr.GrantorName
                };
                
                foreach (var v in q)
                {
                    DataRow dr = privilegesTable.NewRow();
                    dr[COL_COMPONENT] = _sqlMxSystem.GetComponentName(v.ComponentID);
                    dr[COL_PRIVILEGES] = v.PrivilegeName;
                    dr[COL_DESCRIPTION] = _sqlMxSystem.GetComponentPrivilegeDescription(v.ComponentID, v.PrivilgesType);
                    privilegesTable.Rows.Add(dr);
                }

                _privilegesGrid.FillWithData(privilegesTable);
                updateControls();
            }
        }

        #endregion    


     
    }


    /// <summary>
    /// Custom handler to load the components and grantee information
    /// </summary>
    public class CompPrivDataHandler : TabularDataDisplayHandler
    {
        private ComponentPrivilegesSelectionPanel _componentPrivilegesSelectionPanel;

        public CompPrivDataHandler(ComponentPrivilegesSelectionPanel componentPrivilegesSelectionPanel)
        {
            _componentPrivilegesSelectionPanel = componentPrivilegesSelectionPanel;
        }

        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            _componentPrivilegesSelectionPanel.LoadData();
        }

    }
 
}
