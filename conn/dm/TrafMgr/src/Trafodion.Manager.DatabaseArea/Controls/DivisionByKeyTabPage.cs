//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
    /// A tabpage that contains a panel that shows the Division By info for a table/MV/index.
    /// </summary>
    class DivisionByKeyTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionSchemaObject _sqlMxSchemaObject;

        /// <summary>
        /// The TrafodionSchemaObject object property.  
        /// </summary>
        public TrafodionSchemaObject TheTrafodionSchemaObject
        {
            get { return _sqlMxSchemaObject; }
        }

        /// <summary>
        /// Create a tabpage that contains a panel that shows the DivisionByKey Information for a table/MV/index.
        /// </summary>
        /// <param name="aTrafodionSchemaObject">The table/MV/index</param>
        public DivisionByKeyTabPage(TrafodionSchemaObject aTrafodionSchemaObject)
            : base(Properties.Resources.DivisionByKey)
        {
            _sqlMxSchemaObject = aTrafodionSchemaObject;
        }
        public override void PrepareForPopulate()
        {
            object divisionkeys = _sqlMxSchemaObject.TheTrafodionDivisionByColumnDefs;
        }
        /// <summary>
        /// Populate the pane with information from the table/MV/index.  
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();
            DivisionByKeyPanel theDivisionByKeyPanel = new DivisionByKeyPanel(TheTrafodionSchemaObject);
            theDivisionByKeyPanel.Dock = DockStyle.Fill;
            Controls.Add(theDivisionByKeyPanel);
        }
    }
}
