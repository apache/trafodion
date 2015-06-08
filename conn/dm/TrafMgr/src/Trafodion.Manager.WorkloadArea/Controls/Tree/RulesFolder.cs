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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.WorkloadArea.Model;


namespace Trafodion.Manager.WorkloadArea.Controls.Tree
{
    /// <summary>
    /// Represents the services folder in the navigation tree
    /// </summary>
    public class RulesFolder : WmsTreeFolder
    {

        private RulesSubFolder _connectionRulesFolder = null;
        private RulesSubFolder _compilationRulesFolder = null;
        private RulesSubFolder _executionRulesFolder = null;

        #region Properties

        /// <summary>
        /// Returns the WMS System associated with this folder
        /// </summary>
        public WmsSystem WmsSystem
        {
            get { return WmsObject as WmsSystem; }
        }

        #endregion Properties

        /// <summary>
        /// Default constructor for the services folder
        /// </summary>
        /// <param name="aWmsSystem"></param>
        public RulesFolder(WmsSystem aWmsSystem)
            :base(aWmsSystem, false)
        {
            Text = Properties.Resources.Rules;
        }

        /// <summary>
        /// Returns the name displayed in the navigation tree
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return Properties.Resources.Rules;
            }
        }

        /// <summary>
        /// Returns the longer description
        /// </summary>
        override public string LongerDescription
        {
            get
            {
                return ShortDescription;
            }
        }
       
        /// <summary>
        /// Populates this tree node
        /// </summary>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            //Let the child nodes do any clean up before they are disposed
            foreach (TreeNode rulesSubFolder in Nodes)
            {
                if (rulesSubFolder is RulesSubFolder)
                {
                    //((RulesSubFolder)rulesSubFolder).CleanupBeforeDispose();
                }
            } 
            
            Nodes.Clear();

            _connectionRulesFolder = new ConnRulesFolder(WmsSystem);
            _compilationRulesFolder = new CompRulesFolder(WmsSystem);
            _executionRulesFolder = new ExecRulesFolder(WmsSystem);

            Nodes.Add(_connectionRulesFolder);
            Nodes.Add(_compilationRulesFolder);
            Nodes.Add(_executionRulesFolder);
        }

        /// <summary>
        /// Refreshes the list of services
        /// </summary>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            try 
            {
                WmsSystem.WmsRules = null;
                Populate(null);
            } catch (Exception ex) {
                // Got an error. Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), "\nInternal error refreshing WMS rules. \n\n" +
                                "Problem: \t Internal error has occurred while refreshing WMS rules.\n\n" +
                                "Solution: \t Please see error details for recovery information.\n\n" +
                                "Details: \t " + ex.Message + "\n\n",
                                "Internal Error Refreshing WMS Rules", MessageBoxButtons.OK, MessageBoxIcon.Error);

            }

        } 

        /// <summary>
        /// Adds items to the context menu for the services folder
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        public override void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);

            aContextMenuStrip.Items.Add(new ToolStripSeparator());

            ToolStripMenuItem addRuleMenuItem = new ToolStripMenuItem("Add Rule");
            addRuleMenuItem.Tag = this;
            addRuleMenuItem.Click += new EventHandler(addRuleMenuItem_Click);
            

            ToolStripMenuItem associateRuleMenuItem = new ToolStripMenuItem("Associate Rule");
            associateRuleMenuItem.Tag = this;
            associateRuleMenuItem.Click += new EventHandler(associateRuleMenuItem_Click);

            addRuleMenuItem.Enabled = WmsSystem.IsRulesLoaded
                && WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ADD.ToString());
            associateRuleMenuItem.Enabled = WmsSystem.IsRulesLoaded 
                && WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ALTER.ToString());

            aContextMenuStrip.Items.Add(addRuleMenuItem);
            aContextMenuStrip.Items.Add(associateRuleMenuItem);

        }

        protected virtual void associateRuleMenuItem_Click(object sender, EventArgs e)
        {
            string ruleType = WmsCommand.CONN_RULE_TYPE;
            if (this.TreeView.SelectedNode is RulesSubFolder)
                ruleType = ((RulesSubFolder)TreeView.SelectedNode).RuleType; 

            AssociateRuleDialog ard = new AssociateRuleDialog(WmsSystem, ruleType, null);
            ard.ShowDialog();
        }

        void addRuleMenuItem_Click(object sender, EventArgs e)
        {
            string ruleType = WmsCommand.CONN_RULE_TYPE;
            if (this.TreeView.SelectedNode is RulesSubFolder)
                ruleType = ((RulesSubFolder)TreeView.SelectedNode).RuleType;

            AddRuleDialog addDialog = new AddRuleDialog(WmsSystem, ruleType);
            if (addDialog.ShowDialog() == DialogResult.OK)
            {
                DoRefresh(null);
            }
        }

        /// <summary>
        /// Refresh the childNodes
        /// </summary>
        public void RefreshSiblings(RulesSubFolder aWmsRulesSubFolder)
        {
            // Repopulate the tree
            foreach (TreeNode theTreeNode in Nodes)
            {
                //Ignore the child node that requested the refresh
                //Since it is already refreshed
                if (theTreeNode == aWmsRulesSubFolder)
                    continue;

                if (theTreeNode is RulesSubFolder)
                {
                    RulesSubFolder siblingFolder = (RulesSubFolder)theTreeNode;
                    siblingFolder.Populate(null);
                }
            }
        }
    }
}
