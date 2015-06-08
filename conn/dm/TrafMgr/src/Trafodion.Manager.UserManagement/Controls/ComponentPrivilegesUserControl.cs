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
using System.ComponentModel;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class ComponentPrivilegesUserControl : UserControl
    {
        #region Fields
        public delegate void OnChanged();
        public event OnChanged OnChangedImpl;

        private string _captionForSelectionDialog;
        ComponentPrivilegesSelectionPanel componentPrivilegesSelectionPanel;
        ConnectionDefinition _theConnectionDefinition;
        private string _granteeName;
        
        private const string COL_COMPONENT = "Component";
        private const string COL_PRIVILEGES = "Privilege";
        private const string COL_WITHGRANTOPTION = "With Grant Option";
        private const string COL_GRANTOR = "Grantor";
        private const string COL_DESCRIPTION = "Description";

        //using for update,along with change by grid edit.
        DataTable _dtComponentPrivilegesTable = new DataTable();

        //using for save original table,for tracing changes by user.
        DataTable _dtOriginalComponentPrivilegesTable = new DataTable();
        
        //After user changes privileges,save the traces changes of Revoking.
        private List<string> _listRevokePrivilegesCmd = new List<string>();

        //After user changes privileges,save the traces changes of rather than Revoking.
        private List<string> _listAlterPrivilegesCmd = new List<string>();
        private bool _isEdited = false;

        #endregion
        
        #region Properties

        /// <summary>
        /// First comparing the row count of component privileges,
        /// if row count is identical with before,then check whether user get remove or add privileges items,
        /// at last check the revoking or altering privileges command.
        /// </summary>
        public bool PrivilegesChanged
        {
            get
            {
                if (_dtComponentPrivilegesTable.Rows.Count == 0 && _dtOriginalComponentPrivilegesTable.Rows.Count == 0)
                {
                    return false;
                }
                
                if (_dtComponentPrivilegesTable.Rows.Count!=_dtOriginalComponentPrivilegesTable.Rows.Count)
                {
                    return true;
                }

                if (_isEdited)
                {
                    return GetRevokePrivilegesCommand(string.Empty).Count > 0 || GetAlterPrivilegesCommand(string.Empty).Count > 0;
                }
                else
                {
                    return false;
                }
               

            }
        }


        /// <summary>
        /// Get Revoke Privileges Command
        /// </summary>
        /// <param name="grantedByClause"></param>
        /// <returns></returns>
        public List<string> GetRevokePrivilegesCommand(string grantedByClause)
        {
            _listRevokePrivilegesCmd = getRevokePrivilegesCommand(_granteeName, grantedByClause);
            return _listRevokePrivilegesCmd;
        }


        /// <summary>
        /// Get Alter Privileges Command
        /// </summary>
        /// <param name="grantedByClause"></param>
        /// <returns></returns>
        public List<string> GetAlterPrivilegesCommand(string grantedByClause)
        {
            _listAlterPrivilegesCmd = getAlterPrivilegesCommand(_granteeName, grantedByClause);
            return _listAlterPrivilegesCmd;
        }

        /// <summary>
        /// Clear privilige command from the list
        /// </summary>
        public void ClearPrivilegeCommand()
        {
            if (_listAlterPrivilegesCmd != null)
            {
                _listAlterPrivilegesCmd.Clear();
            }

            if (_listRevokePrivilegesCmd != null)
            {
                _listRevokePrivilegesCmd.Clear();
            }
        }
        
        //public List<string> ListRevokePrivilegesCommand
        //{
        //    get
        //    {
        //        _listRevokePrivilegesCmd = getRevokePrivilegesCommand(_granteeName, this.GrantedByClause);
        //        return _listRevokePrivilegesCmd;
        //    }
        //    set { _listRevokePrivilegesCmd = value; }
        //}

        //public List<string> ListAlterPrivilegesCommand
        //{           
        //    get
        //    {
        //        _listAlterPrivilegesCmd = getAlterPrivilegesCommand(_granteeName);
        //        return _listAlterPrivilegesCmd;
        //    }
        //    set { _listAlterPrivilegesCmd = value; }
        //}

        public DataTable OriginalComponentPrivilegesTable
        {
            get { return _dtOriginalComponentPrivilegesTable; }
            set { _dtOriginalComponentPrivilegesTable = value; }
        }

        public string CaptionForSelectionDialog
        {            
            set { _captionForSelectionDialog = value; }
        }

        public string GranteeName
        {
            set { _granteeName = value; }
        }

        public DataTable ComponentPrivilegesTable
        {
            get { return _dtComponentPrivilegesTable; }
        }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public List<DataRow> Privileges
        {
            get
            {
                return _dtComponentPrivilegesTable.AsEnumerable().ToList();
            }
            set
            {
                Hashtable htTempforSaveWithGrantOption = new Hashtable();
                Hashtable htTempForSaveGrantor = new Hashtable();
                foreach (DataRow gridDr in _dtComponentPrivilegesTable.Rows)
                {
                    string txtKey = gridDr[COL_COMPONENT].ToString() + "#" + gridDr[COL_PRIVILEGES].ToString();
                    if (!htTempforSaveWithGrantOption.ContainsKey(txtKey) && (bool)gridDr[COL_WITHGRANTOPTION])
                    {
                        htTempforSaveWithGrantOption.Add(txtKey, true);
                    }
                    if (!htTempForSaveGrantor.ContainsKey(txtKey))
                    {
                        if (_dtComponentPrivilegesTable.Columns.Contains(COL_GRANTOR))
                        {
                            htTempForSaveGrantor.Add(txtKey, gridDr[COL_GRANTOR].ToString());
                        }                       
                    }
                }

                _dtComponentPrivilegesTable.Rows.Clear();
                _comPrivilegesGrid.Rows.Clear();
                bool blnWithGrantOption = false;
                string strGrantor = string.Empty;
                foreach (DataRow dr in value)
                {
                    blnWithGrantOption = false;
                    strGrantor = string.Empty;
                    string txtKey = dr[COL_COMPONENT].ToString() + "#" + dr[COL_PRIVILEGES].ToString();
                    if (htTempforSaveWithGrantOption.ContainsKey(txtKey))
                    {
                        blnWithGrantOption = true;
                    }

                    if (htTempForSaveGrantor.ContainsKey(txtKey))
                    {
                        strGrantor = htTempForSaveGrantor[txtKey] as string;
                    }
                    iGRow row = _comPrivilegesGrid.Rows.Add();
                    row.Cells[COL_COMPONENT].Value = dr[COL_COMPONENT].ToString();
                    row.Cells[COL_PRIVILEGES].Value = dr[COL_PRIVILEGES].ToString();                    
                    row.Cells[COL_DESCRIPTION].Value = dr[COL_DESCRIPTION].ToString();
                    row.Cells[COL_WITHGRANTOPTION].Value = blnWithGrantOption;
                    if (_dtComponentPrivilegesTable.Columns.Contains(COL_GRANTOR))
                    {
                        row.Cells[COL_GRANTOR].Value = strGrantor;
                    }                    
                    DataRow newRow = _dtComponentPrivilegesTable.NewRow();
                    newRow[COL_COMPONENT] = dr[COL_COMPONENT].ToString();
                    newRow[COL_PRIVILEGES] = dr[COL_PRIVILEGES].ToString();
                    newRow[COL_WITHGRANTOPTION] = blnWithGrantOption;
                    newRow[COL_DESCRIPTION] = dr[COL_DESCRIPTION].ToString();
                    if (_dtComponentPrivilegesTable.Columns.Contains(COL_GRANTOR))
                    {
                        newRow[COL_GRANTOR] = strGrantor;
                    }
                    _dtComponentPrivilegesTable.Rows.Add(newRow);
                }

                _isEdited = true;
            }
        }

        #endregion

        #region Constructors

        public ComponentPrivilegesUserControl()
        {
            InitializeComponent();
            _comPrivilegesGrid.Clear();

            _comPrivilegesGrid.Cols.Add(COL_COMPONENT, COL_COMPONENT);
            _comPrivilegesGrid.Cols.Add(COL_PRIVILEGES, COL_PRIVILEGES);

            iGCol withGrantCol = _comPrivilegesGrid.Cols.Add(COL_WITHGRANTOPTION, COL_WITHGRANTOPTION);
            withGrantCol.ColHdrStyle.TextAlign = iGContentAlignment.MiddleCenter;
            withGrantCol.CellStyle.ImageAlign = iGContentAlignment.MiddleCenter;
            withGrantCol.CellStyle.Type = iGCellType.Check;
            withGrantCol.CellStyle.SingleClickEdit = iGBool.True;
            
            _comPrivilegesGrid.Cols.Add(COL_GRANTOR, COL_GRANTOR);

            _comPrivilegesGrid.Cols.Add(COL_DESCRIPTION, COL_DESCRIPTION);
            _comPrivilegesGrid.AfterCommitEdit += new iGAfterCommitEditEventHandler(_comPrivilegesGrid_AfterCommitEdit);
            _comPrivilegesGrid.SelectionChanged += new EventHandler(_comPrivilegesGrid_SelectionChanged);
            _comPrivilegesGrid.AutoResizeCols = true;
            _comPrivilegesGrid.RowMode = true;
            _comPrivilegesGrid.ReadOnly = false;
            _comPrivilegesGrid.AllowColumnFilter = false;
        }

        public ComponentPrivilegesUserControl(ConnectionDefinition aConnectionDefinition, string granteeName)
            : this()
        {
            _theConnectionDefinition = aConnectionDefinition;
            _granteeName = granteeName;
        }

        void _comPrivilegesGrid_SelectionChanged(object sender, EventArgs e)
        {
            updateControls();
        }

        #endregion

        #region Events

        private void _comPrivilegesGrid_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e)
        {
            _isEdited = true;
            if (e.ColIndex == _comPrivilegesGrid.Cols[COL_WITHGRANTOPTION].Index)
            {
                bool isWithGrantedOption;
                if ("true".Equals(_comPrivilegesGrid.Rows[e.RowIndex].Cells[e.ColIndex].Value.ToString(),
                    System.StringComparison.InvariantCultureIgnoreCase))
                {
                    isWithGrantedOption = true;             
                }
                else
                {
                    isWithGrantedOption = false;
                }

                //Sync to datatable
                foreach (DataRow dr in _dtComponentPrivilegesTable.Rows)
                {
                    if (_comPrivilegesGrid.Rows[e.RowIndex].Cells[COL_COMPONENT].Value.ToString().Equals(
                        dr[COL_COMPONENT].ToString()) &&
                        _comPrivilegesGrid.Rows[e.RowIndex].Cells[COL_PRIVILEGES].Value.ToString().Equals(
                        dr[COL_PRIVILEGES].ToString()))
                    {
                        dr[COL_WITHGRANTOPTION] = isWithGrantedOption;
                    }
                }
            }
            updateControls();
        }
             
        private void addComPrivToolStripButton_Click(object sender, EventArgs e)
        {
            showComPrivilegesDialog(_granteeName);
            updateControls();
        }
        

        /// <summary>
        /// remove selected rows in both gridview and datatable.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void deleteComPrivToolStripButton_Click(object sender, EventArgs e)
        {
            if (_comPrivilegesGrid.SelectedRowIndexes.Count > 0)
            {
                List<DataRow> delPrivilegesRows = new List<DataRow>();
                foreach (int index in _comPrivilegesGrid.SelectedRowIndexes)
                {
                    string componentName = _comPrivilegesGrid.Rows[_comPrivilegesGrid.SelectedRowIndexes[0]].Cells[COL_COMPONENT].Value.ToString();
                    string privilegeName = _comPrivilegesGrid.Rows[_comPrivilegesGrid.SelectedRowIndexes[0]].Cells[COL_PRIVILEGES].Value.ToString();
                    _comPrivilegesGrid.Rows.RemoveAt(_comPrivilegesGrid.SelectedRowIndexes[0]);
                    foreach (DataRow dr in _dtComponentPrivilegesTable.Rows)
                    {
                        if (dr[COL_COMPONENT].ToString().Equals(componentName) &&
                            dr[COL_PRIVILEGES].ToString().Equals(privilegeName))
                        {
                            delPrivilegesRows.Add(dr);
                        }
                    }
                }

                foreach (DataRow row in delPrivilegesRows)
                {
                    _dtComponentPrivilegesTable.Rows.Remove(row);
                }
                _isEdited = true; 
            }
            updateControls();
        }
       
        /// <summary>
        /// Delete all privileges from gridview and datatable.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theRemoveAllComPrivStripButton_Click(object sender, EventArgs e)
        {
            _comPrivilegesGrid.Rows.Clear();
            _dtComponentPrivilegesTable.Clear();
            _isEdited = true;
            updateControls();
        }

        #endregion

        #region Private Functions

        /// <summary>
        /// Display component privileges selection dialog.
        /// </summary>
        /// <param name="granteeName"></param>
        private void showComPrivilegesDialog(string granteeName)
        {
            if (componentPrivilegesSelectionPanel == null)
            {
                componentPrivilegesSelectionPanel = new ComponentPrivilegesSelectionPanel(_theConnectionDefinition, granteeName);
            }

            componentPrivilegesSelectionPanel.SelectedPrivileges = Privileges;

            ManageUserDialog dialog = new ManageUserDialog();
            dialog.ShowControl(componentPrivilegesSelectionPanel, _captionForSelectionDialog + ": Select Component Privileges");
            if (dialog.DialogResult == DialogResult.OK)
            {
                if (componentPrivilegesSelectionPanel.SelectedPrivileges.Count >= 0)
                {
                    Privileges = componentPrivilegesSelectionPanel.SelectedPrivileges;
                }

            }
        }

        /// <summary>
        /// Get Privilige data from the ComponentPrivilegesTable
        /// </summary>
        /// <param name="htGrantPrivilege"></param>
        /// <param name="htGrantWithGrantOption"></param>
        public void GetPrivilegeData(Hashtable htGrantPrivilege, Hashtable htGrantWithGrantOption)
        {
            List<string> listPrivilges = new List<string>();

            bool tmpIsWithGrantOption = false;

            StringBuilder sbCmd;
            string componentName = string.Empty;

            foreach (DataRow updatedRow in this.ComponentPrivilegesTable.Rows)
            {
                componentName = updatedRow[COL_COMPONENT].ToString();
                tmpIsWithGrantOption = (bool)updatedRow[COL_WITHGRANTOPTION];

                #region GRANT
                if (tmpIsWithGrantOption)
                {
                    if (htGrantWithGrantOption.Contains(componentName))
                    {
                        listPrivilges = (List<string>)htGrantWithGrantOption[componentName];
                        htGrantWithGrantOption.Remove(componentName);
                    }
                    else
                    {
                        listPrivilges = new List<string>();
                    }

                    listPrivilges.Add(updatedRow[COL_PRIVILEGES].ToString());
                    htGrantWithGrantOption.Add(componentName, listPrivilges);
                }
                else//grant privilege without WITH GRANT OPTION
                {
                    if (htGrantPrivilege.Contains(componentName))
                    {
                        listPrivilges = (List<string>)htGrantPrivilege[componentName];
                        htGrantPrivilege.Remove(componentName);
                    }
                    else
                    {
                        listPrivilges = new List<string>();
                    }

                    listPrivilges.Add(updatedRow[COL_PRIVILEGES].ToString());
                    htGrantPrivilege.Add(componentName, listPrivilges);
                }

                #endregion
            }//end foreach
        }

        /// <summary>
        /// Binding privileges table list to gridview.
        /// </summary>
        /// <param name="privilegesTable">Privileges data table</param>
        public void ShowComponentsPrivileges(DataTable privilegesTable)
        {
            _dtComponentPrivilegesTable = privilegesTable;
            if (_dtComponentPrivilegesTable != null && _dtComponentPrivilegesTable.Rows.Count > 0)
            {
                _comPrivilegesGrid.FillWithData(_dtComponentPrivilegesTable, true);
            }
            _isEdited = false;

            if (!privilegesTable.Columns.Contains(COL_GRANTOR))
            {
                _comPrivilegesGrid.Cols.RemoveAt(COL_GRANTOR);
            }
        }

        /// <summary>
        /// Binding privileges table list to gridview.
        /// </summary>
        /// <param name="privilegesTable">Privileges data table</param>
        /// <param name="isEnableWithGrantOption">If enable WithGrantOption</param>
        public void ShowComponentsPrivileges(DataTable privilegesTable, bool isEnableWithGrantOption)
        {
            ShowComponentsPrivileges(privilegesTable);
            if (!isEnableWithGrantOption)
            {
                _comPrivilegesGrid.Cols[COL_WITHGRANTOPTION].Visible = false;
                _comPrivilegesGrid.ReadOnly = true;
            }
            else
            {
                _comPrivilegesGrid.Cols[COL_WITHGRANTOPTION].Visible = true;
                _comPrivilegesGrid.Cols[COL_WITHGRANTOPTION].CellStyle.SingleClickEdit = iGBool.True;
                _comPrivilegesGrid.ReadOnly = false;
            }
        } 

        private void updateControls()
        {
            deleteComPrivToolStripButton.Enabled = _comPrivilegesGrid.SelectedRowIndexes.Count > 0;
            if (OnChangedImpl != null)
            {
                OnChangedImpl();
            }
        }
        
           /// <summary>
        /// Get revoked privilges by finding between orginal privileges table and updated privileges table.
        /// </summary>
        /// <param name="grantee">grantee name</param>
        /// <param name="grantedByClause">GRANTED BY clause for Grant/Revoke command.</param>
        /// <returns>Revoking privileges command that have been grouped by component and grantee</returns>
        private List<string> getRevokePrivilegesCommand(string grantee, string grantedByClause)
        {
            _listRevokePrivilegesCmd = new List<string>();
            DataTable updatedTbl = _dtComponentPrivilegesTable;

            bool isExistedInUpdatedTable = false;
            Hashtable htRevokePrivilges = new Hashtable();
            List<string> listPrivilges = new List<string>();
            string componentName = string.Empty;

            foreach (DataRow drOrginal in _dtOriginalComponentPrivilegesTable.Rows)
            {
                isExistedInUpdatedTable = false;
                componentName = drOrginal[COL_COMPONENT].ToString();

                foreach (DataRow dr in updatedTbl.Rows)
                {
                    if (dr[COL_COMPONENT].ToString().Equals(componentName) &&
                        dr[COL_PRIVILEGES].ToString().Equals(drOrginal[COL_PRIVILEGES].ToString()))
                    {
                        isExistedInUpdatedTable = true;
                        break;
                    }
                }

                //Not existed in the updated table,so it should be removed by user
                if (!isExistedInUpdatedTable)
                {                    
                    #region Revoke privilege

                    if (htRevokePrivilges.Contains(componentName))
                    {
                        listPrivilges = (List<string>)htRevokePrivilges[componentName];
                        htRevokePrivilges.Remove(componentName);
                    }
                    else
                    {
                        listPrivilges = new List<string>();
                    }
                    if (!listPrivilges.Contains(drOrginal[COL_PRIVILEGES].ToString()))
                        listPrivilges.Add(drOrginal[COL_PRIVILEGES].ToString());
                    htRevokePrivilges.Add(componentName, listPrivilges);
                    #endregion
                }
            }
            if (htRevokePrivilges.Count > 0)
            {
                _listRevokePrivilegesCmd.AddRange(buildSQLCommand(htRevokePrivilges, "REVOKE", grantedByClause));
            }
            return _listRevokePrivilegesCmd;

        }

        /// <summary>
        /// Get Alter privilges by finding between orginal privileges table and updated privileges table.
        /// </summary>
        /// <param name="grantee">grantee name</param>
        /// <param name="grantedByClause">GRANTED BY clause for Grant/Revoke command.</param>
        /// <returns>Alter privileges command that have been grouped by component and grantee</returns>
        private List<string> getAlterPrivilegesCommand(string grantee, string grantedByClause)
        {
            _listAlterPrivilegesCmd = new List<string>();
            DataTable updatedTbl = _dtComponentPrivilegesTable;

            Hashtable htGrantPrivilege = new Hashtable();
            Hashtable htGrantWithGrantOption = new Hashtable();
            Hashtable htRevokeOnlyWithGrantOption = new Hashtable();

            bool isExistedInOrginalTable = false;
            string componentName = string.Empty;
            List<string> listPrivilges = new List<string>();

            foreach (DataRow dr in updatedTbl.Rows)
            {
                componentName = dr[COL_COMPONENT].ToString();
                isExistedInOrginalTable = false;

                foreach (DataRow drOriginal in _dtOriginalComponentPrivilegesTable.Rows)
                {
                    #region Get the changes for GRANT/REVOKE WITH GRANT OPTION
                    if (componentName.Equals(drOriginal[COL_COMPONENT].ToString()) &&
                        dr[COL_PRIVILEGES].ToString().Equals(drOriginal[COL_PRIVILEGES].ToString()))
                    {
                        isExistedInOrginalTable = true;
                        if ((bool)dr[COL_WITHGRANTOPTION] != (bool)drOriginal[COL_WITHGRANTOPTION])
                        {
                            //user checked the WITH GRANT OPTION
                            if ((bool)dr[COL_WITHGRANTOPTION])
                            {
                                if (htGrantWithGrantOption.Contains(drOriginal[COL_COMPONENT].ToString()))
                                {
                                    listPrivilges = (List<string>)htGrantWithGrantOption[componentName];
                                    htGrantWithGrantOption.Remove(componentName);
                                }
                                else
                                {
                                    listPrivilges = new List<string>();
                                }
                                if (!listPrivilges.Contains(dr["Privilege"].ToString()))
                                        listPrivilges.Add(dr["Privilege"].ToString());
                                htGrantWithGrantOption.Add(componentName, listPrivilges);
                            }
                            else //user unchecked the WITH GRANT OPTION
                            {
                                if (htRevokeOnlyWithGrantOption.Contains(componentName))
                                {
                                    listPrivilges = (List<string>)htRevokeOnlyWithGrantOption[componentName];
                                    htRevokeOnlyWithGrantOption.Remove(componentName);
                                }
                                else
                                {
                                    listPrivilges = new List<string>();
                                }
                                if (!listPrivilges.Contains(dr["Privilege"].ToString()))
                                        listPrivilges.Add(dr["Privilege"].ToString());
                                htRevokeOnlyWithGrantOption.Add(componentName, listPrivilges);
                            }
                        }                        
                        break;
                    }
                    #endregion
                }

                //Not existed in the orginal table,so it should be new privilege added by user
                if (!isExistedInOrginalTable)
                {
                    #region Grant new privilege

                    if ((bool)dr[COL_WITHGRANTOPTION])//grant privilege and With grant option
                    {
                        if (htGrantWithGrantOption.Contains(componentName))
                        {
                            listPrivilges = (List<string>)htGrantWithGrantOption[componentName];
                            htGrantWithGrantOption.Remove(componentName);
                        }
                        else
                        {
                            listPrivilges = new List<string>();
                        }
                        if (!listPrivilges.Contains(dr["Privilege"].ToString()))
                            listPrivilges.Add(dr["Privilege"].ToString());
                        htGrantWithGrantOption.Add(componentName, listPrivilges);
                    }
                    else//grant privilege without WITH GRANT OPTION
                    {
                        if (htGrantPrivilege.Contains(componentName))
                        {
                            listPrivilges = (List<string>)htGrantPrivilege[componentName];
                            htGrantPrivilege.Remove(componentName);
                        }
                        else
                        {
                            listPrivilges = new List<string>();
                        }

                        if (!listPrivilges.Contains(dr["Privilege"].ToString()))
                            listPrivilges.Add(dr["Privilege"].ToString());
                        htGrantPrivilege.Add(componentName, listPrivilges);
                    }
                    #endregion
                }
            }

            #region Build Command string.
            if (htGrantPrivilege.Count > 0)
            {
                _listAlterPrivilegesCmd.AddRange(buildSQLCommand(htGrantPrivilege, "GRANT", grantedByClause));
            }

            if (htGrantWithGrantOption.Count > 0)
            {
                _listAlterPrivilegesCmd.AddRange(buildSQLCommand(htGrantWithGrantOption, "GRANTWITH", grantedByClause));
            }


            if (htRevokeOnlyWithGrantOption.Count > 0)
            {
                _listAlterPrivilegesCmd.AddRange(buildSQLCommand(htRevokeOnlyWithGrantOption, "REVOKEWITH", grantedByClause));
            }
            #endregion

            return _listAlterPrivilegesCmd;
        }

        /// <summary>
        /// Build Grant/Revoke command according to hashtable which contains key of component name,
        /// value of privileges list.
        /// </summary>
        /// <param name="htCommand">Contains privilges list by component name as hashtable key.</param>
        /// <param name="commandType">Identify for Grant/Revoke command.</param>
        /// <param name="grantedByClause">GRANTED BY clause for Grant/Revoke command.</param>
        /// <returns></returns>
        private List<string> buildSQLCommand(Hashtable htCommand, string commandType, string grantedByClause)
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

                        cmd.Append(grantedByClause);

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
