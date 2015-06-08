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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A TabPage that displays a primary key's attributes and columns.
    /// </summary>
    public class PrimaryKeyTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionTable _sqlMxTable;
        /// <summary>
        /// Create a TabPage that displays a primary key's attributes and columns.
        /// </summary>
        /// <param name="aTrafodionPrimaryKey">The primary key whose attributes and columns are to be displayed.  Null means
        /// that there is no primary key.</param>
        public PrimaryKeyTabPage(TrafodionTable aTrafodionTable)
            : base("Primary Key")
        {
            _sqlMxTable = aTrafodionTable;
        }

        public override void PrepareForPopulate()
        {
            _sqlMxTable.LoadPrimaryKeyColumns();
        }

        /// <summary>
        /// Populate the tab page
        /// </summary>
        protected override void Populate()
        {
            Controls.Clear();

            // We will choose a control for this tab page based on whether or not there is a primary key
            Control theControl;

            // Check to see if there is a primary key
            if (_sqlMxTable.TheTrafodionPrimaryKey == null)
            {

                // There is not.  We will center a message saying so.
                Label theLabel = new Label();
                theLabel.Text = "There is no primary key.";
                theLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
                theControl = theLabel;

            }
            else
            {

                // There is a primary key, create the display for it.
                theControl = new PrimaryKeyPanel(_sqlMxTable.TheTrafodionPrimaryKey);
            }

            // Make the control fill this tab page
            theControl.Dock = DockStyle.Fill;
            Controls.Add(theControl);
        }

    }

}
