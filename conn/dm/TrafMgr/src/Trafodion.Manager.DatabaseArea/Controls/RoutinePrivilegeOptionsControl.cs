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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class RoutinePrivilegeOptionsControl : UserControl, IPrivilegeOptionsProvider
    {
        #region private member variables

        GrantRevokeControl _grantRevokeTool;

        #endregion private member variables

        public RoutinePrivilegeOptionsControl()
        {
            InitializeComponent();
        }

        public RoutinePrivilegeOptionsControl(GrantRevokeControl grantRevokeTool)
        {
            InitializeComponent();
            _grantRevokeTool = grantRevokeTool;
        }

        #region IPrivilegeOptionsProvider Members

        public void Reset()
        {
            Cursor = Cursors.WaitCursor;
            try
            {
                _executeCheckBox.Checked = true;
                UpdateSelections();
            }
            finally
            {
                Cursor = Cursors.Default;
            }
        }

        public string GetSQLPrivilegeClause()
        {
            return _executeCheckBox.Checked ? TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_EXECUTE) : "";
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
