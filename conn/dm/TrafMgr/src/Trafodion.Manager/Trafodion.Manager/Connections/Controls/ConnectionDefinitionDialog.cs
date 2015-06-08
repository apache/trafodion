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
using System.Diagnostics;

namespace Trafodion.Manager.Framework.Connections.Controls
{

    /// <summary>
    /// This class provides a dialog wrapper for an instance of ConnectionDefinitionUserControl
    /// to let the user add, test, or edit a connection definition.
    /// </summary>
    public partial class ConnectionDefinitionDialog : TrafodionForm
    {

        // True if this is a new connection definition being defined, false if an existing 
        // one is being edited.
        private bool _isNewConnection;

        // The connection definition being worked on.
        private ConnectionDefinition _theConnectionDefinition = null;

        /// <summary>
        /// The Constructor
        /// </summary>
        public ConnectionDefinitionDialog()
        {
            Initialize(true);
        }

        /// <summary>
        /// The alternate constructor, which will not create a new connection.  Basically, this is
        /// for Edit / Connect dialog for modifying connection definition or connecting.
        /// </summary>
        /// <param name="aNewConnection"></param>
        public ConnectionDefinitionDialog(bool aNewConnection)
        {
            Initialize(aNewConnection);
        }

        /// <summary>
        /// The real constructor
        /// </summary>
        /// <param name="aNewConnection"></param>
        /// <returns></returns>
        private void Initialize(bool aNewConnection)
        {
            // Call the designer-generated code
            InitializeComponent();
            
            _isNewConnection = aNewConnection;

            if (!aNewConnection)
            {
                _theAddOnlyButton.Text = Properties.Resources.SaveSystemButtonText;
            }

            // Now, monitoring the Enter key for the child PasswordTextBox.  We want to 
            // simulate the same effect for Enter key as the OK button clicked.  
            _theConnectionDefinitionUserControl.PasswordTextBox.KeyPress += new KeyPressEventHandler(TheChildPasswordTextBox_KeyPress);

            

            // Center the dialog on the parent
            StartPosition = FormStartPosition.CenterParent;

            // Default the result to Cancel
            DialogResult = DialogResult.Cancel;
        
        }

        /// <summary>
        /// Call this to set the dialog up to edit an existing connection definition.
        /// <para/>
        /// This calls ShowDialog
        /// and when it returns, the operation is complete.  The call can inspect DialogResult if desired.
        /// </summary>
        /// <param name="aConnectionDefinition">the existing connection definition</param>
        public DialogResult Edit(ConnectionDefinition aConnectionDefinition)
        {
            if (!Utilities.CheckOdbcDriverExists())
                return DialogResult.Abort;

            // This is not a new connection definition
            _isNewConnection = false;

            // Save the existing connection definition
            _theConnectionDefinition = aConnectionDefinition;

            // Tell the embedded ConnectionDefinitionUserControl to edit the existing connection definition
            _theConnectionDefinitionUserControl.EditFrom(aConnectionDefinition);

            // Set our title to show what we're doing
            Text = Properties.Resources.ConnectAndEditSystem + " - " + aConnectionDefinition.Name;

            if (aConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
            {
                // no longer need the Live Feed Only Connect button
                //_theLiveFeedOnlyConnectButton.Enabled = false;
                //_theLiveFeedOnlyConnectButton.Visible = false;
                _theConnectionDefinitionUserControl.ODBCConnectionProperties.Enabled = true;
                _theConnectionDefinitionUserControl.CommonConnectionProperties.Enabled = false;
                _theConnectionDefinitionUserControl.LiveFeedConnectionProperties.Enabled = false;
            }
            else if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                // would not allow to change
                _theConnectionDefinitionUserControl.LiveFeedOnlyConnectButton.Enabled = false;
                _theOKButton.Visible = false;                
                _theConnectionDefinitionUserControl.ODBCConnectionProperties.Enabled = false;
                _theConnectionDefinitionUserControl.CommonConnectionProperties.Enabled = false;
                _theConnectionDefinitionUserControl.LiveFeedConnectionProperties.Enabled = true;
            }
            else
            {                
                //_theLiveFeedOnlyConnectButton.Visible = enableLiveFeedConnection;                
                _theOKButton.Visible = true;
                _theConnectionDefinitionUserControl.ODBCConnectionProperties.Enabled =true;
                _theConnectionDefinitionUserControl.CommonConnectionProperties.Enabled = true;
                _theConnectionDefinitionUserControl.LiveFeedConnectionProperties.Enabled = true;
            }

            // Show the dialog and complete the work
            return ShowDialog();

        }

