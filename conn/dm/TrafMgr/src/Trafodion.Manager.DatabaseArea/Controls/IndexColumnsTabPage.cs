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
    /// A Tab Page that displays information about an index's columns.
    /// </summary>
    public class IndexColumnsTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionIndex _sqlMxIndex;

        /// <summary>
        /// Create a Tab Page that displays information about an index's columns.
        /// </summary>
        /// <param name="aTrafodionIndex">The index whose columns are to be displayed.</param>
        public IndexColumnsTabPage(TrafodionIndex aTrafodionIndex)
            : base(Properties.Resources.Columns)
        {
            _sqlMxIndex = aTrafodionIndex;
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxIndex.TrafodionColumns;
        }
        /// <summary>
        /// Populate the tab page
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();
            // Create the panel and fill this tab page with it.
            IndexColumnsPanel theIndexColumnsPanel = new IndexColumnsPanel(_sqlMxIndex);
            theIndexColumnsPanel.Dock = DockStyle.Fill;
            Controls.Add(theIndexColumnsPanel);
        }
    }

}
