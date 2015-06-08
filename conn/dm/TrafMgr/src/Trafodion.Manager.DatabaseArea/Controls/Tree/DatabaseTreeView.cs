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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// The database navigation tree.
    /// </summary>
    public class DatabaseTreeView : Trafodion.Manager.Framework.Navigation.NavigationTreeView
    {
        public const string DB_CATALOG_ICON = "DB_CATALOG_ICON";
        public const string DB_SCHEMA_ICON = "DB_SCHEMA_ICON";
        public const string DB_TABLE_ICON = "DB_TABLE_ICON";
        public const string DB_VIEW_ICON = "DB_VIEW_ICON";
        public const string DB_INDEX_ICON = "DB_INDEX_ICON";
        public const string DB_TRIGGER_ICON = "DB_TRIGGER_ICON";
        public const string DB_SPJ_ICON = "DB_SPJ_ICON";
        public const string DB_LIBRARY_ICON = "DB_LIBRARY_ICON";
        public const string DB_CLASS_ICON = "DB_CLASS_ICON";
        public const string DB_SYNONYM_ICON = "DB_SYNONYM_ICON";

        public DatabaseTreeView()
            :base()
        {
            InitializeComponent();
            this.theImageList.Images.Add(DB_CATALOG_ICON, global::Trafodion.Manager.Properties.Resources.DBCatalogIcon);
            this.theImageList.Images.Add(DB_SCHEMA_ICON, global::Trafodion.Manager.Properties.Resources.DBSchemaIcon);
            this.theImageList.Images.Add(DB_TABLE_ICON, global::Trafodion.Manager.Properties.Resources.DBTableIcon);
            this.theImageList.Images.Add(DB_VIEW_ICON, global::Trafodion.Manager.Properties.Resources.DBViewIcon);
            this.theImageList.Images.Add(DB_INDEX_ICON, global::Trafodion.Manager.Properties.Resources.DBIndexIcon);
            this.theImageList.Images.Add(DB_TRIGGER_ICON, global::Trafodion.Manager.Properties.Resources.DBTriggerIcon);
            this.theImageList.Images.Add(DB_SPJ_ICON, global::Trafodion.Manager.Properties.Resources.DBSPJIcon);
            this.theImageList.Images.Add(DB_LIBRARY_ICON, global::Trafodion.Manager.Properties.Resources.LibraryFolderIcon);
            this.theImageList.Images.Add(DB_CLASS_ICON, global::Trafodion.Manager.Properties.Resources.ClassIcon);
            this.theImageList.Images.Add(DB_SYNONYM_ICON, global::Trafodion.Manager.Properties.Resources.DBSynonymIcon);

        }

        bool _isListeningForDBOptionEvent = false;

        /// <summary>
        /// Register to listener to database area option change event
        /// </summary>
        public void CheckAndAddListener()
        {
            //Add the listener, only if you are not already listening
            if (!_isListeningForDBOptionEvent)
            {
                DatabaseAreaOptions options = DatabaseAreaOptions.GetOptions();
                if (options != null)
                {
                    options.DatabaseOptionsChanged += options_DatabaseOptionsChanged;
                    _isListeningForDBOptionEvent = true;
                }
            }
        }

        protected override void Dispose(bool disposing)
        {
            //remove listener if it exists
            if (_isListeningForDBOptionEvent)
            {
                DatabaseAreaOptions options = DatabaseAreaOptions.GetOptions();
                if (options != null)
                {
                    options.DatabaseOptionsChanged -= options_DatabaseOptionsChanged;
                    _isListeningForDBOptionEvent = false;
                }
            }
            base.Dispose(disposing);
        }

        /// <summary>
        /// Refresh the tree if the database area options has changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void options_DatabaseOptionsChanged(object sender, EventArgs e)
        {
            //Remember the current node and its parent
            string currentNodePath = "";
            string parentPath = "";
            if (SelectedNode != null)
            {
                currentNodePath = SelectedNode.FullPath;
                if (SelectedNode.Parent != null)
                {
                    parentPath = SelectedNode.Parent.FullPath;
                }
            }
            
            this.CollapseAll();

            if (!string.IsNullOrEmpty(currentNodePath))
            {
                try
                {
                    TreeNode node = FindByFullPath(currentNodePath);
                    Select(node);
                }
                catch (FindByFullPathFailed fe)
                {
                    //node might have been removed. select the parent.
                    if (!string.IsNullOrEmpty(parentPath))
                    {
                        try
                        {
                            TreeNode node = FindByFullPath(parentPath);
                            Select(node);
                        }
                        catch (FindByFullPathFailed fe1)
                        {
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Selects the tree node that contains the sql object
        /// </summary>
        /// <param name="aTrafodionObject">The sql object to select</param>
        /// <returns>True if selection was successful, else returns false</returns>
        public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            NavigationTreeConnectionFolder theNavigationTreeConnectionFolder = ExpandConnection(aTrafodionObject.ConnectionDefinition);
            if (theNavigationTreeConnectionFolder is CatalogsFolder)
            {
                CatalogsFolder theCatalogsFolder = theNavigationTreeConnectionFolder as CatalogsFolder;
                return theCatalogsFolder.SelectTrafodionObject(aTrafodionObject);
            }
            else if (theNavigationTreeConnectionFolder is SchemasFolder)
            {
                SchemasFolder theSchemasFolder = theNavigationTreeConnectionFolder as SchemasFolder;
                return theSchemasFolder.SelectTrafodionObject(aTrafodionObject);
            }
            return false;
        }

        /// <summary>
        /// Expands the connection folder and returns the connection folder
        /// </summary>
        /// <param name="aConnectionDefinition">Connection Definition</param>
        /// <returns>Connection folder</returns>
        public virtual NavigationTreeConnectionFolder ExpandConnection(ConnectionDefinition aConnectionDefinition)
        {
            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is NavigationTreeConnectionsFolder)
                {
                    NavigationTreeConnectionsFolder theNavigationTreeConnectionsFolder = theTreeNode as NavigationTreeConnectionsFolder;
                    NavigationTreeConnectionFolder theNavigationTreeConnectionFolder = theNavigationTreeConnectionsFolder.FindConnectionFolder(aConnectionDefinition);
                    if (theNavigationTreeConnectionFolder != null)
                    {

                        // Make sure that the connections folder has been populated by selecting
                        // the connections folder if it isn't.  This reduces flashing in the right
                        // pane when clikcing on a SQL object hyperlink in the right pane.
                        if (!theNavigationTreeConnectionFolder.IsExpanded)
                        {
                            Select(theNavigationTreeConnectionFolder);
                        }

                        return theNavigationTreeConnectionFolder;
                    }
                }
            }
            throw new Exception("Not found in systems: " + aConnectionDefinition.Name);
        }

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DatabaseTreeView));
            this.SuspendLayout();
            this.ResumeLayout(false);

        }
    }
}