        /// <summary>
        /// Call this to set the dialog up to add a new connection definition like an existing connection definition.
        /// <para/>
        /// This is like new except that the controls are preloaded
        /// from the settings in the existing connection definition.
        /// <para/>
        /// This calls ShowDialog
        /// and when it returns, the operation is complete.  The call can inspect DialogResult if desired.
        /// </summary>
        /// <param name="aConnectionDefinition">the existing connection definition</param>
        public DialogResult NewLike(ConnectionDefinition aConnectionDefinition)
        {
            if (!Utilities.CheckOdbcDriverExists())
                return DialogResult.Abort;

            // This is a new connection definition
            _isNewConnection = true;

            // Save the existing connection definition
            _theConnectionDefinition = new ConnectionDefinition();

            // Tell the embedded ConnectionDefinitionUserControl to make a copy of the existing connection definition
            _theConnectionDefinitionUserControl.CopyFrom(aConnectionDefinition);

            // Set our title to show what we're doing
            Text = "Add System Like - " + aConnectionDefinition.Name;

            // Show the dialog and complete the work
            return ShowDialog();

        }

        /// <summary>
        /// Call this to set the dialog up to add a new connection definition.
        /// <para/>
        /// This is like new like except that the controls are empty except for the port number which is
        /// set to its default value of 37800.
        /// <para/>
        /// This calls ShowDialog
        /// and when it returns, the operation is complete.  The call can inspect DialogResult if desired.
        /// </summary>
        public DialogResult New()
        {
            return New("Add System");
        }

        /// <summary>
        /// Call this to set the dialog up to add a new connection definition.
        /// This calls ShowDialog
        /// and when it returns, the operation is complete.  The call can inspect DialogResult if desired.
        /// </summary>
        /// <param name="aTitle">Dialog title</param>
        public DialogResult New(string aTitle)
        {
            if (!Utilities.CheckOdbcDriverExists())
                return DialogResult.Abort;

            // This is a new connection definition
            _isNewConnection = true;

            // Create an empty connection definition
            _theConnectionDefinition = new ConnectionDefinition();

            // Set our title to show what we're doing
            Text = aTitle;

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
            string theError = _theConnectionDefinitionUserControl.ValidateInfo();

            // Check to see if an error was returned
            if ((theError != null) && (theError.Length > 0))
            {

                // Show the error to the user
                MessageBox.Show(Utilities.GetForegroundControl(), theError, Properties.Resources.Error, MessageBoxButtons.OK,MessageBoxIcon.Error); 
                
                // Let the caller know that the user's work is not worthy
                return false;
            }
            
            // Let the caller know that the user's work is worthy
            return true;

        }

        /// <summary>
        /// Called when the user clicks the Test button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheTestButtonClick(object sender, EventArgs e)
        {

            // Check to see if the user's work is good enough to test
            if (IsWorthy())
            {

                // Create a connection definition that will not fire events as we load it
                ScratchConnectionDefinition theNoEventsConnectionDefinition = new ScratchConnectionDefinition();

                // Load it with the user's work
                _theConnectionDefinitionUserControl.SaveTo(theNoEventsConnectionDefinition);

                // Do the test and show the user the result
                theNoEventsConnectionDefinition.DoTestOnly();

            }
        }

        /// <summary>
        /// Called when the user clicks the Connect button.
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

            // The user's work is good.  Save the results.  Events will only be fired for the
            // fields that the user changed.
            _theConnectionDefinitionUserControl.SaveTo(_theConnectionDefinition);

            // Check to see if this is a new connection rather than just an edit.
            if (_isNewConnection)
            {
                // It is new, add it to the collection.
                ConnectionDefinition.Add(this, _theConnectionDefinition);
            }

            //Attempt the Live Feed connect;
            //if(_theConnectionDefinition.TheState!=ConnectionDefinition.State.LiveFeedTestSucceeded)
            //    DoLiveFeedTest();
            
            //if (_theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedAuthFailed) 
            //{
                // Attempt the ODBC connect.
                _theConnectionDefinition.DoTest(true);

