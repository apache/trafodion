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
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Connections.Controls
{

    /// <summary>
    /// A dialog that lets the user set passwords for a list of connection definitions
    /// </summary>
    public partial class FixConnectionsDialog : TrafodionForm
    {

        /// <summary>
        /// Forms designer expects an empty constructor
        /// </summary>
        public FixConnectionsDialog()
        {
            // Call designer generated code
            InitializeComponent();
        }

        /// <summary>
        /// The real constructor used to fixup a connection definition
        /// </summary>
        /// <param name="aConnectionDefinition">Connection definition</param>
        public FixConnectionsDialog(ConnectionDefinition aConnectionDefinition)
        {
            List<ConnectionDefinition> connectionDefinitions
                                    = new List<ConnectionDefinition>();
            connectionDefinitions.Add(aConnectionDefinition);
            DoFixup(connectionDefinitions);

        }
        /// <summary>
        /// The real constructor used to fixup a list of connection definitions
        /// </summary>
        /// <param name="aConnectionDefinitions">A list of connection definitions</param>
        public FixConnectionsDialog(List<ConnectionDefinition> aConnectionDefinitions)
        {
            DoFixup(aConnectionDefinitions);
        }

        public void DoFixup(List<ConnectionDefinition> aConnectionDefinitions)
        {
            // Check to see if there are any connections
            if (aConnectionDefinitions.Count == 0)
            {

                // No systems, nothing to do
                return;

            }

            // Call designer generated code
            InitializeComponent();

            // Tab order index for first password panel
            int theTabIndex = 500;

            // Create a password panel for every connection
            foreach (ConnectionDefinition theConnectionDefinition in aConnectionDefinitions)
            {

                // Create a password panel and add it to the list
                EditPasswordPanel theEnterPasswordPanel = new EditPasswordPanel(theConnectionDefinition, true);
                theEnterPasswordPanels.Add(theEnterPasswordPanel);

                // Dock each to the bottom to preserve order
                theEnterPasswordPanel.Dock = DockStyle.Bottom;
                theLoadedSystemsGroupBox.Controls.Add(theEnterPasswordPanel);

                // Assign a tab order
                theEnterPasswordPanel.TabIndex = theTabIndex++;

            }

        }

        /// <summary>
        /// All of the password panels
        /// </summary>
        List<EditPasswordPanel> theEnterPasswordPanels = new List<EditPasswordPanel>();

        /// <summary>
        /// Call when the OK button has been clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theOKButton_Click(object sender, EventArgs e)
        {

            // Look through all of the password panels to determine which the user changed
            foreach (EditPasswordPanel theEnterPasswordPanel in theEnterPasswordPanels)
            {

                // Check to see if the user changed this one
                if (theEnterPasswordPanel.Changed)
                {

                    // It is changed, get its connection definition
                    ConnectionDefinition theConnectionDefinition = theEnterPasswordPanel.ConnectionDefinition;

                    // Update the password
                    theConnectionDefinition.Password = theEnterPasswordPanel.Password;

                    // Test it and don't say anything if it succeeds
                    theConnectionDefinition.DoTest(true);

                }
            }

            // Close the form
            Close();

        }

        /// <summary>
        /// Called when the form is activated
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void FixupConnectionsDialog_Activated(object sender, EventArgs e)
        {

            // Set focus to the first password panel
            theEnterPasswordPanels[0].Focus();

        }
    }
}
