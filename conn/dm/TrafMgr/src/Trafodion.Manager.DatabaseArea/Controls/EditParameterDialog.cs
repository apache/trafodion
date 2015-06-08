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
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Dialog that lets you edit a procedure parameter
    /// </summary>
    public partial class EditParameterDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Private member variables

        SPJParameterUserControl parameterControl = null;

        #endregion Private member variables

        /// <summary>
        /// Dialog that lets you edit a procedure parameter
        /// </summary>
        /// <param name="aTrafodionProcedureColumn"></param>
        public EditParameterDialog(TrafodionProcedureColumn aTrafodionProcedureColumn)
        {
            InitializeComponent();
            parameterControl = new SPJParameterUserControl(aTrafodionProcedureColumn, this);
            paramPanel.Controls.Add(parameterControl);
        }

        /// <summary>
        /// Enable or disable the create button based on validity of controls
        /// </summary>
        /// <param name="isDisableCreate"></param>
        public void disableCreate(bool isDisableCreate)
        {
            this._okButton.Enabled = !isDisableCreate;
        }

        /// <summary>
        /// When OK is clicked, the procedure column model is updated with the edited information.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _okButton_Click(object sender, System.EventArgs e)
        {
            parameterControl.UpdateModel();
        }
    }
}
