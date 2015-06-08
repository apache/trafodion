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

using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.UserManagement.Controls
{
    /// <summary>
    /// Dialog for cascade drop
    /// </summary>
    public partial class DropDialog : TrafodionForm
    {
        public DropDialog(int dropCount)
        {
            InitializeComponent();

            SetWarningMessage(dropCount);
        }

        #region Property

        /// <summary>
        /// Indicate if the Drop mode is Cascade
        /// </summary>
        public bool IsCascade
        {
            get { return chkCascade.Checked; }
        }

        #endregion

        #region Private Method

        private void SetWarningMessage(int dropCount)
        {
            lblWarning.Text = string.Format(Properties.Resources.DropRoleMsg1, dropCount);
        }

        #endregion

        #region Event

        private void btnYes_Click(object sender, System.EventArgs e)
        {
            this.DialogResult = DialogResult.Yes;
            Close();
        }

        private void btnNo_Click(object sender, System.EventArgs e)
        {
            this.DialogResult = DialogResult.No;
            Close();
        }

        #endregion
    }
}
