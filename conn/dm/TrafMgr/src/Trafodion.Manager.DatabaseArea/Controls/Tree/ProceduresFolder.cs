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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for ProceduresFolder.
    /// </summary>
    public class ProceduresFolder : SchemaItemsFolder
    {

        /// <summary>
        /// Constructs a ProceduresFolder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public ProceduresFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.Procedures, aTrafodionSchema)
        {
        }

        /// <summary>
        /// Handles the Procedure created and dropped events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        override public void TrafodionObject_ModelChangedEvent(object sender, TrafodionObject.TrafodionModelChangeEventArgs e)
        {            
            if (e.EventId == TrafodionObject.ChangeEvent.ProcedureCreated)
            {
                DoRefresh(null);
                SelectTrafodionObject(e.ChangeInitiator);
            }
            else if (e.EventId == TrafodionObject.ChangeEvent.ProcedureDropped)
            {
                DoRefresh(null);
                // TO DO: select object?
            }
        }

        /// <summary>
        /// Selects the Procedures folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionProcedure)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is ProcedureLeaf)
                    {
                        ProcedureLeaf theProcedureLeaf = theNode as ProcedureLeaf;
                        if (theProcedureLeaf.TrafodionProcedure.InternalName.Equals(theTargetInternalName))
                        {
                            theProcedureLeaf.TreeView.SelectedNode = theProcedureLeaf;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Resets the procedures list in the parent schema. The list will be populated when it is accessed next
        /// </summary>
        /// <param name="aNameFilter">A navigation tree filter</param>
        /// 
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.TrafodionProcedures = null;
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionProcedures;
        }

        protected override void AddNodes()
        {
            foreach (TrafodionProcedure theTrafodionProcedure in TheTrafodionSchema.TrafodionProcedures)
            {
                ProcedureLeaf theProcedureItem = new ProcedureLeaf(theTrafodionProcedure);
                Nodes.Add(theProcedureItem);
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionProcedure theTrafodionProcedure in TheTrafodionSchema.TrafodionProcedures)
            {
                if (nameFilter.Matches(theTrafodionProcedure.ExternalName))
                {
                    ProcedureLeaf theProcedureItem = new ProcedureLeaf(theTrafodionProcedure);
                    Nodes.Add(theProcedureItem);
                }
            }
        }

        /// <summary>
        /// This method lets the TreeNodes to add context menu items that are specific to the node
        /// The Navigation tree calls this method and passes a context menu strip to which the menu items need to be added
        /// The base nodes implementation of this method needs to be called first to have the common menu items added
        /// </summary>
        /// <param name="aContextMenuStrip">The context menu strip to which the menu items have to be added</param>
        override public void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);

            SchemaFolder theSchemaFolder = (this.Parent) as SchemaFolder;
            //M7
            if (TheTrafodionSchema.ConnectionDefinition.ComponentPrivilegeExists("SQL_OPERATIONS", "CREATE_PROCEDURE"))
            {
                aContextMenuStrip.Items.Add(theSchemaFolder.GetCreateProcedureMenuItem(this));
            }
        }
    }
}
