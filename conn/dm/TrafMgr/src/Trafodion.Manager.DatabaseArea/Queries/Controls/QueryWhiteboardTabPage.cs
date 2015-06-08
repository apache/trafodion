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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public class QueryWhiteboardTabPage : DelayedPopulateClonableTabPage, IMenuProvider
    {
        private DatabaseTreeView _theDatabaseTreeView = null;
        private FloatingQueryWhiteboardControl theDatabaseWhiteboardUserControl = null;

        public QueryWhiteboardTabPage()
            : this(null)
        {
        }

        public QueryWhiteboardTabPage(DatabaseTreeView aDatabaseTreeView)
            : base("SQL Whiteboard")
        {
            _theDatabaseTreeView = aDatabaseTreeView;
        }

        protected override void Populate()
        {
            Controls.Clear();
            //QueryWhiteboardUserControl theDatabaseWhiteboardUserControl = new QueryWhiteboardUserControl(_theDatabaseTreeView);
            theDatabaseWhiteboardUserControl = new FloatingQueryWhiteboardControl(_theDatabaseTreeView);
            theDatabaseWhiteboardUserControl.Dock = DockStyle.Fill;
            Controls.Add(theDatabaseWhiteboardUserControl);

            TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(this);
        }

        /// <summary>
        /// Implemeting the IMenuProvider interface
        /// </summary>
        /// <returns></returns>
        public Trafodion.Manager.Framework.Controls.TrafodionMenuStrip GetMenuItems(ImmutableMenuStripWrapper aMenuStripWrapper)
        {
            Trafodion.Manager.Framework.Controls.TrafodionMenuStrip menus = null;
            if (theDatabaseWhiteboardUserControl != null)
            {
                return theDatabaseWhiteboardUserControl.GetMenuItems(aMenuStripWrapper);
            }
            return menus;
        }

    }


}
