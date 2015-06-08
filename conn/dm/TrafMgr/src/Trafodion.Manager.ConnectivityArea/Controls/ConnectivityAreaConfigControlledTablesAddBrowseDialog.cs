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
    public partial class ConnectivityAreaConfigControlledTablesAddBrowseDialog : TrafodionForm
    {

        // True if this is a new Controlled Tabled being defined, false if an existing 
        // one is being edited.
        private bool _isNewControlledTable;

        //The object being created.
        private object _NDCSControlledTable;

        public object NDCSControlledTable
        {
            get { return _NDCSControlledTable; }
            set { _NDCSControlledTable = value; }
        }

        public ConnectivityAreaConfigControlledTablesAddBrowseUserControl BrowseUserControl
        {
            get { return _theConnectivityAreaControlledTablesAddBrowseUserControl; }
            set { _theConnectivityAreaControlledTablesAddBrowseUserControl = value; }
        }

        /// <summary>
        /// The Constructor
        /// </summary>
        public ConnectivityAreaConfigControlledTablesAddBrowseDialog()
        {

            // Call the designer-generated code
            InitializeComponent();

            // Center the dialog on the parent
            StartPosition = FormStartPosition.CenterParent;

            // Default the result to Cancel
            DialogResult = DialogResult.Cancel;

            this._theConnectivityAreaControlledTablesAddBrowseUserControl.theDatabaseTreeView.AfterSelect += new TreeViewEventHandler(theDatabaseTreeView_AfterSelect);
        
        }

        void theDatabaseTreeView_AfterSelect(object sender, TreeViewEventArgs e)
        {

            this._theOKButton.Enabled = (this._theConnectivityAreaControlledTablesAddBrowseUserControl.theDatabaseTreeView.SelectedNode is DatabaseArea.Controls.Tree.TableFolder);
            /*
            if (null != this._theConnectivityAreaControlledTablesAddBrowseUserControl.theDatabaseTreeView.SelectedNode.Parent)
            {
                
                string parentNodeName = this._theConnectivityAreaControlledTablesAddBrowseUserControl.theDatabaseTreeView.SelectedNode.Parent.Text;
                this._theOKButton.Enabled = (parentNodeName.Equals("Tables")
                                            //|| parentNodeName.Equals("Views")
                                            //|| parentNodeName.Equals("Index")
                                            );
            }   
            else {
                this._theOKButton.Enabled = false;
            }
             * */
        }

        public void OpenDialog()
        {
            this._theConnectivityAreaControlledTablesAddBrowseUserControl.PopulateDataTreeView();
            this._theOKButton.Enabled = false;
            this.ShowDialog();
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
            string theError = null;// _theConnectivityAreaControlledTablesAddBrowseUserControl.ValidateInfo();

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
            if (_isNewControlledTable)
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
