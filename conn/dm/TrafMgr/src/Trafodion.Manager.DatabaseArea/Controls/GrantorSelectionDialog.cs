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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

using TenTec.Windows.iGridLib;
using System.Collections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class GrantorSelectionDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {

        #region Private Member

        private const string COL_KEY_USER_OR_ROLE_NAME = "USER_OR_ROLE_NAME";
        private const string COL_TEXT_USER_NAME = "User Name";
        private const string COL_TEXT_ROLE_NAME = "Role Name";
        private const string COL_KEY_AS_GRANTOR = "AS_GRANTOR";
        private const string COL_TEXT_AS_GRANTOR = "As Grantor";
        private const string COL_KEY_USER_OR_ROLE_TYPE = "USER_OR_ROLE_TYPE";
        private const string COL_TEXT_USER_OR_ROLE_TYPE = "User or Role Type";
        private const string COL_USER_VALUE = "USER";
        private const string COL_ROLE_VALUE = "ROLE";
        private const string NONE_USER_OR_ROLE_SELECTED = "";

        #endregion

        public GrantorSelectionDialog(ArrayList roleAndUserList)
        {
            InitializeComponent();
            InitializeDialog(roleAndUserList);
            BindGrid();
        }

        #region Property

        public ArrayList RoleAndUserList
        {
            get;
            private set;
        }

        public string GrantedBy
        {
            get;
            private set;
        }

        private bool IsSelectingUser
        {
            get
            {
                return this.rbtnUser.Checked;
            }
        }

        private bool IsSelectingRole
        {
            get
            {
                return this.rbtnRole.Checked;
            }
        }

        private bool IsUserType(string value )
        {
            return value == COL_USER_VALUE;
        }

        private bool IsRoleType(string value)
        {
            return value == COL_ROLE_VALUE;
        }

        #endregion

        #region Private Method

        private void InitializeDialog(ArrayList roleAndUserList)
        {
            this.RoleAndUserList = roleAndUserList;
            GrantedBy = NONE_USER_OR_ROLE_SELECTED;

            // Add columns to iGrid
            iGCol colUserOrRoleName = this.grdGrantor.Cols.Add(COL_KEY_USER_OR_ROLE_NAME, COL_KEY_USER_OR_ROLE_NAME, 234);
            colUserOrRoleName.CellStyle.ReadOnly = iGBool.True;

            iGCol colGrantedBy = this.grdGrantor.Cols.Add(COL_KEY_AS_GRANTOR, COL_TEXT_AS_GRANTOR, 80);
            colGrantedBy.CellStyle.ReadOnly = iGBool.False;
            colGrantedBy.CellStyle.Type = iGCellType.Check;
            colGrantedBy.CellStyle.SingleClickEdit = iGBool.True;
            colGrantedBy.CellStyle.TextAlign = iGContentAlignment.MiddleCenter;
            colGrantedBy.CellStyle.ImageAlign = iGContentAlignment.MiddleCenter;

            iGCol colUserRoleType = this.grdGrantor.Cols.Add(COL_KEY_USER_OR_ROLE_TYPE, COL_TEXT_USER_OR_ROLE_TYPE);
            colUserRoleType.Visible = false;

            this.grdGrantor.ReadOnly = false;

            // Register event to iGrid
            this.grdGrantor.AfterCommitEdit += new iGAfterCommitEditEventHandler(grdGrantor_AfterCommitEdit);
        }

        /// <summary>
        /// Bind both Users and Roles to iGrid
        /// </summary>
        private void BindGrid()
        {
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(COL_KEY_USER_OR_ROLE_NAME, System.Type.GetType("System.String"));
            dataTable.Columns.Add(COL_KEY_AS_GRANTOR, System.Type.GetType("System.Boolean"));
            dataTable.Columns.Add(COL_KEY_USER_OR_ROLE_TYPE, System.Type.GetType("System.String"));
            
            if (RoleAndUserList != null && RoleAndUserList.Count == 2)
            {
                for (int i = 0; i < 2; i++)
                {
                    /*
                     * If i = 0, it's User list
                     * If i = 1, it's Role list
                    */
                    string typeUserOrRole = string.Empty;
                    if (i == 0)
                    {
                        typeUserOrRole = COL_USER_VALUE;
                    }
                    else if(i==1)
                    {
                        typeUserOrRole = COL_ROLE_VALUE;
                    }

                    foreach (string[] userOrRoleNames in (List<string[]>)RoleAndUserList[i])
                    {
                        DataRow dr = dataTable.NewRow();
                        dr[COL_KEY_USER_OR_ROLE_NAME] = userOrRoleNames[0];
                        dr[COL_KEY_AS_GRANTOR] = false;
                        dr[COL_KEY_USER_OR_ROLE_TYPE] = typeUserOrRole;
                        dataTable.Rows.Add(dr);
                    }
                }
            }

            this.grdGrantor.FillWithData(dataTable, true);

            FilterGrid();
        }

        private void FilterGrid()
        {
            bool isUser = IsSelectingUser;
            bool isRole = IsSelectingRole;

            this.grdGrantor.Cols[COL_KEY_USER_OR_ROLE_NAME].Text = isUser ? COL_TEXT_USER_NAME : COL_TEXT_ROLE_NAME;

            foreach (iGRow row in this.grdGrantor.Rows)
            {
                string userOrRoleType = (string)row.Cells[COL_KEY_USER_OR_ROLE_TYPE].Value;
                row.Visible = IsUserType(userOrRoleType) && isUser || IsRoleType(userOrRoleType) && isRole;
            }
        }

        #endregion

        #region Event

        private void rbtnUser_CheckedChanged(object sender, EventArgs e)
        {
            FilterGrid();
        }

        private void rbtnRole_CheckedChanged(object sender, EventArgs e)
        {
            FilterGrid();
        }

        private void grdGrantor_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e)
        {
            if (e.ColIndex == this.grdGrantor.Cols[COL_KEY_AS_GRANTOR].Index)
            {
                iGCell currentGrantedByCell = this.grdGrantor.Rows[e.RowIndex].Cells[e.ColIndex];
                if ((bool)currentGrantedByCell.Value)
                {
                    foreach (iGRow row in this.grdGrantor.Rows)
                    {
                        if (e.RowIndex != row.Index)
                        {
                            iGCell grantedBycell = this.grdGrantor.Rows[row.Index].Cells[e.ColIndex];
                            grantedBycell.Value = false;
                        }
                    }
                    GrantedBy = this.grdGrantor.Rows[e.RowIndex].Cells[COL_KEY_USER_OR_ROLE_NAME].Value.ToString();
                    btnOK.Enabled = true;
                }
                else
                {
                    GrantedBy = NONE_USER_OR_ROLE_SELECTED;
                    btnOK.Enabled = false;
                }
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        #endregion
    }
}
