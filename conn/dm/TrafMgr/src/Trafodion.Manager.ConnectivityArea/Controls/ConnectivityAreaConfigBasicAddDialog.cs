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
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;
namespace Trafodion.Manager.ConnectivityArea.Controls
{

    /// <summary>
    /// This class provides a dialog wrapper for an instance of ConnectionDefinitionUserControl
    /// to let the user add, test, or edit a connection definition.
    /// </summary>
    public partial class ConnectivityAreaConfigBasicAddDialog : TrafodionForm
    {

        // True if this is a new Controlled Tabled being defined, false if an existing 
        // one is being edited.
        private bool _isNewRow;
        private string _titleText = "Add";

        public ConnectivityAreaConfigBasicAddUserControl UserControl
        {
            get { return _theConnectivityAreaBasicAddUserControl; }
            set { _theConnectivityAreaBasicAddUserControl = value;
                  _theConnectivityAreaBasicAddUserControl.OKButton = this._theOKButton;

            }
        }


        /// <summary>
        /// The Constructor
        /// </summary>
        public ConnectivityAreaConfigBasicAddDialog()
        {
            _theConnectivityAreaBasicAddUserControl = new ConnectivityAreaConfigBasicAddUserControl();

            // Call the designer-generated code
            InitializeComponent();

            // Center the dialog on the parent
            StartPosition = FormStartPosition.CenterParent;

            // Default the result to Cancel
            DialogResult = DialogResult.Cancel;

            _theConnectivityAreaBasicAddUserControl.OKButton = this._theOKButton;
        }

        /// <summary>
        /// The Constructor
        /// </summary>
        public ConnectivityAreaConfigBasicAddDialog(string inTitleText, int aType)
        {
            _theConnectivityAreaBasicAddUserControl = new ConnectivityAreaConfigBasicAddUserControl(aType);

            // Call the designer-generated code
            InitializeComponent();

            // Center the dialog on the parent
            StartPosition = FormStartPosition.CenterParent;

            // Default the result to Cancel
            DialogResult = DialogResult.Cancel;

            // Text to be used as the title of the form
            this._titleText = inTitleText;

            _theConnectivityAreaBasicAddUserControl.OKButton = this._theOKButton;

        }

        /// <summary>
        /// Call this to set the dialog up to edit an existing Controlled Table.
        /// <para/>
        /// This calls ShowDialog
        /// and when it returns, the operation is complete.  The call can inspect DialogResult if desired.
        /// </summary>
        /// <param name="aConnectionDefinition">the existing connection definition</param>
        public DialogResult Edit(object objectToEdit)
        {
            // This is not a new connection definition
            _isNewRow = false;

            // Tell the embedded ConnectionDefinitionUserControl to edit the existing connection definition
            _theConnectivityAreaBasicAddUserControl.EditFrom(objectToEdit);

            // Set our title to show what we're doing
            Text = _titleText; //+ objectToEdit.Name;

            // Show the dialog and complete the work
            return ShowDialog();
        }


        /// <summary>
        /// Call this to set the dialog up to add a new controlled table entry.
         /// This calls ShowDialog
        /// and when it returns, the operation is complete.  The call can inspect DialogResult if desired.
        /// </summary>
        public DialogResult New()
        {
            



            // This is a new controlled table
            _isNewRow = true;

            // Create an empty controlled table
            //_theControlledTable = new Model.ControlledTable();

            // Set our title to show what we're doing
            Text = _titleText;

            // Show the dialog and complete the work
            return ShowDialog();

        }

        /// <summary>
        /// This methods asks the embedded ConnectionDefinitionUserControl to validate its
        /// contents and return and error message if appropriate.  If there is an error 
        /// messsage, it is shown to the user.
        /// </summary>
        /// <returns>true if OK else false if error detected</returns>
        private bool IsWorthy()
        {

            // Ask the embedded ConnectionDefinitionUserControl to validate its contents
            string theError = _theConnectivityAreaBasicAddUserControl.ValidateInfo();

            // Check to see if an error was returned
            if ((theError != null) && (theError.Length > 0))
            {

                // Show the error to the user
                MessageBox.Show(Utilities.GetForegroundControl(), theError, Properties.Resources.Error, MessageBoxButtons.OK); // TODO - Should not use naked MessageBox ...has no app name
                
                // Let the caller know that the user's work is not worthy
                return false;
            }
            
            // Let the caller know that the user's work is worthy
            return true;

        }

        /// <summary>
        /// Called when the user clicks the OK button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheOKButtonClick(object sender, EventArgs e)
        {

            // Check to see if the user's work is good enough to accept
            if (!IsWorthy())
            {

                // There are problems and the user has been told.  Return without closing the dialog
                // so that the user can continue.
                return;

            }

            // The user's work is good.  Save the results in the table. 
            //Add To Table or return the new table object

            // Check to see if this is a new connection rather than just an edit.
            if (_isNewRow)
            {

                // It is new, add it to the table.               

            } 

            // Let the caller know that there was no test failure or the user did not set a (possibly empty) password.
            DialogResult = DialogResult.OK;

            // Dismiss this dialog
            Close();

        }

        /// <summary>
        /// Called when the user clicks the cancel button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheCancelButtonClick(object sender, EventArgs e)
        {

            // Dismiss this dialog
            Close();

        }
    }
}
