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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Base class for Database Area navigation tree folders
    /// This class overrides the framework and provides custom context menu capabilities
    /// </summary>
    public class DatabaseTreeFolder : NavigationTreeFolder
    {
        private TrafodionObject _sqlMxObject = null;
        private System.EventHandler<TrafodionObject.TrafodionModelChangeEventArgs> _modelChangedHandler = null;
        private System.EventHandler _modelRemovedHandler = null;
        private System.EventHandler<TrafodionObject.TrafodionModelEventArgs> _modelReplacedHandler = null;

        /// <summary>
        /// The Sql Object represented by this folder node
        /// </summary>
        public TrafodionObject TrafodionObject
        {
            get { return _sqlMxObject; }
            set { _sqlMxObject = value; }
        }

        /// <summary>
        /// Constructs the tree folder node for hold the sql object.
        /// </summary>
        /// <param name="aTrafodionObject">The sql object</param>
        public DatabaseTreeFolder(TrafodionObject aTrafodionObject)
        {
            TrafodionObject = aTrafodionObject;
            Text = aTrafodionObject.ExternalName;

            _modelChangedHandler = new System.EventHandler<TrafodionObject.TrafodionModelChangeEventArgs>(TrafodionObject_ModelChangedEvent);
            _modelRemovedHandler = new System.EventHandler(TrafodionObject_ModelRemovedEvent);
            _modelReplacedHandler = new System.EventHandler<TrafodionObject.TrafodionModelEventArgs>(TrafodionObject_ModelReplacedEvent);

            //Register the event handlers for the sql object model
            AddHandlers(TrafodionObject);
        }

        /// <summary>
        /// Constructs the tree folder node for hold the sql object.
        /// If this is a subfolder, then the handlers are not registered for the associated sql object
        /// since the parent folder already registered the handlers
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <param name="isSubFolder">True or False</param>
        public DatabaseTreeFolder(TrafodionObject aTrafodionObject, Boolean isSubFolder)
        {
            ImageKey = DatabaseTreeView.FOLDER_CLOSED_ICON;
            SelectedImageKey = DatabaseTreeView.FOLDER_CLOSED_ICON;

            TrafodionObject = aTrafodionObject;
            Text = aTrafodionObject.ExternalName;

            {
                _modelChangedHandler = new System.EventHandler<TrafodionObject.TrafodionModelChangeEventArgs>(TrafodionObject_ModelChangedEvent);
                _modelRemovedHandler = new System.EventHandler(TrafodionObject_ModelRemovedEvent);
                _modelReplacedHandler = new System.EventHandler<TrafodionObject.TrafodionModelEventArgs>(TrafodionObject_ModelReplacedEvent);
                AddHandlers(TrafodionObject);
            }
        }

        /// <summary>
        /// Register to listen to the sql object's model events
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        private void AddHandlers(TrafodionObject aTrafodionObject)
        {
            aTrafodionObject.ModelChangedEvent += _modelChangedHandler;
            aTrafodionObject.ModelRemovedEvent += _modelRemovedHandler;
            aTrafodionObject.ModelReplacedEvent += _modelReplacedHandler;
        }

        /// <summary>
        /// Unregister listeners to the sql object's model
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        private void RemoveHandlers(TrafodionObject aTrafodionObject)
        {
            aTrafodionObject.ModelChangedEvent -= _modelChangedHandler;
            aTrafodionObject.ModelRemovedEvent -= _modelRemovedHandler;
            aTrafodionObject.ModelReplacedEvent -= _modelReplacedHandler;
        }

        /// <summary>
        /// Perform cleanup
        /// </summary>
        /// <param name="disposing"></param>
        public override void Dispose(bool disposing)
        {
            if (_sqlMxObject != null)
            {
                RemoveHandlers(_sqlMxObject);
            }
            base.Dispose(disposing);
        }

        /// <summary>
        /// Refreshes the state of the node
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {

        }
        /// <summary>
        /// Handle the sql model changed event
        /// </summary>
        /// <param name="sender">The model of the sql object contained in the tree node</param>
        /// <param name="e"></param>
        public virtual void TrafodionObject_ModelChangedEvent(object sender, TrafodionObject.TrafodionModelChangeEventArgs e)
        {

        }

        /// <summary>
        /// Handle the sql model removed event
        /// </summary>
        /// <param name="sender">The model of the sql object contained in the tree node</param>
        /// <param name="e"></param>
        public virtual void TrafodionObject_ModelRemovedEvent(object sender, System.EventArgs e)
        {
            //Remove the model event handlers from the old sql model
            RemoveHandlers((TrafodionObject)sender);
            this.TrafodionObject = null;

            //In some random scenarios, the treeview is null, so check for not null
            if (TreeView != null && TreeView.SelectedNode == this)
            {
                System.Windows.Forms.MessageBox.Show(
                String.Format(Properties.Resources.ObjectNotAvailable, ((TrafodionObject)sender).ExternalName),
                Properties.Resources.Warning, System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning
                );
            }

            this.Remove();
        }

        /// <summary>
        /// Handle the sql model replaced event
        /// </summary>
        /// <param name="sender">The model of the sql object contained in the tree node</param>
        /// <param name="e">Contains the new model that replaces the old model</param>
        public virtual void TrafodionObject_ModelReplacedEvent(object sender, TrafodionObject.TrafodionModelEventArgs e)
        {
            //Remove the model event handlers from the old sql model
            RemoveHandlers((TrafodionObject)sender);

            //Set the new sql model and register handlers to listen to the model events
            this.TrafodionObject = e.TrafodionObject;
            AddHandlers(TrafodionObject);

            if (TreeView != null && TreeView.SelectedNode == this)
            {
                System.Windows.Forms.MessageBox.Show(
                    String.Format(Properties.Resources.ObjectRecreated, ((TrafodionObject)sender).ExternalName),
                    Properties.Resources.Warning, System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning
                    );
            }
        }


        protected override void PrepareForPopulate()
        {
            
        }

        /// <summary>
        /// Short description for this node
        /// </summary>
        override public string ShortDescription
        {
            get { return TrafodionObject.ExternalName; }
        }

        /// <summary>
        /// Long description for this node
        /// </summary>
        override public string LongerDescription
        {
            get { return TrafodionObject.VisibleAnsiName; }
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

            //If the sql object support DDL, display the ShowDDL menu
            if (TrafodionObject.AllowShowDDL)
                aContextMenuStrip.Items.Add(GetShowDDLMenuItem(this));

            aContextMenuStrip.Items.Add(GetGrantRevokeMenuItem(this));
        }

        /// <summary>
        /// Static method to create a ShowDDL context menu item
        /// </summary>
        /// <returns>The context menu item</returns>
        public static ToolStripMenuItem GetShowDDLMenuItem(TreeNode node)
        {
            ToolStripMenuItem showDDLMenuItem = new ToolStripMenuItem(Properties.Resources.ShowDDL);
            showDDLMenuItem.Tag = node;
            showDDLMenuItem.Click += new EventHandler(showDDLMenuItem_Click);

            return showDDLMenuItem;
        }

        /// <summary>
        /// Static method to create a Grant/Revole context menu item
        /// </summary>
        /// <returns>The context menu item</returns>
        public static ToolStripMenuItem GetGrantRevokeMenuItem(TreeNode node)
        {
            ToolStripMenuItem showGrantRevokeMenuItem = new ToolStripMenuItem("Grant/Revoke Privileges");
            showGrantRevokeMenuItem.Tag = node;
            showGrantRevokeMenuItem.Click += new EventHandler(showGrantRevokeMenuItem_Click);

            return showGrantRevokeMenuItem;
        }

        /// <summary>
        /// Event handler for the Show DDL menu click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public static void showDDLMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                //Pass the source tree node from which the menu was clicked, to the ShowDDLControl constructor
                ShowDDLControl showDDLForm = new ShowDDLControl(((ToolStripMenuItem)sender).Tag);

                // Get the sqlObject from the node that was selected.
                Object sourceNode = ((ToolStripMenuItem)sender).Tag;
                TrafodionObject theTrafodionObject = null;

                if (sourceNode is DatabaseTreeFolder)
                {
                    theTrafodionObject = ((DatabaseTreeFolder)sourceNode).TrafodionObject;
                }
                else if (sourceNode is DatabaseTreeNode)
                {
                    theTrafodionObject = ((DatabaseTreeNode)sourceNode).TrafodionObject;
                }

                //Place the ShowDDL user control into a managed window
                Utilities.LaunchManagedWindow(Properties.Resources.ShowDDL, showDDLForm, theTrafodionObject.ConnectionDefinition, showDDLForm.Size, true);
                //Trafodion.Manager.Framework.Controls.WindowsManager.PutInWindow(showDDLForm.Size, showDDLForm, Properties.Resources.ShowDDL, theTrafodionObject.ConnectionDefinition);
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error,
                      MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Event handler for the Grant/Revoke menu click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public static void showGrantRevokeMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                // Get the sqlObject from the node that was selected.
                Object sourceNode = ((ToolStripMenuItem)sender).Tag;
                TrafodionObject theTrafodionObject = null;

                if (sourceNode is DatabaseTreeFolder)
                {
                    theTrafodionObject = ((DatabaseTreeFolder)sourceNode).TrafodionObject;
                }
                else if (sourceNode is DatabaseTreeNode)
                {
                    theTrafodionObject = ((DatabaseTreeNode)sourceNode).TrafodionObject;
                }
                GrantRevokeControl grd = new GrantRevokeControl(theTrafodionObject);

                Utilities.LaunchManagedWindow(Properties.Resources.GrantRevokeDialogTitle, grd, theTrafodionObject.ConnectionDefinition, grd.Size, true);
                //Trafodion.Manager.Framework.Controls.WindowsManager.PutInWindow(grd.Size, grd, Properties.Resources.GrantRevokeDialogTitle, theTrafodionObject.ConnectionDefinition);
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error,
                      MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

    }
}
