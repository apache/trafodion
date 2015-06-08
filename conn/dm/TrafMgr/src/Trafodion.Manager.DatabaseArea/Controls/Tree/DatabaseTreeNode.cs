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
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Base class for Database Area navigation tree leaf nodes
    /// This class overrides the framework and provides custom context menu capabilities
    /// </summary>
    public class DatabaseTreeNode : NavigationTreeNode
    {
        private TrafodionObject _sqlMxObject = null;
        private System.EventHandler<TrafodionObject.TrafodionModelChangeEventArgs> _modelChangedHandler = null;
        private System.EventHandler _modelRemovedHandler = null;
        private System.EventHandler<TrafodionObject.TrafodionModelEventArgs> _modelReplacedHandler = null;

        /// <summary>
        /// The sql object represented by this node
        /// </summary>
        public TrafodionObject TrafodionObject
        {
            get { return _sqlMxObject; }
            set { _sqlMxObject = value; }
        }

        /// <summary>
        /// Constructs the leaf node for the sql object
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        public DatabaseTreeNode(TrafodionObject aTrafodionObject)
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
        /// Register to listen on the sql object model events
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        private void AddHandlers(TrafodionObject aTrafodionObject)
        {
            aTrafodionObject.ModelChangedEvent += _modelChangedHandler;
            aTrafodionObject.ModelRemovedEvent += _modelRemovedHandler;
            aTrafodionObject.ModelReplacedEvent += _modelReplacedHandler;
        }

        /// <summary>
        /// Unregister listeners on the sql object model
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
        /// Handle model change event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public virtual void TrafodionObject_ModelChangedEvent(object sender, TrafodionObject.TrafodionModelChangeEventArgs e)
        {

        }

        /// <summary>
        /// Handle the Sql object removed event
        /// </summary>
        /// <param name="sender">The sql object model that was removed</param>
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
                    System.String.Format(Properties.Resources.ObjectNotAvailable, ((TrafodionObject)sender).ExternalName),
                    Properties.Resources.Warning, System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning
                    );
            }

            this.Remove();
        }

        /// <summary>
        /// Handle the model replaced event
        /// </summary>
        /// <param name="sender">the sql model that was replaced</param>
        /// <param name="e">Contains the new model that replaces the old</param>
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
                System.String.Format(Properties.Resources.ObjectRecreated, ((TrafodionObject)sender).ExternalName),
                Properties.Resources.Warning, System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning
                );
            }
        }
        
        /// <summary>
        /// Short description of this node
        /// </summary>
        override public string ShortDescription
        {
            get { return TrafodionObject.ExternalName; }
        }

        /// <summary>
        /// Long description of this node
        /// </summary>
        override public string LongerDescription
        {
            get { return TrafodionObject.VisibleAnsiName; }
        }

        /// <summary>
        /// Refreshes the state of the node
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TrafodionObject.Refresh();
        }
        
        /// <summary>
        /// Add ShowDDL menu to the context menu list
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        override public void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);

            //If the sql object support DDL, display the ShowDDL menu
            if (TrafodionObject.AllowShowDDL)
                aContextMenuStrip.Items.Add(DatabaseTreeFolder.GetShowDDLMenuItem(this));

            if (TrafodionObject.SupportsPrivileges(this.TrafodionObject))
            {
                aContextMenuStrip.Items.Add(DatabaseTreeFolder.GetGrantRevokeMenuItem(this));
            }
        }
    }
}
