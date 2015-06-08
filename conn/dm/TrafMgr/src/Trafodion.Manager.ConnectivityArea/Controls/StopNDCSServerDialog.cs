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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    public partial class StopNDCSServerDialog : TrafodionForm
    {
        #region Fields

        NDCSSystem theSystem;
        ConnectionDefinition aConnectionDefinition;
        private bool canClose = true;
        public int StopMode = -1;
        string theReason;
        #endregion Fields

        /// <summary>
        /// Get the stop reason
        /// </summary>
        public string TheReason
        {
            get
            {
                return theReason;
            }
        }

        
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="NdcsSystem"></param>
        public StopNDCSServerDialog(int theConnectedCount)
        {
            InitializeComponent();

            this._buttonStopImmediately.DialogResult = DialogResult.OK;
            this.AffectedConnections.Text = theConnectedCount.ToString();
            this._buttonStopImmediately.Enabled = true;
            this.FormClosing += new FormClosingEventHandler(StopDatasourceDialog_FormClosing);
            // Center the dialog on the parent
            StartPosition = FormStartPosition.CenterParent;

            // Default the result to Cancel
            DialogResult = DialogResult.Cancel;
        }

        /// <summary>
        /// Catch the form closing event so that the dialog remains open if exceptions are thrown
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void StopDatasourceDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (!canClose)
            {
                e.Cancel = true;
                canClose = true;
            }
        }

        /// <summary>
        /// Cancel event button handler to to close dialog
        /// </summary>
        private void _theCancelButton_Click(object sender, EventArgs e)
        {      
            // Allow dialog to be dismissed
            canClose = true;            
        }

        /// <summary>
        /// Ok event button handler to add the service
        /// </summary>
        private void _buttonStopImmediately_Click(object sender, EventArgs e)
        {
            StopMode = (int) NDCSObject.StopMode.STOP_IMMEDIATE;
            // Allow dialog to be dismissed
            canClose = true;            
        }


    }
}
