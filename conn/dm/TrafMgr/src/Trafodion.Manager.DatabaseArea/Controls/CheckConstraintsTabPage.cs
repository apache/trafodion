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
    /// A tabpage that contains a panel that shows the Check Constraints info for a table object.
    /// </summary>
    class CheckConstraintsTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionTable _sqlMxTable;

        /// <summary>
        /// Create a tabpage that contains a panel that shows the Store Order Information for a TrafodionTable.
        /// </summary>
        /// <param name="aTrafodionTable">The TrafodionTable</param>
        public CheckConstraintsTabPage(TrafodionTable aTrafodionTable)
            : base(Properties.Resources.CheckConstraints)
        {
            _sqlMxTable = aTrafodionTable;
        }

        /// <summary>
        /// The TrafodionTable object property.  
        /// </summary>
        public TrafodionTable TrafodionTable
        {
            get { return _sqlMxTable; }
        }

        public override void PrepareForPopulate()
        {
            object constraints = _sqlMxTable.TheCheckConstraints;
        }
        /// <summary>
        /// Populate the pane with check constraints information from the table.  
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();
            CheckConstraintsPanel theCheckConstraintsPanel = new CheckConstraintsPanel(TrafodionTable);
            theCheckConstraintsPanel.Dock = DockStyle.Fill;
            Controls.Add(theCheckConstraintsPanel);
        }

    }

}
