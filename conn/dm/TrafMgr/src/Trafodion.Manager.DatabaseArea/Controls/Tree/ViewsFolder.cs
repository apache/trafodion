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
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for ViewsFolder.
    /// </summary>
    public class ViewsFolder : SchemaItemsFolder
    {
        /// <summary>
        /// Constructs a Views Folder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public ViewsFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.Views, aTrafodionSchema)
        {
        }

        /// <summary>
        /// Selects the Materialized Views  folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionView)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is ViewLeaf)
                    {
                        ViewLeaf theViewLeaf = theNode as ViewLeaf;
                        if (theViewLeaf.TrafodionView.InternalName.Equals(theTargetInternalName))
                        {
                            theViewLeaf.TreeView.SelectedNode = theViewLeaf;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        protected override bool IsPopulateRequired
        {
            get
            {
                return base.IsPopulateRequired;
            }
        }

        /// <summary>
        /// Resets the Views  list in the parent schema. The list will be populated when it is accessed next
        /// </summary>
        /// <param name="aNameFilter">A navigation tree filter</param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.TrafodionViews = null;
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionViews;
        }

        protected override void AddNodes()
        {
            foreach (TrafodionView theTrafodionView in TheTrafodionSchema.TrafodionViews)
            {
                ViewLeaf theViewItem = new ViewLeaf(theTrafodionView);
                Nodes.Add(theViewItem);
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionView theTrafodionView in TheTrafodionSchema.TrafodionViews)
            {
                if (nameFilter.Matches(theTrafodionView.ExternalName))
                {
                    ViewLeaf theViewItem = new ViewLeaf(theTrafodionView);
                    Nodes.Add(theViewItem);
                }
            }
        }

        /// <summary>
        /// Handles the View Validation events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        override public void TrafodionObject_ModelChangedEvent(object sender, TrafodionObject.TrafodionModelChangeEventArgs e)
        {
            if (e.EventId == TrafodionObject.ChangeEvent.ViewValidated)
            {
                DoRefresh(null);
                if (e.ChangeInitiator is TrafodionView)
                {
                    SelectTrafodionObject(e.ChangeInitiator);
                } 
            }
        }
    }
}
