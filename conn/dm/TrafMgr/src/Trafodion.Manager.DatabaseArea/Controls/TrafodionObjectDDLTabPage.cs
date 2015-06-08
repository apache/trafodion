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
    /// A TabPage that displays DDL for a sql object
    /// </summary>
    public class TrafodionObjectDDLTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionObject _sqlMxObject;

        /// <summary>
        /// The sql object whose DDL is displayed in this tab page
        /// </summary>
        public TrafodionObject TrafodionObject
        {
            get { return _sqlMxObject; }
        }

        /// <summary>
        /// Create a TabPage that displays a sql object's DDL.
        /// </summary>
        /// <param name="aTrafodionObject">The sql object whose DLL is to be displayed.</param>
        public TrafodionObjectDDLTabPage(TrafodionObject aTrafodionObject)
            : base(Properties.Resources.DDL)
        {
            _sqlMxObject = aTrafodionObject;
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxObject.DDLText;
        }
        /// <summary>
        /// Constructs a DDL panel and adds it to the DDL tab page
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            TrafodionObjectDDLPanel sqlMxObjectDDLPanel = new TrafodionObjectDDLPanel(TrafodionObject);
            sqlMxObjectDDLPanel.Dock = DockStyle.Fill;
            Controls.Add(sqlMxObjectDDLPanel);
        }
    }

}
