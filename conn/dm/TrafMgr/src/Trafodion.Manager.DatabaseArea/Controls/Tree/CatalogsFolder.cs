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
    /// Summary description for CatalogsFolder.
    /// </summary>
    public class CatalogsFolder : NavigationTreeConnectionFolder
    {
        /// <summary>
        /// Constructs a catalogs folder
        /// </summary>
        /// <param name="aTrafodionSystem">The system for which the catalog is being displayed</param>
        public CatalogsFolder(TrafodionSystem aTrafodionSystem)
            : base(aTrafodionSystem.ConnectionDefinition)
        {
            theTrafodionSystem = aTrafodionSystem;
            Text = ShortDescription;
        }
        /// <summary>
        /// Constructs a catalogs folder
        /// </summary>
        /// <param name="aTrafodionSystem">The system for which the catalog is being displayed</param>
        public CatalogsFolder(TrafodionSystem aTrafodionSystem, bool refreshCatalogs)
            : base(aTrafodionSystem.ConnectionDefinition)
        {
            theTrafodionSystem = aTrafodionSystem;
            Text = ShortDescription;
            if (refreshCatalogs)
            {
                //If the system is from the cache, clear the catalogs list, so it is refreshed
                theTrafodionSystem.TrafodionCatalogs = null;
            }
        }
        /// <summary>
        /// Selects the sql object in the tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        internal bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is IHasTrafodionCatalog)
            {
                TrafodionCatalog theTrafodionCatalog = ((IHasTrafodionCatalog)aTrafodionObject).TheTrafodionCatalog;
                string theTrafodionCatalogInternalName = theTrafodionCatalog.InternalName;

                DoPopulate(null);

                foreach (TreeNode theTreeNode in Nodes)
                {
                    if (theTreeNode is CatalogFolder)
                    {
                        CatalogFolder theCatalogFolder = theTreeNode as CatalogFolder;
                        if (theCatalogFolder.TheTrafodionCatalog.InternalName.Equals(theTrafodionCatalogInternalName))
                        {
                            if (aTrafodionObject is TrafodionCatalog)
                            {
                                theCatalogFolder.TreeView.SelectedNode = theCatalogFolder;
                                return true;
                            }
                            return theCatalogFolder.SelectTrafodionObject(aTrafodionObject);
                        }
                    }
                }

            }
            return false;
        }

        /// <summary>
        /// Refreshes the catalogs folder
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSystem.Refresh();
        }

        protected override void PrepareForPopulate()
        {
            if (theTrafodionSystem.ConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                object c = TheTrafodionSystem.TrafodionCatalogs; //force a fetch of the catalogs list
            }
        }

        /// <summary>
        /// Populates the catalogs folder
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();

            foreach (TrafodionCatalog theTrafodionCatalog in TheTrafodionSystem.TrafodionCatalogs)
            {
                CatalogFolder theCatalogFolder = new CatalogFolder(theTrafodionCatalog);
                Nodes.Add(theCatalogFolder);
            }

        }

        /// <summary>
        /// The System model for which this catalog is defined
        /// </summary>
        public TrafodionSystem TheTrafodionSystem
        {
            get
            {
                return theTrafodionSystem;
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
                    return TheConnectionDefinition.Name + " - All Catalogs";
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

        private TrafodionSystem theTrafodionSystem;

    }
}
