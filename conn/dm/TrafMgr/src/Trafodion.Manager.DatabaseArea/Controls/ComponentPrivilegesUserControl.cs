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
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{   
    public partial class ComponentPrivilegesUserControl : UserControl
    {
        #region Fields
        public delegate void OnComponentPrivilegesChanged();
        public event OnComponentPrivilegesChanged OnComponentPrivilegesChangedImpl;

        ConnectionDefinition _connectionDefinition;
        string _componentName;
        string _granteeName;
        bool _IsWithGrantOptionEnabled = false;

        private DataTable _componentPrivilegesTable = new DataTable();
        private DataTable _componentPrivilegesTableCopy = new DataTable();
        private List<string> _listCommandString = new List<string>();        
        private List<string> _listPrivilgeForGrantColChanged = new List<string>();
        private List<string> _listPrivilgeForWithGrantColChanged = new List<string>();

        private const string COL_GRANTEE = "Grantee";
        private const string COL_COMPONENT = "Component";
        private const string COL_PRIVILEGES = "Privilege";
        private const string COL_DESCRIPTION = "Description";
        private const string COL_ISGRANTED = "Grant";
        private const string COL_WITHGRANTOPTION = "With Grant Option";
        private const string COL_GRANTOR = "Grantor";
        #endregion

        #region Properties
        
        public bool PrivilegesChanged
        {
            get
            {
                return _listPrivilgeForGrantColChanged.Count > 0 ||
                _listPrivilgeForWithGrantColChanged.Count > 0;
            }
        }

        public List<string> ListCommandString
        {
            get
            {
                _listCommandString = getAlterPrivilegesCommand(_granteeName);
                return _listCommandString;
            }
           
        }
        public DataTable ComponentPrivilegesTable
        {
            get { return _componentPrivilegesTable; }
            set { _componentPrivilegesTable = value; }
        }      

        public DataTable ComponentPrivilegesTableCopy
        {
            get { return _componentPrivilegesTableCopy; }
            set { _componentPrivilegesTableCopy = value; }
        }

        public bool IsWithGrantOptionEnabled
        {
            set
            {
                _IsWithGrantOptionEnabled = value;
            }
        }

        public bool IsPublicGranteeType
        {
            set;
            get;
        }

        public ConnectionDefinition ConnectionDefinition
        {
          get { return _connectionDefinition; }
          set { _connectionDefinition = value; }
        }
        public string Component
        {
          get { return _componentName; }
          set { _componentName = value; }
        }

        public string GranteeName
        {
          get { return _granteeName; }
          set { _granteeName = value; }
        }

        public void SetGrantedByVisibility(bool isVisible)
        {
            this._grantedByPanel.Visible = isVisible;
        }

        public object GrantedByDataSource
        {
            get { return _grantedByComboBox.DataSource; }
            set 
            { 
                _grantedByComboBox.DataSource = value;
                _grantedByComboBox.SelectedIndex = -1;
            }
        }

        #endregion

        #region Constructors
        public ComponentPrivilegesUserControl()
        {
            InitializeComponent();

            _addPrivilegeToolStripButton.Image =  global::Trafodion.Manager.Properties.Resources.EditAddIcon;
            _deletePrivilegeToolStripButton.Image = global::Trafodion.Manager.Properties.Resources.EditRemoveIcon;

            _privilegesGrid.Clear();
            iGCol componentCol = _privilegesGrid.Cols.Add(COL_COMPONENT, COL_COMPONENT);
            componentCol.MinWidth = componentCol.MaxWidth = 110;
            iGCol privilegeCol = _privilegesGrid.Cols.Add(COL_PRIVILEGES, COL_PRIVILEGES);
            privilegeCol.MinWidth = privilegeCol.MaxWidth = 150;
            iGCol grantCol = _privilegesGrid.Cols.Add(COL_ISGRANTED, COL_ISGRANTED);
            grantCol.ColHdrStyle.TextAlign = iGContentAlignment.MiddleCenter;
            grantCol.CellStyle.ImageAlign = iGContentAlignment.MiddleCenter;
            grantCol.CellStyle.Type = iGCellType.Check;
            grantCol.CellStyle.SingleClickEdit = iGBool.True;
            grantCol.MinWidth = grantCol.MaxWidth = 60;

            iGCol withGrantCol = _privilegesGrid.Cols.Add(COL_WITHGRANTOPTION, COL_WITHGRANTOPTION);
            withGrantCol.ColHdrStyle.TextAlign = iGContentAlignment.MiddleCenter;
            withGrantCol.CellStyle.ImageAlign = iGContentAlignment.MiddleCenter;
            withGrantCol.CellStyle.Type = iGCellType.Check;
            withGrantCol.CellStyle.SingleClickEdit = iGBool.True;
            withGrantCol.MinWidth = withGrantCol.MaxWidth = 130;
            iGCol grantedByCol = _privilegesGrid.Cols.Add(COL_GRANTOR, COL_GRANTOR);
            grantedByCol.MinWidth = 100;
            grantedByCol.MaxWidth = 200;
            iGCol descriptionCol = _privilegesGrid.Cols.Add(COL_DESCRIPTION, COL_DESCRIPTION);
            descriptionCol.MinWidth = 200;

            _privilegesGrid.AfterCommitEdit += new iGAfterCommitEditEventHandler(_privilegesGrid_AfterCommitEdit);
            _privilegesGrid.AutoWidthColMode = iGAutoWidthColMode.Header;
            _privilegesGrid.AutoResizeCols = true;
            _privilegesGrid.ReadOnly = false;

            _toolTip.SetToolTip(_grantedByComboBox, global::Trafodion.Manager.Properties.Resources.GrantedByToolTip);

            _componentComboBox.Items.Add("ALL");
        }                

        #endregion

        #region private events
        private void _privilegesGrid_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e)
        {
            string selectedGrantor = string.Empty;
            #region Keep Grant and With Grant Option consistency and Recording user changes by list string.
            if (_privilegesGrid.Rows[e.RowIndex].Cells[COL_GRANTOR].Value != null)
            {
                selectedGrantor = _privilegesGrid.Rows[e.RowIndex].Cells[COL_GRANTOR].Value.ToString();
            }
            //For recording user changes on privilges
            string strPrivilege = _privilegesGrid.Rows[e.RowIndex].Cells[COL_COMPONENT].Value.ToString() +
                 _privilegesGrid.Rows[e.RowIndex].Cells[COL_PRIVILEGES].Value.ToString() +
                 selectedGrantor;
            if (e.ColIndex == _privilegesGrid.Cols[COL_ISGRANTED].Index)
            {
                //Recording user changes on privilges
                if (_listPrivilgeForGrantColChanged.Contains(strPrivilege))
                {
                    _listPrivilgeForGrantColChanged.Remove(strPrivilege);
                }
                else
                {
                    _listPrivilgeForGrantColChanged.Add(strPrivilege);
                }

                if ("false".Equals(_privilegesGrid.Rows[e.RowIndex].Cells[e.ColIndex].Value.ToString(),
                    System.StringComparison.InvariantCultureIgnoreCase))
                {                    
                    //Only in case of WITH GRANT OPTION column is CHECKED and it should become UNCHECKED at this time.
                    if ((bool)_privilegesGrid.Rows[e.RowIndex].Cells[_privilegesGrid.Cols[COL_WITHGRANTOPTION].Index].Value)
                    {
                        //Also remove from list string that recording user changes on privilges
                        if (_listPrivilgeForWithGrantColChanged.Contains(strPrivilege))
                        {
                            _listPrivilgeForWithGrantColChanged.Remove(strPrivilege);
                        }
                        else
                        {
                            _listPrivilgeForWithGrantColChanged.Add(strPrivilege);
                        }
                    }

                    _privilegesGrid.Rows[e.RowIndex].Cells[_privilegesGrid.Cols[COL_WITHGRANTOPTION].Index].Value = false;
                }
            }
            if (e.ColIndex == _privilegesGrid.Cols[COL_WITHGRANTOPTION].Index)
            {
                //Recording user changes on privilges on WITH GRANT OPTION
                if (_listPrivilgeForWithGrantColChanged.Contains(strPrivilege))
                {
                    _listPrivilgeForWithGrantColChanged.Remove(strPrivilege);
                }
                else
                {
                    _listPrivilgeForWithGrantColChanged.Add(strPrivilege);
                }

                if ("true".Equals(_privilegesGrid.Rows[e.RowIndex].Cells[e.ColIndex].Value.ToString(),
                    System.StringComparison.InvariantCultureIgnoreCase))
                {                                        
                    //Only in case of GRANTED column is UNCHECKED and it should become CHECKED at this time.
                    if (!(bool)_privilegesGrid.Rows[e.RowIndex].Cells[_privilegesGrid.Cols[COL_ISGRANTED].Index].Value)
                    {
                        //Also recording the privilege changes which made by WITH GRANT OPTION checked.
                        if (_listPrivilgeForGrantColChanged.Contains(strPrivilege))
                        {
                            _listPrivilgeForGrantColChanged.Remove(strPrivilege);
                        }
                        else
                        {
                            _listPrivilgeForGrantColChanged.Add(strPrivilege);
                        }                        
                    }

                    _privilegesGrid.Rows[e.RowIndex].Cells[_privilegesGrid.Cols[COL_ISGRANTED].Index].Value = true;
                }
            }
            #endregion

            //Update changes to the Datatable which binded to gridview.
            FindAndUpdateTable(_granteeName, _privilegesGrid.Rows[e.RowIndex].Cells[COL_COMPONENT].Value.ToString(),
                _privilegesGrid.Rows[e.RowIndex].Cells[COL_PRIVILEGES].Value.ToString(),
               (bool)_privilegesGrid.Rows[e.RowIndex].Cells[COL_ISGRANTED].Value,
               (bool)_privilegesGrid.Rows[e.RowIndex].Cells[COL_WITHGRANTOPTION].Value,
               selectedGrantor); 
           
            if (OnComponentPrivilegesChangedImpl != null)
            {
                OnComponentPrivilegesChangedImpl();
            }

        }
        ///// <summary>
        ///// Display related component privilges for the grantee.
        ///// </summary>
        ///// <param name="sender"></param>
        ///// <param name="e"></param>
        //private void _componentComboBox_SelectedIndexChanged(object sender, EventArgs e)
        //{
        //    if (_granteeName != null)
        //    {
        //        ShowPrivilegesToGrid(_granteeName, false);
        //    }
        //}
        #endregion

        #region public functions
        /// <summary>
        /// Load all components 
        /// </summary>
        //public void LoadComponents()
        //{
        //    TrafodionSystem _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_connectionDefinition);
        //    if (_sqlMxSystem.Components.Count > 0)
        //    {
        //        string[] componentNames = (from component in _sqlMxSystem.Components select component.ComponentName).Distinct().ToArray();
        //        _componentComboBox.Items.Clear();
        //        _componentComboBox.Items.Add("ALL");
        //        _componentComboBox.Items.AddRange(componentNames);
        //    }
        //    _componentComboBox.SelectedIndexChanged += _componentComboBox_SelectedIndexChanged;
        //    if (!string.IsNullOrEmpty(_defaultComponentName))
        //    {
        //        _componentComboBox.SelectedItem = _defaultComponentName;
        //    }
        //}


        /// <summary>
        /// Clear all privileges data and saved changes from change.
        /// </summary>
        public void ClearPrivileges()
        {
            _privilegesGrid.Rows.Clear();
            _listCommandString.Clear();
            _listPrivilgeForGrantColChanged.Clear();
            _listPrivilgeForWithGrantColChanged.Clear();
            _privilegesGroupBox.Text = "Privileges";
        }

        /// <summary>
        /// Restore all component privileges to the unchanged.
        /// </summary>
        public void ResetAllPrivileges()
        {
            _grantedByComboBox.Text = string.Empty;

            if (_componentPrivilegesTable != null)
            {
                _privilegesGrid.Rows.Clear();
                _componentPrivilegesTable = _componentPrivilegesTableCopy.Copy();
                ShowPrivilegesToGrid(_granteeName, false);             
            }
            _listCommandString.Clear();
            _listPrivilgeForGrantColChanged.Clear();
            _listPrivilgeForWithGrantColChanged.Clear();

            if (OnComponentPrivilegesChangedImpl != null)
            {
                OnComponentPrivilegesChangedImpl();
            }
        }
        /// <summary>
        /// When deleting a grantee from list,it means all related privileges should be revoked.
        /// </summary>
        /// <returns></returns>
        public List<string> getRevokeAllPrivilegesCommand(string selectedComponentName)
        {
            List<string> listReturn = new List<string>();
            Hashtable htRevokePrivilges = new Hashtable();
            List<string> listPrivilges = new List<string>();
            string componentName = string.Empty;
            bool tmpGranted;
            bool tmpIsWithGrantOption;
            foreach (DataRow orginalRow in _componentPrivilegesTableCopy.Rows)
            {
                tmpGranted = (bool)orginalRow[COL_ISGRANTED];
                tmpIsWithGrantOption = (bool)orginalRow[COL_WITHGRANTOPTION];
                componentName = orginalRow[COL_COMPONENT].ToString();
                if (!selectedComponentName.Equals(AlterComponentPrivilegesUserControl.STR_COMPONENT_ALL) 
                    && string.Compare(componentName,selectedComponentName,true)!=0)
                {
                    continue;
                }
                if (tmpGranted)
                {
                    if (htRevokePrivilges.Contains(componentName))
                    {
                        listPrivilges = (List<string>)htRevokePrivilges[componentName];
                        htRevokePrivilges.Remove(componentName);
                    }
                    else
                    {
                        listPrivilges = new List<string>();
                    }

                    listPrivilges.Add(orginalRow[COL_PRIVILEGES].ToString());
                    htRevokePrivilges.Add(componentName, listPrivilges);
                }
            }

            if (htRevokePrivilges.Count > 0)
            {
                listReturn.AddRange(buildSQLCommand(htRevokePrivilges, "REVOKE"));
            }

            return listReturn;
        }

        /// <summary>
        /// Show all component privilges for a grantee in the grid view.
        /// </summary>
        /// <param name="granteeName"></param>
        public void ShowPrivilegesToGrid(string granteeName,bool needLoading)
        {
            //Clear user sorting property for privileges grid table.
            _privilegesGrid.SortObject.Clear();

            if (needLoading)
            {
                _listCommandString.Clear();
                _listPrivilgeForGrantColChanged.Clear();
                _listPrivilgeForWithGrantColChanged.Clear();
                LoadAllPrivilegesByGrantee(granteeName);
            }

            if (_componentPrivilegesTable != null)
            {
                _privilegesGrid.Rows.Clear();                
                foreach (DataRow dr in _componentPrivilegesTable.Rows)
                {
                    if (!_componentName.Equals("ALL") && !dr[COL_COMPONENT].ToString().Equals(_componentName))
                        continue;
                    iGRow row = _privilegesGrid.Rows.Add();
                    row.Cells[COL_COMPONENT].Value = dr[COL_COMPONENT].ToString();
                    row.Cells[COL_PRIVILEGES].Value = dr[COL_PRIVILEGES].ToString();
                    row.Cells[COL_DESCRIPTION].Value = dr[COL_DESCRIPTION].ToString();
                    row.Cells[COL_GRANTOR].Value = dr[COL_GRANTOR].ToString();
                    row.Cells[COL_WITHGRANTOPTION].Value = dr[COL_WITHGRANTOPTION];
                    row.Cells[COL_ISGRANTED].Value = dr[COL_ISGRANTED];
                }

                /*
                * Compatibility for M6 & M7-
                * For M6, the availability is determined by _IsWithGrantOptionEnabled
                * For M7, the availability is determined by IsPublicGranteeType. 
                * It's new requirement since M7: If IsPublicGranteeType, the With Grant Option column should be disabled
                */
                if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    _privilegesGrid.Cols[COL_WITHGRANTOPTION].CellStyle.ReadOnly = _IsWithGrantOptionEnabled ? iGBool.False : iGBool.True;
                }
                else
                {
                    _privilegesGrid.Cols[COL_WITHGRANTOPTION].CellStyle.ReadOnly = IsPublicGranteeType ? iGBool.True : iGBool.False;
                }
            }            
            _privilegesGroupBox.Text = "Privileges for grantee " + granteeName;
        }

        #endregion

        #region private functions
        /// <summary>
        /// Load all component privileges list from server and store in dataTable
        /// </summary>
        /// <param name="granteeName"></param>
        private void LoadAllPrivilegesByGrantee(string granteeName)
        {
            if (granteeName == null || string.IsNullOrEmpty(granteeName))
            {
                return;
            }
            TrafodionSystem _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_connectionDefinition);
            _granteeName = granteeName;
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
                 where priv.GranteeName.Equals(granteeName)
                 select priv).ToArray();
                var q =

                from c in _sqlMxSystem.ComponentPrivilegeUsages.Where<ComponentPrivilegeUsage>(c => selectedComponents.Contains(c.ComponentUID))

                join p in privilegesForUser on c.ComponentUID equals p.ComponentUID into ps

                from pr in ps.Where<ComponentPrivilege>(pr => pr.ComponentUID == c.ComponentUID && pr.PrivType.Equals(c.PrivType)).DefaultIfEmpty()
                select new
                {
                    ComponentID = c.ComponentUID,
                    PrivilegeName = c.PrivName,
                    PrivilegeType=c.PrivType,
                    IsGranted = pr == null ? false : true,
                    WithGrant = pr == null ? false : pr.Grantable,
                    Grantor = pr == null ? "" : pr.GrantorName
                };
                _componentPrivilegesTable = initComPrivMappingsTable();
                foreach (var v in q)
                {
                    DataRow dr = _componentPrivilegesTable.NewRow();
                    dr[COL_GRANTEE] = granteeName;
                    dr[COL_COMPONENT] = _sqlMxSystem.GetComponentName(v.ComponentID);
                    dr[COL_PRIVILEGES] = v.PrivilegeName;
                    dr[COL_DESCRIPTION] = _sqlMxSystem.GetComponentPrivilegeDescription(v.ComponentID, v.PrivilegeType);
                    dr[COL_ISGRANTED] = v.IsGranted;
                    dr[COL_WITHGRANTOPTION] = v.WithGrant;
                    dr[COL_GRANTOR] = v.Grantor;

                    _componentPrivilegesTable.Rows.Add(dr);
                }

                _componentPrivilegesTableCopy = _componentPrivilegesTable.Copy();


            }
        }

        /// <summary>
        /// update user input to the datatable.
        /// </summary>
        /// <param name="granteeName"></param>
        /// <param name="componetName"></param>
        /// <param name="privilegeName"></param>
        /// <param name="isGranted"></param>
        /// <param name="isWithGrantedOption"></param>
        /// <param name="grantedBy"></param>
        private void FindAndUpdateTable(string granteeName, string componetName, string privilegeName,
            bool isGranted, bool isWithGrantedOption, string grantedBy)
        {            
            if (_componentPrivilegesTable == null) return;
            foreach (DataRow dr in _componentPrivilegesTable.Rows)
            {
                if (dr[COL_GRANTEE].ToString().Equals(granteeName) &&
                    dr[COL_COMPONENT].ToString().Equals(componetName) &&
                    dr[COL_PRIVILEGES].ToString().Equals(privilegeName) &&
                    dr[COL_GRANTOR].ToString().Equals(grantedBy))
                {
                    dr[COL_ISGRANTED] = isGranted;
                    dr[COL_WITHGRANTOPTION] = isWithGrantedOption;
                    break;
                }
            }
        }    

        /// <summary>
        /// Initialize component privileges table schema,use this table for binding gridview.
        /// </summary>
        /// <returns></returns>
        private DataTable initComPrivMappingsTable()
        {
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(COL_GRANTEE, System.Type.GetType("System.String"));
            dataTable.Columns.Add(COL_COMPONENT, System.Type.GetType("System.String"));
            dataTable.Columns.Add(COL_PRIVILEGES, System.Type.GetType("System.String"));
            dataTable.Columns.Add(COL_ISGRANTED, System.Type.GetType("System.Boolean"));
            dataTable.Columns.Add(COL_DESCRIPTION, System.Type.GetType("System.String"));
            dataTable.Columns.Add(COL_WITHGRANTOPTION, System.Type.GetType("System.Boolean"));
            dataTable.Columns.Add(COL_GRANTOR, System.Type.GetType("System.String"));
            return dataTable;
        }
     
        /// <summary>
        /// Check whether the datarow has been updated.
        /// </summary>
        /// <param name="grantee"></param>
        /// <param name="componentName"></param>
        /// <param name="privilge"></param>
        /// <param name="isGranted"></param>
        /// <param name="isWithGrantOption"></param>
        /// <param name="grantedBy"></param>
        /// <returns></returns>
        private bool getUpdated(string grantee, string componentName, string privilge, 
                                ref bool isGranted, ref bool isWithGrantOption,string grantedBy)
        {
            foreach (DataRow dr in _componentPrivilegesTableCopy.Rows)
            {
                if (dr[COL_GRANTEE].ToString().Equals(grantee) &&
                    dr[COL_COMPONENT].ToString().Equals(componentName) &&
                    dr[COL_PRIVILEGES].ToString().Equals(privilge)&&
                    dr[COL_GRANTOR].ToString().Equals(grantedBy))
                {
                    isGranted = (bool)dr[COL_ISGRANTED];
                    isWithGrantOption = (bool)dr[COL_WITHGRANTOPTION];
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// According to finding differences between updated table and original table to build grant/revoke command.
        /// </summary>
        /// <param name="grantee"></param>
        /// <returns></returns>
        private List<string> getAlterPrivilegesCommand(string grantee)
        {
            List<string> listReturn = new List<string>();

            Hashtable htGrantPrivilege = new Hashtable();
            Hashtable htGrantWithGrantOption = new Hashtable();
            Hashtable htRevokePrivilges = new Hashtable();
            Hashtable htRevokeOnlyWithGrantOption = new Hashtable();

            List<string> listPrivileges = new List<string>();

            bool tmpOrginalGranted = false;
            bool tmpOrginalIsWithGrantOption = false;
            bool tmpUpdatedGranted = false;
            bool tmpUpdatedIsWithGrantOption = false;
            StringBuilder sbCmd;
            string componentName = string.Empty;

            foreach (DataRow updatedRow in _componentPrivilegesTable.Rows)
            {
                componentName = updatedRow[COL_COMPONENT].ToString();

                tmpOrginalGranted = false;
                tmpOrginalIsWithGrantOption = false;
                tmpUpdatedGranted = (bool)updatedRow[COL_ISGRANTED];
                tmpUpdatedIsWithGrantOption = (bool)updatedRow[COL_WITHGRANTOPTION];

                //Get orginal privilges status.
                if (!getUpdated(updatedRow[COL_GRANTEE].ToString(), updatedRow[COL_COMPONENT].ToString(),
                    updatedRow[COL_PRIVILEGES].ToString(), ref tmpOrginalGranted, ref tmpOrginalIsWithGrantOption, 
                    updatedRow[COL_GRANTOR].ToString()))
                    return listReturn;

                //user didn't change anything.
                if (tmpUpdatedGranted == tmpOrginalGranted &&
                    tmpUpdatedIsWithGrantOption == tmpOrginalIsWithGrantOption)
                {
                    continue;
                }

                #region Grant or Revoke
                if (tmpOrginalGranted == false && tmpUpdatedGranted == true)
                {
                    #region GRANT
                    if (tmpUpdatedIsWithGrantOption)//grant privilege and With grant option
                    {
                        if (htGrantWithGrantOption.Contains(componentName))
                        {
                            listPrivileges = (List<string>)htGrantWithGrantOption[componentName];
                            htGrantWithGrantOption.Remove(componentName);
                        }
                        else
                        {
                            listPrivileges = new List<string>();
                        }

                        if (!listPrivileges.Contains(updatedRow[COL_PRIVILEGES].ToString())) 
                            listPrivileges.Add(updatedRow[COL_PRIVILEGES].ToString());
                        htGrantWithGrantOption.Add(componentName, listPrivileges);
                    }
                    else//grant privilege without WITH GRANT OPTION
                    {
                        if (htGrantPrivilege.Contains(componentName))
                        {
                            listPrivileges = (List<string>)htGrantPrivilege[componentName];
                            htGrantPrivilege.Remove(componentName);
                        }
                        else
                        {
                            listPrivileges = new List<string>();
                        }

                        if (!listPrivileges.Contains(updatedRow[COL_PRIVILEGES].ToString())) 
                            listPrivileges.Add(updatedRow[COL_PRIVILEGES].ToString());
                        htGrantPrivilege.Add(componentName, listPrivileges);
                    }
                    #endregion
                }
                else
                {
                    if (tmpOrginalGranted && tmpOrginalIsWithGrantOption)
                    {
                        #region REVOKE
                        //Revoke privileges
                        if (!tmpUpdatedGranted && !tmpUpdatedIsWithGrantOption)
                        {

                            if (htRevokePrivilges.Contains(componentName))
                            {
                                listPrivileges = (List<string>)htRevokePrivilges[componentName];
                                htRevokePrivilges.Remove(componentName);
                            }
                            else
                            {
                                listPrivileges = new List<string>();
                            }

                            if (!listPrivileges.Contains(updatedRow[COL_PRIVILEGES].ToString())) 
                                listPrivileges.Add(updatedRow[COL_PRIVILEGES].ToString());
                            htRevokePrivilges.Add(componentName, listPrivileges);
                        }
                        else if (tmpUpdatedGranted && !tmpUpdatedIsWithGrantOption)//Only revoke WITH GRANT OPTION
                        {
                            if (htRevokeOnlyWithGrantOption.Contains(componentName))
                            {
                                listPrivileges = (List<string>)htRevokeOnlyWithGrantOption[componentName];
                                htRevokeOnlyWithGrantOption.Remove(componentName);
                            }
                            else
                            {
                                listPrivileges = new List<string>();
                            }

                            if (!listPrivileges.Contains(updatedRow[COL_PRIVILEGES].ToString())) 
                                listPrivileges.Add(updatedRow[COL_PRIVILEGES].ToString());
                            htRevokeOnlyWithGrantOption.Add(componentName, listPrivileges);
                        }
                        #endregion
                    }
                    else if (tmpOrginalGranted && !tmpOrginalIsWithGrantOption)
                    {

                        if (!tmpUpdatedGranted)
                        {
                            #region Revoke Privileges

                            if (htRevokePrivilges.Contains(componentName))
                            {
                                listPrivileges = (List<string>)htRevokePrivilges[componentName];
                                htRevokePrivilges.Remove(componentName);
                            }
                            else
                            {
                                listPrivileges = new List<string>();
                            }

                            if (!listPrivileges.Contains(updatedRow[COL_PRIVILEGES].ToString())) 
                                listPrivileges.Add(updatedRow[COL_PRIVILEGES].ToString());
                            htRevokePrivilges.Add(componentName, listPrivileges);
                            #endregion
                        }
                        else
                        {
                            if (tmpUpdatedIsWithGrantOption)
                            {
                                #region Grant WITH GRANT Option
                                if (htGrantWithGrantOption.Contains(componentName))
                                {
                                    listPrivileges = (List<string>)htGrantWithGrantOption[componentName];
                                    htGrantWithGrantOption.Remove(componentName);
                                }
                                else
                                {
                                    listPrivileges = new List<string>();
                                }

                                if (!listPrivileges.Contains(updatedRow[COL_PRIVILEGES].ToString())) 
                                    listPrivileges.Add(updatedRow[COL_PRIVILEGES].ToString());
                                htGrantWithGrantOption.Add(componentName, listPrivileges);
                                #endregion
                            }
                        }

                    }
                }
                #endregion
                
            }//end foreach

            #region Build Command string.
            if (htGrantPrivilege.Count>0)
            {
                listReturn.AddRange(buildSQLCommand(htGrantPrivilege, "GRANT"));
            }

            if (htGrantWithGrantOption.Count > 0)
            {
                listReturn.AddRange(buildSQLCommand(htGrantWithGrantOption, "GRANTWITH"));
            }

            if (htRevokePrivilges.Count > 0)
            {
                listReturn.AddRange(buildSQLCommand(htRevokePrivilges, "REVOKE"));
            }

            if (htRevokeOnlyWithGrantOption.Count > 0)
            {
                listReturn.AddRange(buildSQLCommand(htRevokeOnlyWithGrantOption, "REVOKEWITH"));
            }
            #endregion

            return listReturn;
        }//end function

        /// <summary>
        /// Build Grant/Revoke command according to hashtable which contains key of component name,
        /// value of privileges list.
        /// </summary>
        /// <param name="htCommand">Contains privilges list by component name as hashtable key.</param>
        /// <param name="commandType">Identify for Grant/Revoke command.</param>
        /// <returns></returns>
        private List<string> buildSQLCommand(Hashtable htCommand,string commandType)
        {
            List<string> listReturn = new List<string>();
            if (htCommand != null)
            {
                bool hasCommand = false;
                StringBuilder cmd = new StringBuilder();
                List<string> privileges;
                string strComp;
                foreach (DictionaryEntry de in htCommand)
                {
                    hasCommand = false;
                    cmd = new StringBuilder();
                    strComp = de.Key.ToString();
                    privileges = (List<string>)de.Value;
                    if (privileges != null && privileges.Count > 0)
                    {
                        hasCommand = true;
                        #region BuildCommand
                        switch (commandType)
                        {
                            case "GRANT":                                
                                cmd.Append(" GRANT COMPONENT PRIVILEGE ");
                                break;
                            case "GRANTWITH":
                                cmd.Append(" GRANT COMPONENT PRIVILEGE ");
                                break;
                            case "REVOKE":
                                cmd.Append(" REVOKE COMPONENT PRIVILEGE ");
                                break;
                            case "REVOKEWITH":
                                cmd.Append(" REVOKE GRANT OPTION FOR COMPONENT PRIVILEGE ");
                                break;
                                
                        }
                        //contact privileges.
                        foreach (string priv in privileges)
                        {
                            cmd.Append(priv);
                            cmd.Append(",");
                        }
                        cmd.Remove(cmd.Length - 1, 1);
                        cmd.Append(" ON ");
                        cmd.Append(strComp);
                        switch (commandType)
                        {
                            case "GRANT":
                                cmd.Append(" TO ");
                                cmd.Append(Utilities.ExternalUserName(_granteeName));
                                break;
                            case "GRANTWITH":
                                cmd.Append(" TO ");
                                cmd.Append(Utilities.ExternalUserName(_granteeName));
                                cmd.Append(" WITH GRANT OPTION ");
                                break;
                            case "REVOKE":
                                cmd.Append(" FROM ");
                                cmd.Append(Utilities.ExternalUserName(_granteeName));
                                break;
                            case "REVOKEWITH":
                                cmd.Append(" FROM ");
                                cmd.Append(Utilities.ExternalUserName(_granteeName));
                                break;
                        }

                        if(!string.IsNullOrEmpty(_grantedByComboBox.Text.Trim()))
                        {
                            cmd.Append (" GRANTED BY " + Utilities.ExternalUserName(_grantedByComboBox.Text.Trim()));
                        }
                        
                        #endregion
                    }

                    if (hasCommand)
                    {
                        listReturn.Add(cmd.ToString());
                    }
                    
                }
            }
            return listReturn;
        }
        #endregion
    }
}
