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

using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for CatalogFolder.
    /// </summary>
    public class CatalogFolder : DatabaseTreeFolder, IHasTrafodionCatalog
    {
        private SchemasFolder _schemasFolder;


        /// <summary>
        /// Constructs a Catalog tree node for the Catalog object
        /// </summary>
        /// <param name="aTrafodionCatalog">The catalog object</param>
        public CatalogFolder(TrafodionCatalog aTrafodionCatalog)
            : base(aTrafodionCatalog)
        {
            ImageKey = DatabaseTreeView.DB_CATALOG_ICON;
            SelectedImageKey = DatabaseTreeView.DB_CATALOG_ICON;
        }

        /// <summary>
        /// Finds and select the specific schema in the schemas sub-folder
        /// </summary>
        /// <param name="aTrafodionObject">The object to select in the schemas folder</param>
        /// <returns>True or False</returns>
        public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            DoPopulate(null);
            return _schemasFolder.SelectTrafodionObject(aTrafodionObject);
        }
        /// <summary>
        /// Resets the schemas subfolder
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionCatalog.Refresh();
        }
        /// <summary>
        /// Populates the schemas subfolder with list of schemas
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
            _schemasFolder = new SchemasFolder(TheTrafodionCatalog);
            Nodes.Add(_schemasFolder);
        }
        /// <summary>
        /// The catalog object whose information is displayed in this node
        /// </summary>
        public TrafodionCatalog TheTrafodionCatalog
        {
            get { return (TrafodionCatalog)this.TrafodionObject; }
        }

        /// <summary>
        /// The schemas sub folder
        /// </summary>
        public SchemasFolder SchemasFolder
        {
            get { return _schemasFolder; }
        }

        /// <summary>
        /// Long description of the catalog
        /// </summary>
        override public string LongerDescription
        {
            get { return Properties.Resources.Catalog + " " + TheTrafodionCatalog.VisibleAnsiName; }
        }
    }
}
