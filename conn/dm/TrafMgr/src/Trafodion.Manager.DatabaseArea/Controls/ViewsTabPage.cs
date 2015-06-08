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

namespace Trafodion.Manager.DatabaseArea.Controls
{

    public class ViewsTabPage : SchemaLevelObjectListTabPage
    {

        /// <summary>
        /// Constructs a Views  tab page and passes the parameters to the base class for storage
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="aTabName"></param>
        /// 
        public ViewsTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema aTrafodionSchema, string aTabName)
            : base(aDatabaseObjectsControl, aTrafodionSchema, aTabName)
        {
        }

        public override void PrepareForPopulate()
        {
            object a = TheTrafodionSchema.TrafodionViews;
        }
        /// <summary>
        /// Fills the page with the list of  Views 
        /// A panel with the datagridview of the views is added to the tab page using e generic helper class
        /// </summary>
        protected override void Populate()
        {
            Controls.Clear();
            TrafodionViewListPanel theViewsListPanel = new TrafodionViewListPanel(TheDatabaseObjectsControl, HeaderText, TheTrafodionSchema, TheTrafodionSchema.TrafodionViews, TabName);
            theViewsListPanel.Dock = DockStyle.Fill;
            Controls.Add(theViewsListPanel);
        }
    }

}
