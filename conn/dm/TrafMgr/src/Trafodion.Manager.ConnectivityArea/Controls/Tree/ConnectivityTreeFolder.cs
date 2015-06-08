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
    /// Base class for Tree folder in Connectivity Area
    /// that holds a ConnectivityObject
    /// </summary>
    public class ConnectivityTreeFolder : NavigationTreeFolder
    {
        #region Member variables

        private NDCSObject _connectivityObject = null;
        private System.EventHandler<NDCSObject.NDCSModelEventArgs> _modelChangedHandler = null;
        private System.EventHandler _modelRemovedHandler = null;
        private System.EventHandler<NDCSObject.NDCSModelEventArgs> _modelReplacedHandler = null;

        #endregion Member variables

        #region Properties

        /// <summary>
        /// The Connectivity Object represented by this folder node
        /// </summary>
        public NDCSObject ConnectivityObject
        {
            get { return _connectivityObject; }
            set { _connectivityObject = value; }
        }

        /// <summary>
        /// Short description of the Connectivity Object contained in the folder
        /// </summary>
        public override string ShortDescription
        {
            get { return ConnectivityObject.Name; }
        }

        /// <summary>
        /// Long description of the Connectivity Object contianed in the folder
        /// </summary>
        public override string LongerDescription
        {
            get { return ConnectivityObject.Name; }
        }

        #endregion Properties

        /// <summary>
        /// Constructs the BASE of tree folder node for hold the Connectivity object.
        /// </summary>
        /// <param name="aConnectivityObject">The Connectivity object for this folder</param>
        /// <param name="isSubFolder">If the folder is just a subfolder holding other subfolders</param>
        public ConnectivityTreeFolder(NDCSObject aConnectivityObject, bool isSubFolder)
        {
            _connectivityObject = aConnectivityObject;
            Text = ShortDescription;

            ImageKey = ConnectivityTreeView.FOLDER_CLOSED_ICON;
            SelectedImageKey = ConnectivityTreeView.FOLDER_CLOSED_ICON;

            if (!isSubFolder)
            {
                _modelChangedHandler = new System.EventHandler<NDCSObject.NDCSModelEventArgs>(ConnectivityObject_ModelChangedEvent);
                _modelRemovedHandler = new System.EventHandler(ConnectivityObject_ModelRemovedEvent);
                _modelReplacedHandler = new System.EventHandler<NDCSObject.NDCSModelEventArgs>(ConnectivityObject_ModelReplacedEvent);

                //Register the event handlers for the Connectivity object model
                AddHandlers(ConnectivityObject);
            }
        }

        /// <summary>
        /// Register to listen on the Connectivity Object model events
        /// </summary>
        /// <param name="aConnectivityObject"></param>
        private void AddHandlers(NDCSObject aConnectivityObject)
        {
            aConnectivityObject.ModelChangedEvent += _modelChangedHandler;
            aConnectivityObject.ModelRemovedEvent += _modelRemovedHandler;
            aConnectivityObject.ModelReplacedEvent += _modelReplacedHandler;
        }

        /// <summary>
        /// Unregister listeners on the Connectivity object model
        /// </summary>
        /// <param name="aConnectivityObject"></param>
        private void RemoveHandlers(NDCSObject aConnectivityObject)
        {
            aConnectivityObject.ModelChangedEvent -= _modelChangedHandler;
            aConnectivityObject.ModelRemovedEvent -= _modelRemovedHandler;
            aConnectivityObject.ModelReplacedEvent -= _modelReplacedHandler;
        }

        /// <summary>
        /// Handle model change event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ConnectivityObject_ModelChangedEvent(object sender, NDCSObject.NDCSModelEventArgs e)
        {

        }

        /// <summary>
        /// Handle the Connectivity object removed event
        /// </summary>
        /// <param name="sender">The Wms object model that was removed</param>
        /// <param name="e"></param>
        void ConnectivityObject_ModelRemovedEvent(object sender, System.EventArgs e)
        {
            //Remove the model event handlers from the old wms model
            RemoveHandlers((NDCSObject)sender);

            this.ConnectivityObject = null;
            if (TreeView.SelectedNode == this)
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
        /// <param name="sender">the wms model that was replaced</param>
        /// <param name="e">Contains the new model that replaces the old</param>
        void ConnectivityObject_ModelReplacedEvent(object sender, NDCSObject.NDCSModelEventArgs e)
        {
            //Remove the model event handlers from the old wms model
            RemoveHandlers((NDCSObject)sender);

            //Set the new wms model and register handlers to listen to the model events
            this.ConnectivityObject = e.NDCSObject;
            AddHandlers(ConnectivityObject);

            if (TreeView.SelectedNode == this)
            {
                System.Windows.Forms.MessageBox.Show(
                System.String.Format("Object {0} has changed. It may have been recreated in another session.", ((NDCSObject)sender).Name),
                "Warning", System.Windows.Forms.MessageBoxButtons.OK, System.Windows.Forms.MessageBoxIcon.Warning
                );
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {

        }
    }
}
