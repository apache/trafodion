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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for TablesFolder.
    /// </summary>
    public class TablesFolder : SchemaItemsFolder
    {
        /// <summary>
        /// Constructs a TablesFolder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public TablesFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.Tables, aTrafodionSchema)
        {
        }

        /// <summary>
        /// Selects the Table folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if ((aTrafodionObject is TrafodionTable) || (aTrafodionObject is TrafodionIndex) || (aTrafodionObject is TrafodionTrigger))
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is TableFolder)
                    {
                        TableFolder theTableFolder = theNode as TableFolder;
                        if (aTrafodionObject is TrafodionTable)
                        {
                            if (theTableFolder.TrafodionTable.InternalName.Equals(theTargetInternalName))
                            {
                                theTableFolder.TreeView.SelectedNode = theTableFolder;
                                return true;
                            }
                        }
                        else if (aTrafodionObject is TrafodionIndex)
                        {
                            if (theTableFolder.SelectTrafodionObject(aTrafodionObject))
                            {
                                return true;
                            }
                        }
                        else if (aTrafodionObject is TrafodionTrigger)
                        {
                            if (theTableFolder.SelectTrafodionObject(aTrafodionObject))
                            {
                                return true;
                            }
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Resets the tables list in the parent schema. The list will be populated when it is accessed next
        /// </summary>
        /// <param name="aNameFilter">A navigation tree filter</param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.TrafodionTables = null;
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionTables;
        }

        protected override void AddNodes()
        {
            foreach (TrafodionTable theTrafodionTable in TheTrafodionSchema.TrafodionTables)
            {
                RespondUI(this);

                // Metadata tables like MVS_*, HISTOGRAM* not to be shown to Trafodion users
                if (!(TrafodionName.IsASystemTableName(theTrafodionTable.ExternalName)))
                {
                    TableFolder theTableFolder = new TableFolder(theTrafodionTable);
                    Nodes.Add(theTableFolder);
                }
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionTable theTrafodionTable in TheTrafodionSchema.TrafodionTables)
            {
                RespondUI(this);

                if (nameFilter.Matches(theTrafodionTable.ExternalName))
                {
                    TableFolder theTableFolder = new TableFolder(theTrafodionTable);
                    Nodes.Add(theTableFolder);
                }
            }
        }
    }
}
