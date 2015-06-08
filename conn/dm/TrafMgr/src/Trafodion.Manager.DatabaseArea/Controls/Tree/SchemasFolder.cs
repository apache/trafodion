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
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for SchemasFolder.
    /// </summary>
    public class SchemasFolder : NavigationTreeConnectionFolder, IHasTrafodionCatalog
    {
        public SchemasFolder(TrafodionSystem aTrafodionSystem)
            : base(aTrafodionSystem.ConnectionDefinition)
        {
            theTrafodionCatalog = new TrafodionCatalog(aTrafodionSystem, "TRAFODION");
            Text = aTrafodionSystem.ConnectionDefinition.Name;
        }
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aTrafodionCatalog"></param>
        public SchemasFolder(TrafodionCatalog aTrafodionCatalog)
            : base(aTrafodionCatalog.TrafodionSystem.ConnectionDefinition)
        {
            theTrafodionCatalog = aTrafodionCatalog;
            Text = ShortDescription;
            ImageKey = DatabaseTreeView.FOLDER_CLOSED_ICON;
            SelectedImageKey = DatabaseTreeView.FOLDER_CLOSED_ICON;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is IHasTrafodionSchema)
            {
                TrafodionSchema theTrafodionSchema = ((IHasTrafodionSchema)aTrafodionObject).TheTrafodionSchema;
                string theTrafodionSchemaInternalName = theTrafodionSchema.InternalName;

                DoPopulate(null);

                foreach (TreeNode theTreeNode in Nodes)
                {
                    if (theTreeNode is SchemaFolder)
                    {
                        SchemaFolder theSchemaFolder = theTreeNode as SchemaFolder;
                        if (theSchemaFolder.TheTrafodionSchema.InternalName.Equals(theTrafodionSchemaInternalName))
                        {
                            if (aTrafodionObject is TrafodionSchema)
                            {
                                theSchemaFolder.TreeView.SelectedNode = theSchemaFolder;
                                return true;
                            }
                            return theSchemaFolder.SelectTrafodionObject(aTrafodionObject);
                        }
                    }
                }

            }
            return false;
        }

        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            if (TheTrafodionCatalog.ConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                TheTrafodionCatalog.TrafodionSchemas = null;
            }
        }

        protected override void PrepareForPopulate()
        {
            object s = TheTrafodionCatalog.TrafodionSchemas; //force the fetch of schema list
        }

        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();

            if (this.TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                foreach (TrafodionSchema theTrafodionSchema in TheTrafodionCatalog.TrafodionSchemas)
                {
                    if (!TrafodionName.IsAnInternalSchemaName(theTrafodionSchema.ExternalName))
                    {
                        SchemaFolder theSchemaFolder = new SchemaFolder(theTrafodionSchema);
                        Nodes.Add(theSchemaFolder);
                    }
                }
            }
        }

        /// <summary>
        /// Returns the Catalog object.
        /// </summary>
        public TrafodionCatalog TheTrafodionCatalog
        {
            get
            {
                return theTrafodionCatalog;
            }
        }

        /// <summary>
        /// Short description
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                if (TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                {
                    return "Schemas";
                }

                return TheConnectionDefinition.Name;
            }
        }

        /// <summary>
        /// Long description
        /// </summary>
        override public string LongerDescription
        {
            get
            {
                return ShortDescription;
            }
        }

        /// <summary>
        /// Add context munu according to logon roles.
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        //override public void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        //{
        //    NavigationTreeFolder theParentFolder = (this.Parent) as NavigationTreeFolder;
        //    theParentFolder.AddToContextMenu(aContextMenuStrip);
        //}

        private TrafodionCatalog theTrafodionCatalog;

    }
}
