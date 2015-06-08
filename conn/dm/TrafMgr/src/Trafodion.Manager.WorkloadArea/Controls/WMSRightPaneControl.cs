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
using Trafodion.Manager.DatabaseArea.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Favorites;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.WorkloadArea.Controls.Tree;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// User control for the right pane
    /// </summary>
    public partial class WMSRightPaneControl : TrafodionRightPaneControl
    {
        private EditSystemConfigurationUserControl _editSystemConfigurationUserControl;
        private AlterComponentPrivilegesUserControl _privilegesUserControl;
        private FixSystemUserControl _fixSystemUserControl;
        private MySystemsUserControl _mySystemsUserControl;
        private ServicesSummaryUserControl _serviceSummaryUserControl;
        //private WorkloadsUserControl _workloadsUserControl;
        private TabbedWorkloadUserControlWrapper _workloadsUserControl;
        private RulesSummaryUserControl _rulesSummaryUserControl;
        
        private TrafodionTextBox _nonWMSPrivilegeErrorBox;        
        private TrafodionTabControl _configTabControl = new TrafodionTabControl();
        private TrafodionTabPage _configurationTabPage = new TrafodionTabPage("WMS Defaults and Status");
        private TrafodionTabPage _privilegesTabPage = new TrafodionTabPage("Privileges");

        private WMSNavigationControl _WMSNavigationControl;
        private WMSTreeView _WMSTreeView;
        private FavoritesTreeView _WMSFavoritesTreeView;

        private string previousFullPath = "";        
        private NavigationTreeView.SelectedHandler theWMSTreeControlSelectedHandler = null;
        private TreeViewCancelEventHandler thWMSTreeControlBeforeSelectedHandler = null; //To capture 'Before Select'
        private NavigationTreeNameFilter.ChangedHandler theFilterChangedHandler = null;

        /// <summary>
        /// 
        /// </summary>
        public WMSTreeView WMSTreeView
        {
            get { return _WMSTreeView; }
            set { _WMSTreeView = value; }
        }


        /// <summary>
        /// 
        /// </summary>
        public WMSNavigationControl WMSNavigationControl
        {
            get { return _WMSNavigationControl; }
            set
            {
                _WMSNavigationControl = value;

                _WMSTreeView = (_WMSNavigationControl == null) ? null : _WMSNavigationControl.WMSTreeView;
                
                bool treeAvailable = (_WMSTreeView != null);
                bool favoritestreeAvailable = (_WMSFavoritesTreeView != null);
                if (treeAvailable)
                {

                    //Event handler added to capture the 'Before Select' event. 
                    //This allows us to cancel if changes haven't been saved
                    if (thWMSTreeControlBeforeSelectedHandler == null)
                    {
                        thWMSTreeControlBeforeSelectedHandler = new TreeViewCancelEventHandler(WMSTreeViewControlBeforeSelect);
                    }

                    WMSTreeView.Selected += theWMSTreeControlSelectedHandler;
                    WMSTreeView.BeforeSelect += thWMSTreeControlBeforeSelectedHandler;

                }
            }
        }

        /// <summary>
        /// Handles the event triggered before a 'Select' is performed on the WMS Tree
        /// </summary>
        void WMSTreeViewControlBeforeSelect(object sender, TreeViewCancelEventArgs e)
        {
            try
            {
                // check if user is about to change slected node
                if (previousFullPath != e.Node.FullPath)
                {
                  
                    if (((TrafodionTabPage)_configTabControl.SelectedTab).Equals(_privilegesTabPage) && (this._privilegesUserControl != null && this._privilegesUserControl.HasPrivilegesChanged))
                    {
                        if (!_privilegesUserControl.HasNewGranteeAdded)
                        {
                            DialogResult dr = MessageBox.Show(Trafodion.Manager.Properties.Resources.UnappliedChangesConfirmMessage,
                                Trafodion.Manager.Properties.Resources.UnappliedChangesConfirmMessageCaption, 
                                MessageBoxButtons.YesNoCancel);
                            if (dr == DialogResult.Yes)
                            {
                                //Apply Changes then continue
                                this._privilegesUserControl.ApplyChanges();
                                e.Cancel = false;
                            }
                            else if (dr == DialogResult.No)
                            {
                                //Discard changes and continue
                                this._privilegesUserControl.ResetChanges();
                                e.Cancel = false;
                            }
                            else
                            {
                                //Cancel navigation
                                e.Cancel = true;
                            }
                        }
                        else
                        {
                            DialogResult dr = MessageBox.Show(Trafodion.Manager.Properties.Resources.UnappliedChangesOfAddNewGranteeConfirmMessage,
                                Trafodion.Manager.Properties.Resources.UnappliedChangesOfAddNewGranteeConfirmCaption, MessageBoxButtons.YesNo);
                            if (dr == DialogResult.Yes)
                            {
                                //Discard changes and continue
                                this._privilegesUserControl.ResetChanges();
                                e.Cancel = false;
                            }
                            else
                            {
                                //Cancel navigation
                                e.Cancel = true;
                            }
                        }
                    }
 
                }
            }
            catch 
            {
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public WMSRightPaneControl()
            : base()
        {
            InitializeComponent();
            _configTabControl.TabPages.Clear();
            _configTabControl.TabPages.Add(_configurationTabPage);
        }

        public WMSRightPaneControl(WMSNavigationControl aWMSNavigationControl)
            : base(aWMSNavigationControl)
        {
            InitializeComponent();
            _configTabControl.TabPages.Clear();
            _configTabControl.TabPages.Add(_configurationTabPage);
            this.WMSNavigationControl = aWMSNavigationControl;
        }

        public override void HandleNavigationTreeNodeSelect(NavigationTreeNode aNavigationTreeNode)
        {
            if (Trafodion.Manager.Framework.Utilities.InUnselectedTabPage(this))
            {
                return;
            }

            previousFullPath = aNavigationTreeNode.FullPath;

            ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
            
            if (aNavigationTreeNode is ConfigurationFolder)
            {
                // If logged in, show the WMS Configuration tab
                ConfigurationFolder theSystemConfigFolder = (ConfigurationFolder)aNavigationTreeNode;
                if (_editSystemConfigurationUserControl == null)
                {
                    _editSystemConfigurationUserControl = new EditSystemConfigurationUserControl(theSystemConfigFolder.WmsSystem);
                    _editSystemConfigurationUserControl.Dock = DockStyle.Fill;
                }
                else
                {
                    _editSystemConfigurationUserControl.WmsSystem = theSystemConfigFolder.WmsSystem;
                }
                AddControl(_editSystemConfigurationUserControl);

            }
            else
            if (aNavigationTreeNode is ServicesFolder)
            {
                if (_serviceSummaryUserControl == null)
                {
                    _serviceSummaryUserControl = new ServicesSummaryUserControl(theConnectionDefinition);
                }
                else
                {
                    _serviceSummaryUserControl.ConnectionDefn = theConnectionDefinition;
                }
                AddControl(_serviceSummaryUserControl);
            }
            if (aNavigationTreeNode is ConnRulesFolder)
            {                
                if (_rulesSummaryUserControl == null)
                {
                    _rulesSummaryUserControl = new RulesSummaryUserControl(theConnectionDefinition, WmsCommand.CONN_RULE_TYPE);
                }
                else
                {
                    _rulesSummaryUserControl.RuleType = WmsCommand.CONN_RULE_TYPE;
                    _rulesSummaryUserControl.ConnectionDefn = theConnectionDefinition;
                }
                AddControl(_rulesSummaryUserControl);
            }
            else
            if (aNavigationTreeNode is CompRulesFolder)
            {                     
                if (_rulesSummaryUserControl == null)
                {
                    _rulesSummaryUserControl = new RulesSummaryUserControl(theConnectionDefinition, WmsCommand.COMP_RULE_TYPE);
                }
                else
                {
                    _rulesSummaryUserControl.RuleType = WmsCommand.COMP_RULE_TYPE;
                    _rulesSummaryUserControl.ConnectionDefn = theConnectionDefinition;
                }
                AddControl(_rulesSummaryUserControl);
            }
            else
            if (aNavigationTreeNode is ExecRulesFolder)
            {
                if (_rulesSummaryUserControl == null)
                {
                    _rulesSummaryUserControl = new RulesSummaryUserControl(theConnectionDefinition, WmsCommand.EXEC_RULE_TYPE);
                }
                else
                {
                    _rulesSummaryUserControl.RuleType = WmsCommand.EXEC_RULE_TYPE;
                    _rulesSummaryUserControl.ConnectionDefn = theConnectionDefinition;
                }
                AddControl(_rulesSummaryUserControl);
            }
            else
            if (aNavigationTreeNode is RulesFolder)
            {                
                if (_rulesSummaryUserControl == null)
                {                    
                    _rulesSummaryUserControl = new RulesSummaryUserControl(theConnectionDefinition, "ALL");
                 }
                else
                {                    
                    _rulesSummaryUserControl.RuleType = "ALL";
                    _rulesSummaryUserControl.ConnectionDefn = theConnectionDefinition;
                }

                AddControl(_rulesSummaryUserControl);

                if (!aNavigationTreeNode.IsExpanded)
                {
                    aNavigationTreeNode.Expand();
                }
            }
            else
            {
                if ((theConnectionDefinition != null) && (theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded))
                {
                    AddControl(new FixSystemUserControl(theConnectionDefinition));
                }
                else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionsFolder)
                {
                    AddControl(new MySystemsUserControl(true));
                }
                else if (aNavigationTreeNode is SystemFolder)
                {
                    SystemFolder theSystemFolder = (SystemFolder)aNavigationTreeNode;
                    if (theConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))
                    {
                        if (_editSystemConfigurationUserControl == null)
                        {
                            _editSystemConfigurationUserControl = new EditSystemConfigurationUserControl(theSystemFolder.WmsSystem);
                            _editSystemConfigurationUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
                            _configurationTabPage.Controls.Clear();
                            _configurationTabPage.Controls.Add(_editSystemConfigurationUserControl);
                        }
                        else
                        {
                            _editSystemConfigurationUserControl.WmsSystem = theSystemFolder.WmsSystem;
                        }

                        if (_privilegesUserControl == null)
                        {
                            _privilegesUserControl = new AlterComponentPrivilegesUserControl(theConnectionDefinition, "WMS");
                            _privilegesUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
                            _privilegesTabPage.Controls.Clear();
                            _privilegesTabPage.Controls.Add(_privilegesUserControl);
                        }
                        else
                        {
                            _privilegesUserControl.ConnectionDefinition = theConnectionDefinition;
                        }

                        if (!_configTabControl.TabPages.Contains(_privilegesTabPage))
                        {
                            _configTabControl.TabPages.Add(_privilegesTabPage);
                        }

                        AddControl(_configTabControl);
                        _configTabControl.SelectTab(0);

                        if (!aNavigationTreeNode.IsExpanded)
                        {
                            aNavigationTreeNode.ExpandAll();
                        }
                    }
                    else
                    {
                        if (_nonWMSPrivilegeErrorBox == null)
                        {
                            _nonWMSPrivilegeErrorBox = new TrafodionTextBox();
                            _nonWMSPrivilegeErrorBox.Multiline = true;
                            _nonWMSPrivilegeErrorBox.Dock = System.Windows.Forms.DockStyle.Fill;
                            _nonWMSPrivilegeErrorBox.ReadOnly = true;
                            _nonWMSPrivilegeErrorBox.Text = Properties.Resources.NonWMSPrivilegeErrorText;
                        }
                        AddControl(_nonWMSPrivilegeErrorBox);
                    }
                }
            }
        }

        public override void SetRightPaneHeader(NavigationTreeNode aNavigationTreeNode)
        {            
            if (aNavigationTreeNode is SystemFolder && aNavigationTreeNode.TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded
                && aNavigationTreeNode.TheConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))                            
                    SetLabels("WMS Configuration");            
            else
                SetLabels(aNavigationTreeNode.LongerDescription);      
        }

        public override TrafodionRightPaneControl DoClone()
        {
            return new WMSRightPaneControl();
        }
    }
}
