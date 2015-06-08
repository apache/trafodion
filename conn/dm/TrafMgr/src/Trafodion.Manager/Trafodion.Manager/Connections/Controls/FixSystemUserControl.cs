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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Connections.Controls
{
    public partial class FixSystemUserControl : UserControl
    {
        private ConnectionDefinition _theConnectionDefinition = null;

        /// <summary>
        /// Default constructor
        /// </summary>
        public FixSystemUserControl()
        {
            InitializeComponent();

            // Now, start monitoring child's PasswordTextBox so that when the Enter key is pressed,
            // we'll treat it as the Apply/OK button is clicked. 
            _theConnectionDefinitionUserControl.PasswordTextBox.KeyPress += TheChildPasswordTextBox_KeyPress;
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;
        }

        public FixSystemUserControl(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            TheConnectionDefinition = aConnectionDefinition;
            SetConnectionPropertiesStatus(TheConnectionDefinition);          
            //_enableLiveFeedConnectButton = anEnableLiveFeedConnectButton;
            //_theLiveFeedOnlyConnectButton.Visible = _enableLiveFeedConnectButton;
            //_theConnectionDefinitionUserControl.LiveFeedConnectionProperties.Enabled = _enableLiveFeedConnectButton;
        }

        /// <summary>
        /// Property: TheConnectionDefinition
        /// </summary>
        public ConnectionDefinition TheConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set
            {
                _theConnectionDefinition = value;
                if (_theConnectionDefinition != null)
                {
                    _theConnectionDefinitionUserControl.EditFrom(_theConnectionDefinition);
                }
                SetConnectionPropertiesStatus(TheConnectionDefinition);      
            }
        }

        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if(_theConnectionDefinition != null)
            {
                if ((aReason == ConnectionDefinition.Reason.Password) &&
                    (_theConnectionDefinition.Name.Equals(aConnectionDefinition.Name, StringComparison.OrdinalIgnoreCase)))
                {
                    //If a connection definition has been modified, retry to reload the form
                    if (_theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded &&
                        _theConnectionDefinition.Equals(aConnectionDefinition))
                    {
                        _theConnectionDefinitionUserControl.EditFrom(aConnectionDefinition);
                        _theConnectionDefinition.Set(aConnectionDefinition);
                    }
                }
                //after connection definition changed,reset connection properties.
                SetConnectionPropertiesStatus(aConnectionDefinition);
            }
        }

        private void SetConnectionPropertiesStatus(ConnectionDefinition aConnectionDefinition) 
        {
            try
            {
                var groupBoxGeneral = _theConnectionDefinitionUserControl.CommonConnectionProperties;
                var groupBoxLiveFeed = _theConnectionDefinitionUserControl.LiveFeedConnectionProperties;
                var groupBoxODBC = _theConnectionDefinitionUserControl.ODBCConnectionProperties;

                if (aConnectionDefinition != null)
                {
                    if (aConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
                    {
                        groupBoxGeneral.Enabled = false;
                        groupBoxODBC.Enabled = true;
                        groupBoxLiveFeed.Enabled = false;
                    }
                    else if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                    {
                        groupBoxGeneral.Enabled = false;
                        groupBoxODBC.Enabled = false;
                        groupBoxLiveFeed.Enabled = true;
                    }
                    else
                    {
                        groupBoxGeneral.Enabled = true;
                        groupBoxODBC.Enabled = true;
                        groupBoxLiveFeed.Enabled = true;
                    }

                }
            }
            catch (InvalidOperationException)
            {           
            }
        }

        /// <summary>
        /// This methods asks the embedded ConnectionDefinitionUserControl to validate its
        /// contents and return and error message if appropriate.  If there is an error 
        /// messsage, it is shown to the user.
        /// </summary>
        /// <returns>true if OK else false if error detected</returns>
        private bool IsWorthy()
        {
            if (!Utilities.CheckOdbcDriverExists())
                return false;

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
        /// Called when the user clicks the Apply button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheApplyButtonClick(object sender, EventArgs e)
        {
            // Check to see if the user's work is good enough to accept
            if (!IsWorthy())
            {

                // There are problems and the user has been told.  Return without closing the dialog
                // so that the user can continue.
                return;
            }

            // Save the changes
            _theConnectionDefinitionUserControl.SaveTo(_theConnectionDefinition);

            // The user's work is good.  Test the results.  Events will only be fired for the
            // fields that the user changed.
            // If the test failed.  The user will have been told.
            // If the test succeeded, the system will be promoted
            //if (_theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
            //    DoLiveFeedTest();
            //if (_theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedAuthFailed)             
            //    _theConnectionDefinition.DoTest(true);
            _theConnectionDefinition.DoTest(true);
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
                TheApplyButtonClick(sender, e);
            }
        }

        /// <summary>
        /// Edit the connection definition without testing it
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theEditButton_Click(object sender, EventArgs e)
        {

            // Check to see if the user's work is good enough to accept
            if (!IsWorthy())
            {

                // There are problems and the user has been told.  Return without closing the dialog
                // so that the user can continue.
                return;

            }

            // Save the changes
            _theConnectionDefinitionUserControl.SaveTo(_theConnectionDefinition);
            if (_theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded && _theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
                _theConnectionDefinition.Password = ""; //Force an event so listeners
        }

        //private void DoLiveFeedTest() 
        //{
        //    _theConnectionDefinition.FireOnLiveFeedTest();

        //    // Check to see if the test failed.  The user will have been told.
        //    if (_theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded &&
        //        _theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
        //    {
        //        // not connected
        //        //Here we handle a special state: LiveFeedAuthFailed state, because when authentication failure which means name/password is not correct so as no need to do a ODBC connection later, 
        //        //so we immediately tell user to input the right user name and password and try again.
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
        //    }
        //}

        void MyDispose()
        {
            _theConnectionDefinitionUserControl.PasswordTextBox.KeyPress -= TheChildPasswordTextBox_KeyPress;
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }

        //private void _theLiveFeedOnlyConnectButton_Click(object sender, EventArgs e)
        //{
        //    // Check to see if the user's work is good enough to accept
        //    if (!IsWorthy())
        //    {

        //        // There are problems and the user has been told.  Return without closing the dialog
        //        // so that the user can continue.
        //        return;

        //    }

        //    // Save the changes
        //    _theConnectionDefinitionUserControl.SaveTo(_theConnectionDefinition);

        //    if (_theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded ||
        //        _theConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
        //    {
        //        return;
        //    }
        //    else
        //    {
        //        // Attempt the live feed connect.
        //        //_theConnectionDefinition.DoTest(true);
        //        _theConnectionDefinition.FireOnLiveFeedTest(true);

        //        // Check to see if the test failed.  The user will have been told.
        //        if (_theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded &&
        //            _theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
        //        {
        //            // not connected
        //            if (_theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedAuthFailed)
        //            {
        //                _theConnectionDefinition.FireOnLiveFeedTest(false);
        //            }

        //            if (_theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded &&
        //                _theConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
        //            {
        //                MessageBox.Show(Utilities.GetForegroundControl(), "Failed to connect to the Live Feed Server. Please provide a valid user id and password, or correct the Live Feed Port Number.",
        //                                Properties.Resources.Error, MessageBoxButtons.OK); 
        //                return;
        //            }
        //        }
        //    }
        //}
    }
}
