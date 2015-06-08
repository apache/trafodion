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
using System.ComponentModel;
using System.Windows.Forms;

using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.DatabaseArea.Controls;

namespace Trafodion.Manager.ConnectivityArea.Controls
{

    /// <summary>
    /// This user control allows the user to view, set, and edit the settings that define what
    /// the users thinks is a system but really is a connection definition.
    /// <para/>
    /// An event is fired anytime the user chnages anything in this control and a container can
    /// use this event to determine whether or not it should chnage its controls.
    /// </summary>
    public partial class ConnectivityAreaConfigControlledTablesAddBrowseUserControl : UserControl
    {

        #region Member Variables

        // True if editing an existing connection definition, false if adding a new one
        private bool _isEditing = false;

        // The starting version for editing or adding like, null if new
        private object _theLoadedControlledTable = null;

        #endregion

        /// <summary>
        /// The constructor
        /// </summary>
        public ConnectivityAreaConfigControlledTablesAddBrowseUserControl()
        {

            // Call the designer-generated code
            InitializeComponent();
            SetupTreeView();
        }

        public void SetupTreeView()
        {
            this.theDatabaseTreeView.SetAndPopulateRootFolders();
        }

        public void PopulateDataTreeView()
        {
            //SetupTreeView();
            this.theDatabaseTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;
            TreeNode systemNode = (this.theDatabaseTreeView.SelectedNode);
            this.theDatabaseTreeView.Nodes.Clear();// = (this.theDatabaseTreeView.SelectedNode);
            this.theDatabaseTreeView.Nodes.Add(systemNode);
            systemNode.Expand();

            //this.theDatabaseTreeView.
        }

        public string GetFullPathOfSelected()
        {
            try
            {
                /* Previous verson  -  for rollback
                string delimeter = ".";
                TreeNode tableNode = this.theDatabaseTreeView.SelectedNode;
                TreeNode schemaNode = tableNode.Parent.Parent;
                TreeNode catalogNode = schemaNode.Parent.Parent;               
                return catalogNode.Text + delimeter + schemaNode.Text + delimeter + tableNode.Text;
                */
            
                //Get the currently selected node and cast as a DatabaseTreeFolder
                if (this.theDatabaseTreeView.SelectedNode is DatabaseArea.Controls.Tree.TableFolder)
                {
                    DatabaseArea.Controls.Tree.TableFolder tableNode = (DatabaseArea.Controls.Tree.TableFolder)this.theDatabaseTreeView.SelectedNode;
                    return tableNode.TrafodionTable.RealAnsiName; 
                }

                return "";     
            }
            catch
            {
                //Error obtaining full path of selected tree item.
                return "";
            }
        }


    }

}
