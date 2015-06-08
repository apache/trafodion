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

namespace Trafodion.Manager.Framework.Connections.Controls
{

    /// <summary>
    /// This is control that is intended to be shown in the right pane when the user
    /// selects an empty connections folder.  It is intended to guide the user toward
    /// adding a connection rather than just letting them guess when they start up and there's
    /// nothing there and nothing happening.
    /// <para/>
    /// It has an Add System button.
    /// </summary>
    public partial class NoSystemsUserControl : UserControl
    {

        // The constructor
        public NoSystemsUserControl()
        {

            // Call the designer-generated code
            InitializeComponent();

        }

        /// <summary>
        /// Lets the user add a system
        /// </summary>
        private void DoAddSystem()
        {

            // Create a connection definition dialog
            ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog();

            // Tell it that the user wants to add a system
            theConnectionDefinitionDialog.New();

        }

        /// <summary>
        /// Called when the user clicks the Add System button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheAddSystemButtonClick(object sender, EventArgs e)
        {

            // Do it.  We call a method rather than putting the code here so that we won't lose the
            // code if the button is deleted in the designer in the process of chnaging the GUI.
            DoAddSystem();

        }

    }
}
