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
    public partial class StopNDCSServiceDialog : TrafodionForm
    {
        #region Fields

        private bool canClose = true;

        /// <summary>
        /// keep tracking the stop mode
        /// </summary>
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
        public StopNDCSServiceDialog(string Title, int theConnectedCount)
        {
            InitializeComponent();

            this._buttonStopImmediately.DialogResult = DialogResult.OK;
            this._buttonOnClientDisconnect.DialogResult = DialogResult.OK;

            // only use the datasource name if only one selected
            if (Title.Length > 0)
            {
                this.Text = this.Text + Title;
            }

            this.AffectedConnections.Text = theConnectedCount.ToString();
            this._buttonOnClientDisconnect.Enabled = false;
            this._buttonStopImmediately.Enabled = false;
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
            theReason = this._textBoxStopReason.Text.Trim();

            // Allow dialog to be dismissed
            canClose = true;            
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _buttonOnClientDisconnect_Click(object sender, EventArgs e)
        {
            StopMode = (int) NDCSObject.StopMode.STOP_DISCONNECT;
            theReason = this._textBoxStopReason.Text.Trim();


            // Allow dialog to be dismissed
            canClose = true;
        }

        /// <summary>
        /// Called by the services control to let this Dialog know if the fields are valid and
        /// ok button can be enabled.
        /// </summary>
        /// <param name="inputValid"></param>
        public void userInputValid(bool inputValid)
        {
            if (inputValid)
            {
                this._buttonStopImmediately.Enabled = true;
                this._buttonOnClientDisconnect.Enabled = true;
            }
            else
            {
                this._buttonStopImmediately.Enabled = false;
                this._buttonOnClientDisconnect.Enabled = false;
            }

        }

        private void validate_Reason()
        {

            if (this._textBoxStopReason.Text.Trim() == "")
            {
                userInputValid(false);
            }
            else
            {
                userInputValid(true);
            }

        }



        private void _textBoxStopReason_TextChanged(object sender, EventArgs e)
        {
            validate_Reason();
        }


    }
}