                // Check to see if the test failed.  The user will have been told.
                if (_theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
                {
                    if (!_isNewConnection)
                    {
                        // The test falied.  Let the user continue working.
                        // If Live Feed Connected, close the ConnectionDefinitionDialog window
                        if (_theConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
                            Close();
                        return;
                    }
                }
            //}            

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

        /// <summary>
        /// Called when the child PasswordTextBox has a key press event.
        /// Specifically, this is to capture the Enter key.  When user pressed the Enter key, 
        /// which generates the same effect as the OK button clicked. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheChildPasswordTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (Char)Keys.Enter)
            {
                TheOKButtonClick(sender, e);
            }
        }

        /// <summary>
        /// Adds a connetion definition without connecting
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theAddOnlyButton_Click(object sender, EventArgs e)
        {
            // Check to see if the user's work is good enough to accept
            if (!IsWorthy())
            {

                // There are problems and the user has been told.  Return without closing the dialog
                // so that the user can continue.
                return;

            }

            // The user's work is good.  Save the results.  Events will only be fired for the
            // fields that the user changed. But, reset the password field.
            if (_theConnectionDefinition.TheState!=ConnectionDefinition.State.TestSucceeded && _theConnectionDefinition.TheState!=ConnectionDefinition.State.LiveFeedTestSucceeded)
            {
                _theConnectionDefinitionUserControl.PasswordTextBox.Text = "";
            }
            _theConnectionDefinitionUserControl.SaveTo(_theConnectionDefinition);


            // Check to see if this is a new connection rather than just an edit.
            if (_isNewConnection)
            {
                // It is new, add it to the collection.
                ConnectionDefinition.Add(this, _theConnectionDefinition);
            }
            else
            {
                //Force an event to be fired to notify listeners that the connection definition has been modified.
                //But for Connected State, don't do this
                if (_theConnectionDefinition.TheState!=ConnectionDefinition.State.TestSucceeded && _theConnectionDefinition.TheState!=ConnectionDefinition.State.LiveFeedTestSucceeded)
                    _theConnectionDefinition.Password = ""; 
            }

            // Let the caller know that there was no test failure or the user did not set a (possibly empty) password.
            DialogResult = DialogResult.OK;

            // Dismiss this dialog
            Close();

        }

        //private void DoLiveFeedTest() 
        //{
        //    _theConnectionDefinition.FireOnLiveFeedTest();

        //    // Check to see if the test failed.  The user will have been told.
        //    if (_theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded &&
        //        _theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
        //    {
        //        //Here we handle a special state: LiveFeedAuthFailed state, because when authentication failure which means name/password is not correct so as no need to do a ODBC connection later
        //        if (_theConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedAuthFailed)
        //        {
        //            MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.AuthenticationErrorMessage,
        //                Properties.Resources.AuthenticationError, MessageBoxButtons.OK);
        //        }
        //        else
        //        {
        //            MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.TestLiveFeedFailMessage,
        //                Properties.Resources.LiveFeedAuthenticationError, MessageBoxButtons.OK);
        //        }
        //        if (!_isNewConnection)
        //        {
        //            // The test falied.  Let the user continue working.
        //            return;
        //        }
        //    }
        //}

        //private void _theLiveFeedOnlyConnectButton_Click(object sender, EventArgs e)
        //{
        //    // Check to see if the user's work is good enough to accept
        //    if (!IsWorthy())
        //    {
        //        // There are problems and the user has been told.  Return without closing the dialog
        //        // so that the user can continue.
        //        return;
        //    }

        //    // The user's work is good.  Save the results.  Events will only be fired for the
        //    // fields that the user changed.
        //    _theConnectionDefinitionUserControl.SaveTo(_theConnectionDefinition);


        //    // Check to see if this is a new connection rather than just an edit.
        //    if (_isNewConnection)
        //    {
        //        // It is new, add it to the collection.
        //        ConnectionDefinition.Add(this, _theConnectionDefinition);
        //    }

        //    if (_theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded ||
        //        _theConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
        //    {
        //        // Let the caller know that there was no test failure or the user did not set a (possibly empty) password.
        //        DialogResult = DialogResult.OK;

        //        // Dismiss this dialog
        //        Close();
        //    }

        //    // Attempt the live feed connect.
        //    //_theConnectionDefinition.DoTest(true);
        //    _theConnectionDefinition.FireOnLiveFeedTest(true);

        //    // Check to see if the test failed.  The user will have been told.
        //    if (_theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded &&
        //        _theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
        //    {

        //        _theConnectionDefinition.FireOnLiveFeedTest(false);

        //        if (_theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded &&
        //            _theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
        //        {
        //            MessageBox.Show(Utilities.GetForegroundControl(), "Failed to connect to the Live Feed Server. Please provide a valid user id and password, or correct the Live Feed Port Number.", 
        //                Properties.Resources.Error, MessageBoxButtons.OK); 
        //            if (!_isNewConnection)
        //            {
        //                // The test falied.  Let the user continue working.
        //                return;
        //            }
        //        }
        //    }

        //    // Let the caller know that there was no test failure or the user did not set a (possibly empty) password.
        //    DialogResult = DialogResult.OK;

        //    // Dismiss this dialog
        //    Close();
        //}

    }
}
