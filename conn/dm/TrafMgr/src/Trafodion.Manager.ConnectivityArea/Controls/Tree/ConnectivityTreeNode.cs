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
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.ConnectivityArea.Model;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{
    /// <summary>
    /// A BASE class for a leaf node that holds a  Connectivity Object
    /// </summary>
    public class ConnectivityTreeNode : NavigationTreeNode
    {
        #region Member variables

        private NDCSObject _ndcsObject = null;
        private System.EventHandler<NDCSObject.NDCSModelEventArgs> _modelChangedHandler = null;
        private System.EventHandler _modelRemovedHandler = null;
        private System.EventHandler<NDCSObject.NDCSModelEventArgs> _modelReplacedHandler = null;

        #endregion Member variables

        #region Properties

        /// <summary>
        /// The Connectivity Object contained in the tree node
        /// </summary>
        public NDCSObject NDCSObject
        {
            get { return _ndcsObject; }
            set { _ndcsObject = value; }
        }

        /// <summary>
        /// The short description of the Connectivity object contained in the tree node
        /// </summary>
        public override string ShortDescription
        {
            get { return NDCSObject.Name; }
        }

        /// <summary>
        /// The long description of the Connectivity object contained in the tree node
        /// </summary>
        public override string LongerDescription
        {
            get { return NDCSObject.Name; }
        }
        #endregion Properties


        /// <summary>
        /// Constructs the base of a leaf node for the connectivity object
        /// </summary>
        /// <param name="aNdcsObject"></param>
        public ConnectivityTreeNode(NDCSObject aNdcsObject)
        {
            ImageKey = ConnectivityTreeView.BLANK_DOCUMENT_ICON;
            SelectedImageKey = ConnectivityTreeView.BLANK_DOCUMENT_ICON;

            _ndcsObject = aNdcsObject;
            Text = aNdcsObject.Name;
            Tag = aNdcsObject;

            _modelChangedHandler = new System.EventHandler<NDCSObject.NDCSModelEventArgs>(NDCSObject_ModelChangedEvent);
            _modelRemovedHandler = new System.EventHandler(NDCSObject_ModelRemovedEvent);
            _modelReplacedHandler = new System.EventHandler<NDCSObject.NDCSModelEventArgs>(NDCSObject_ModelReplacedEvent);

            //Register the event handlers for the Wms object model
            AddHandlers(NDCSObject);
        }

        /// <summary>
        /// Register to listen on the Connectivity object model events
        /// </summary>
        /// <param name="aNdcsObject"></param>
        private void AddHandlers(NDCSObject aNdcsObject)
        {
            aNdcsObject.ModelChangedEvent += _modelChangedHandler;
            aNdcsObject.ModelRemovedEvent += _modelRemovedHandler;
            aNdcsObject.ModelReplacedEvent += _modelReplacedHandler;
        }

        /// <summary>
        /// Unregister listeners on the Connectivity object model
        /// </summary>
        /// <param name="aNdcsObject"></param>
        private void RemoveHandlers(NDCSObject aNdcsObject)
        {
            aNdcsObject.ModelChangedEvent -= _modelChangedHandler;
            aNdcsObject.ModelRemovedEvent -= _modelRemovedHandler;
            aNdcsObject.ModelReplacedEvent -= _modelReplacedHandler;
        }

        /// <summary>
        /// Handle model change event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void NDCSObject_ModelChangedEvent(object sender, NDCSObject.NDCSModelEventArgs e)
        {

        }

        /// <summary>
        /// Handle the Connectivity object removed event
        /// </summary>
        /// <param name="sender">The Connectivity object model that was removed</param>
        /// <param name="e"></param>
        void NDCSObject_ModelRemovedEvent(object sender, System.EventArgs e)
        {
            //Remove the model event handlers from the old wms model
            RemoveHandlers((NDCSObject)sender);

            this.NDCSObject = null;
            if ( (TreeView == null) || (TreeView.SelectedNode == this) )
            {
                System.Windows.Forms.MessageBox.Show(
                    System.String.Format("Object {0} is not accessible anymore. It may have been removed in another session.", ((NDCSObject)sender).Name),
                    "Warning", System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning
                    );
            }

            this.Remove();
        }

        /// <summary>
        /// Handle the model replaced event
        /// </summary>
        /// <param name="sender">the connectivity model that was replaced</param>
        /// <param name="e">Contains the new model that replaces the old</param>
        void NDCSObject_ModelReplacedEvent(object sender, NDCSObject.NDCSModelEventArgs e)
        {
            //Remove the model event handlers from the old wms model
            RemoveHandlers((NDCSObject)sender);

            //Set the new wms model and register handlers to listen to the model events
            this.NDCSObject = e.NDCSObject;
            AddHandlers(NDCSObject);

            if (TreeView.SelectedNode == this)
            {
                System.Windows.Forms.MessageBox.Show(
                System.String.Format("Object {0} has changed. It may have been recreated in another session.", ((NDCSObject)sender).Name),
                "Warning", System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning
                );
            }
        }

        /// <summary>
        /// Refreshes the node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            NDCSObject.Refresh();
        }
    }
}
