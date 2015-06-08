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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using System.Collections.Generic;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class LibraryPrivilegeOptionsControl : UserControl, IPrivilegeOptionsProvider
    {
        #region private member variables

        GrantRevokeControl _grantRevokeTool;

        #endregion private member variables

        public LibraryPrivilegeOptionsControl()
        {
            InitializeComponent();
        }

        public LibraryPrivilegeOptionsControl(GrantRevokeControl grantRevokeTool)
        {
            InitializeComponent();
            _grantRevokeTool = grantRevokeTool;
            _allCheckBox.Checked = true;
        }

        private void _allCheckBox_CheckedChanged(object sender, System.EventArgs e)
        {
            if (_allCheckBox.Checked)
            {
                _usageCheckBox.Checked = _updateCheckBox.Checked = true;
                _objectLevelPrivilegesGroupBox.Enabled = false;
            }
            else
            {
                _objectLevelPrivilegesGroupBox.Enabled = true;
            }

        }

        private void _usageCheckBox_CheckedChanged(object sender, System.EventArgs e)
        {
            if (_usageCheckBox.Checked && _updateCheckBox.Checked)
            {
                _allCheckBox.Checked = true;
            }
            else
            {
                _allCheckBox.Checked = false;
            }
        }

        #region IPrivilegeOptionsProvider Members

        public void Reset()
        {
            Cursor = Cursors.WaitCursor;
            try
            {
                _allCheckBox.Checked = true;
                UpdateSelections();
            }
            finally
            {
                Cursor = Cursors.Default;
            }
        }

        public string GetSQLPrivilegeClause()
        {            
            List<string> selectedPrivileges = new List<string>();

            if (_allCheckBox.Checked)
            {
                selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALL));
            }
            else
            {
                if (_updateCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_UPDATE));
                }
                if (_usageCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_USAGE));
                }
            }

            return string.Join(", ", selectedPrivileges.ToArray());
        }

        public void UpdateSelections()
        {
                      
        }
        public string IsValid()
        {
            if (string.IsNullOrEmpty(GetSQLPrivilegeClause()))
                return Properties.Resources.NoObjectPrivilegesSelectedMessage;

            return "";
        }
        #endregion IPrivilegeOptionsProvider Members



       
    }
}
