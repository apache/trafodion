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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.ConnectivityArea.Controls.Tree;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    public enum DSConfigFormType { Edit = 0, Create = 1, CreateLike = 2 };

    /// <summary>
    /// User control that's displayed in the right pane
    /// </summary>
    public partial class ConnectivityAreaDatasourceConfigurationUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        private ConnectivityNavigationControl _connectivityNavigationControl;
        private ConnectivityTreeView _connectivityTreeView;
        private NavigationTreeNode _activeTreeNode;
        private NavigationTreeView.SelectedHandler theConnectivityTreeControlSelectedHandler = null;
        private DSConfigFormType _formType = DSConfigFormType.Edit;
        private TrafodionForm _createForm;
        private DataSourcesFolder _datasourcesFolder;


        //Vars to keep track of the configuration user controls. These are used for tracking changes made to the DS.
        private ConnectivityAreaDatasourceConfigGeneralUserControl _generalConfig;
        private ConnectivityAreaDatasourceConfigDefineSetUserControl _defineSetConfig;
        private ConnectivityAreaDatasourceConfigCQDUserControl _cqdConfig;
        private ConnectivityAreaDatasourceConfigControlledTablesUserControl _controlledTableConfig;

        private int currentSelectedDataSourceFolderSubTabPageIdx = 0;

        #endregion Fields

        # region Properties

        public ConnectivityAreaDatasourceConfigGeneralUserControl ConnectivityAreaDatasourceConfigGeneralUserControl
        {
            get
            {
                return _generalConfig;
            }
        }
        /// <summary>
        /// 
        /// </summary>
        public NavigationTreeNode ActiveTreeNode
        {
            get { return _activeTreeNode; }
            set { _activeTreeNode = value; }
        }

        public TrafodionForm CreateForm
        {
            set { _createForm = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityTreeView ConnectivityTreeView
        {
            get { return _connectivityTreeView; }
            set { _connectivityTreeView = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityNavigationControl ConnectivityNavigationControl
        {
            get { return _connectivityNavigationControl; }
            set
            {
                _connectivityNavigationControl = value;

                _connectivityTreeView = (_connectivityNavigationControl == null) ? null : _connectivityNavigationControl.ConnectivityTreeView;

                {
                    bool treeAvailable = (_connectivityTreeView != null);

                    if (treeAvailable)
                    {
                        if (theConnectivityTreeControlSelectedHandler == null)
                        {
                            theConnectivityTreeControlSelectedHandler = new NavigationTreeView.SelectedHandler(ConnectivityTreeControlSelected);
                        }
                        ConnectivityTreeView.Selected += theConnectivityTreeControlSelectedHandler;

                    }

                }

            }
        }

        #endregion Properties

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            ConnectivityAreaDatasourceConfigurationUserControl theClonedConnectivityAreaDatasourceConfigurationUserControl =
                new ConnectivityAreaDatasourceConfigurationUserControl(this._connectivityTreeView);

            theClonedConnectivityAreaDatasourceConfigurationUserControl.ActiveTreeNode = this.ActiveTreeNode;
            theClonedConnectivityAreaDatasourceConfigurationUserControl.SetLabels(theTopPanelLowerLabel.Text);

            if (theClonedConnectivityAreaDatasourceConfigurationUserControl.ActiveTreeNode is TreeNode)
            {
                theClonedConnectivityAreaDatasourceConfigurationUserControl.TabSelected();
            }
            //else if (theClonedConnectivityAreaMonitoringUserControl is Exception)
            //{
            //    // raise the exception here
            //}

            theClonedConnectivityAreaDatasourceConfigurationUserControl.Size = Size;

            return theClonedConnectivityAreaDatasourceConfigurationUserControl;

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.TabPageTitle_Configuration + " | " + theTopPanelLowerLabel.Text; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return this.ConnectivityTreeView.CurrentConnectionDefinition; }
        }

        #endregion

        /// <summary>
        /// Default Constructor
        /// </summary>
        public ConnectivityAreaDatasourceConfigurationUserControl(ConnectivityTreeView theConnectivityTreeView)
        {
            InitializeComponent();

            if (theConnectivityTreeView != null)
            {
                this._connectivityTreeView = theConnectivityTreeView;
            }

            // Initially non-blank to make them easy to see in the designer so set them empty now
            theTopPanelLowerLabel.Text = "";


            this.dsCancel_TrafodionButton.Visible = false;
            this.dsApply_TrafodionButton.Click += new System.EventHandler(this.dsApply_TrafodionButton_Click);
        }

        /// <summary>
        /// Default Constructor
        /// </summary>
        public ConnectivityAreaDatasourceConfigurationUserControl(ConnectivityTreeView theConnectivityTreeView, DSConfigFormType aFormType)
        {

            InitializeComponent();

            if (theConnectivityTreeView != null)
            {
                this._connectivityTreeView = theConnectivityTreeView;
            }


            // Initially non-blank to make them easy to see in the designer so set them empty now
            theTopPanelLowerLabel.Text = "";

            this._formType = aFormType;
            if (aFormType == DSConfigFormType.Create || aFormType == DSConfigFormType.CreateLike)
            {
                this.dsApply_TrafodionButton.Text = "&Create";
                this.dsApply_TrafodionButton.Click += new System.EventHandler(this.dsCreate_TrafodionButton_Click);

                this.theTopPanel.Visible = false;
                this.dsReload_TrafodionButton.Text = "&Reset";
                this.dsCancel_TrafodionButton.Visible = true;
            }
            else
            {
                this.dsApply_TrafodionButton.Text = "&Apply";
                this.dsApply_TrafodionButton.Click += new System.EventHandler(this.dsApply_TrafodionButton_Click);
                this.dsCancel_TrafodionButton.Visible = false;
            }
        }

        public void TabSelected()
        {
            this.ConnectivityTreeControlSelected(this._activeTreeNode);
        }


        /// <summary>
        /// Handles clicks to the Top TabControl tabs
        /// </summary>
        /// 
        void DatasourceConfiguration_PropogateTabSelectionChange(object sender, EventArgs e)
        {
            TrafodionTabPage theSelectedTabPage = (TrafodionTabPage)TheConnectivityAreaTabControl.SelectedTab;

            if (this.ConnectivityTreeView.SelectedNode is DataSourceFolder)
            {
                currentSelectedDataSourceFolderSubTabPageIdx = this.TheConnectivityAreaTabControl.SelectedIndex;
            }
        }
    
        /// <summary>
        /// Handles clicks to the  Connectivity Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        public void ConnectivityTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
            if (theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                string theTopPanelLowerText = aNavigationTreeNode.LongerDescription;

                // Change the main area depending on what type of node is selected                
                if (this._formType.Equals(DSConfigFormType.Create) || this._formType.Equals(DSConfigFormType.CreateLike))
                {
                    // We are creating a data source
                    NDCSDataSource dataSource = null;
                    if (this._formType.Equals(DSConfigFormType.Create))
                    {
                        //Create brand new data source based on TreeNode object
                        this._datasourcesFolder = (DataSourcesFolder)aNavigationTreeNode;
                        dataSource = new NDCSDataSource(this._datasourcesFolder.ConnectivitySystem, "New Datasource");
                        dataSource.ResourceStats.connectInfo = true;
                        dataSource.ResourceStats.sessionSummary = true;
                    }
                    else
                    {
                        //Create Data source like
                        this._datasourcesFolder = (DataSourcesFolder)aNavigationTreeNode.Parent;
                        dataSource = ((DataSourceFolder)aNavigationTreeNode).NdcsDataSource;
                        dataSource.PopulateDetail();
                    }

                    if (this._generalConfig != null)
                    {
                        _generalConfig.Dispose();
                    }
                    this._generalConfig = new ConnectivityAreaDatasourceConfigGeneralUserControl(dataSource, (!this._formType.Equals(DSConfigFormType.Edit)));

                    if (this._defineSetConfig != null)
                    {
                        _defineSetConfig.Dispose();
                    }
                    this._defineSetConfig = new ConnectivityAreaDatasourceConfigDefineSetUserControl(dataSource);

                    if (this._cqdConfig != null)
                    {
                        _cqdConfig.Dispose();
                    }
                    this._cqdConfig = new ConnectivityAreaDatasourceConfigCQDUserControl(dataSource);

                    if (this._controlledTableConfig != null)
                    {
                        _controlledTableConfig.Dispose();
                    }
                    this._controlledTableConfig = new ConnectivityAreaDatasourceConfigControlledTablesUserControl(dataSource);

                    AddToTabControl(_generalConfig, Properties.Resources.TabPageLabel_GeneralProperties);
                    AddNewTabControl(_defineSetConfig, Properties.Resources.TabPageLabel_Sets);
                    AddNewTabControl(_cqdConfig, Properties.Resources.TabPageLabel_CQD);
                    AddNewTabControl(_controlledTableConfig, Properties.Resources.TabPageLabel_ControlledTables);

                }
                else if (aNavigationTreeNode is NDCSServicesFolder)
                {
                    NDCSServicesFolder theServicesFolder = (NDCSServicesFolder)aNavigationTreeNode;
                    AddToTabControl(new ConnectivitySystemSummaryUserControl(theServicesFolder.ConnectivitySystem),
                        theServicesFolder.Name);
                }
                else if (aNavigationTreeNode is NDCSServiceFolder)
                {
                    NDCSServicesFolder theServicesFolder = (NDCSServicesFolder)aNavigationTreeNode.Parent;
                    NDCSServiceFolder theServiceFolder = (NDCSServiceFolder)aNavigationTreeNode;

                    AddToTabControl(new ConnectivitySystemSummaryUserControl(theServicesFolder.ConnectivitySystem),
                        theServicesFolder.Name);
                }
                else if (aNavigationTreeNode is DataSourcesFolder)
                {
                    DataSourcesFolder dataSourcesFolder = (DataSourcesFolder)aNavigationTreeNode;
                    AddToTabControl(new ConnectivityAreaDatasourcesConfigPermissionsUserControl(dataSourcesFolder.ConnectivitySystem),
                        "Permissions");

                    //                AddToTabControl(new ServicesUserControl(this, theServicesFolder.WmsSystem), Properties.Resources.Services);
                }
                else if (aNavigationTreeNode is DataSourceFolder)
                {
                    //setup DS config
                    DataSourcesFolder dataSourcesFolder = (DataSourcesFolder)aNavigationTreeNode.Parent;
                    DataSourceFolder dataSourceFolder = (DataSourceFolder)aNavigationTreeNode;
                    theTopPanelLowerText = Properties.Resources.Label_Datasource + " " + aNavigationTreeNode.LongerDescription;

                    try
                    {
                        dataSourceFolder.NdcsDataSource.PopulateDetail();
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }

                    if (this._generalConfig != null)
                    {
                        _generalConfig.Dispose();
                    }
                    this._generalConfig = new ConnectivityAreaDatasourceConfigGeneralUserControl(dataSourceFolder.NdcsDataSource, false);

                    if (this._defineSetConfig != null)
                    {
                        _defineSetConfig.Dispose();
                    }
                    this._defineSetConfig = new ConnectivityAreaDatasourceConfigDefineSetUserControl(dataSourceFolder.NdcsDataSource);

                    if (this._cqdConfig != null)
                    {
                        _cqdConfig.Dispose();
                    }
                    this._cqdConfig = new ConnectivityAreaDatasourceConfigCQDUserControl(dataSourceFolder.NdcsDataSource);

                    if (this._controlledTableConfig != null)
                    {
                        _controlledTableConfig.Dispose();
                    }
                    this._controlledTableConfig = new ConnectivityAreaDatasourceConfigControlledTablesUserControl(dataSourceFolder.NdcsDataSource);

                    this.dsApply_TrafodionButton.Enabled = false;

                    //Set the back-reference to this control for handling the Apply button
                    this._generalConfig.LinkToParent = this;
                    this._defineSetConfig.LinkToParent = this;
                    this._cqdConfig.LinkToParent = this;
                    this._controlledTableConfig.LinkToParent = this;

                    AddToTabControl(_generalConfig, Properties.Resources.TabPageLabel_GeneralProperties);

                    AddNewTabControl(_defineSetConfig, Properties.Resources.TabPageLabel_Sets);

                    AddNewTabControl(_cqdConfig, Properties.Resources.TabPageLabel_CQD);

                    AddNewTabControl(_controlledTableConfig, Properties.Resources.TabPageLabel_ControlledTables);

                    if (currentSelectedDataSourceFolderSubTabPageIdx > -1)
                    {
                        this.TheConnectivityAreaTabControl.SelectTab(currentSelectedDataSourceFolderSubTabPageIdx);
                    }

                }
                else
                {

                    if ((theConnectionDefinition != null) && (theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded))
                    {
                        AddToTabControl(new FixSystemTabPage(theConnectionDefinition));
                    }
                    else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionsFolder)
                    {
                        AddToTabControl(new MySystemsTabPage());
                    }
                    else if (aNavigationTreeNode is ConnectivitySystemFolder)
                    {
                        // If logged in, show the Connectivity Monitoring tab
                        ConnectivitySystemFolder theSystemFolder = (ConnectivitySystemFolder)aNavigationTreeNode;
                        AddToTabControl(new ConnectivitySystemSummaryUserControl(theSystemFolder.ConnectivitySystem),
                            Properties.Resources.ActiveSystemSummary);
                    }

                }

                this.TheConnectivityAreaTabControl.TrafodionTabControlSelectedEvent += new TabControlEventHandler(DatasourceConfiguration_PropogateTabSelectionChange);

                SetLabels(theTopPanelLowerText);
            }
        }


        /// <summary>
        /// Helper method to add a control into a new tab into the right pane
        /// </summary>
        /// <param name="aUserControl"></param>
        public bool datasourceValuesChanged()
        {
            try
            {
                if (!ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER"))
                    return false;
                if (_generalConfig.ValuesChanged || _defineSetConfig.ValuesChanged || _cqdConfig.ValuesChanged || _controlledTableConfig.ValuesChanged)
                    return true;
            }
            catch (Exception e)
            {
            }

            return false;
        }

        /// <summary>
        /// Helper method to add a control into a new tab into the right pane
        /// </summary>
        /// <param name="aUserControl"></param>
        private void AddNewTabControl(UserControl aUserControl, string aTabText)
        {
            TabPage theTabPage = new TrafodionTabPage(aTabText);
            theTabPage.AutoScroll = true;

            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);
            //TheConnectivityAreaTabControl.TabPages.Add(theTabPage);
            AddNewTabControl(theTabPage);
        }

        private void AddNewTabControl(TabPage aTabPage)
        {
            aTabPage.AutoScroll = true;
            if(TheConnectivityAreaTabControl == null)
                TheConnectivityAreaTabControl = new TrafodionTabControl();

            aTabPage.Size = new System.Drawing.Size(1, 1); //To prevent a small square from being shown until the tab page resizes
            TheConnectivityAreaTabControl.TabPages.Add(aTabPage);
        }

        /// <summary>
        /// Helper method to add controls into a tab into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aUserControl"></param>
        private void AddToTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabControl topTabControl = new TrafodionTabControl();           

            // Create the tab page with the user control dock filled
            TabPage theTabPage = new TrafodionTabPage(aTabText);
            //theTabPage.AutoScroll = true;

            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            //aUserControl.AutoScroll = true;
            theTabPage.Controls.Add(aUserControl);

            AddToTabControl(theTabPage);
        }

        /// <summary>
        /// Helper method to add a tab page into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aTabPage"></param>
        private void AddToTabControl(TabPage aTabPage)
        {
            TrafodionTabControl aConnectivityTabControl = new TrafodionTabControl();
            aConnectivityTabControl.TabPages.Add(aTabPage);
            TheConnectivityAreaTabControl = aConnectivityTabControl;
        }

        /// <summary>
        /// Get and set the tab control
        /// </summary>
        public TrafodionTabControl TheConnectivityAreaTabControl
        {
            get
            {
                return _connectivityTopTabControl;
            }
            set
            {
                foreach (Control control in _ConnectivityMainBodyPanel.Controls)
                {
                    control.Dispose();
                }
                _ConnectivityMainBodyPanel.Controls.Clear();
                _connectivityTopTabControl = value;
                _connectivityTopTabControl.Dock = DockStyle.Fill;
                _ConnectivityMainBodyPanel.Controls.Add(_connectivityTopTabControl);
            }
        }

        private delegate void SetLabelsDelegate(string aTopPanelLowerText);
        private void SetLabels(string aTopPanelLowerText)
        {
            if (InvokeRequired)
            {
                Invoke(new SetLabelsDelegate(SetLabels), new object[] { aTopPanelLowerText});
            }
            else
            {
                theTopPanelLowerLabel.Text = aTopPanelLowerText;

                this.DSConfig_TrafodionToolTip.SetToolTip(theTopPanelLowerLabel, aTopPanelLowerText);
            }
        }

        public void EnableApplyButton()
        {
            this.dsApply_TrafodionButton.Enabled = ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER"); ;
        }

        private void dsApply_TrafodionButton_Click(object sender, EventArgs e)
        {
            ApplyChanges();
        }

        public void ApplyChanges()
        {
            try
            {
                bool updateSucceeded = true;
                NDCSDataSource updatedDataSource = null;

                if (this._generalConfig.CheckAndCommit() && this._defineSetConfig.CheckAndCommit() &&
                    this._cqdConfig.CheckAndCommit() && this._controlledTableConfig.CheckAndCommit())
                {
                    updatedDataSource = this._generalConfig.NdcsDatasource;
                }

                DataTable errorMessagesTable = updatedDataSource.Update();
                this.dsApply_TrafodionButton.Enabled = false;

                if (errorMessagesTable.Rows.Count > 0)
                {
                    if (errorMessagesTable.Rows.Count == 1)
                    {
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), errorMessagesTable.Rows[0][1] as string, Properties.Resources.UpdateDsException, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    else
                    {
                        TrafodionMultipleMessageDialog omd = new TrafodionMultipleMessageDialog("Failed to update one or more data source properties.", errorMessagesTable, System.Drawing.SystemIcons.Error);
                        omd.Show();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void dsCreate_TrafodionButton_Click(object sender, EventArgs e)
        {
            try
            {
            bool createSucceeded = true;
            NDCSDataSource newDataSource = null;

            if (this._generalConfig.CheckAndCommit() && this._defineSetConfig.CheckAndCommit() &&
                this._cqdConfig.CheckAndCommit() && this._controlledTableConfig.CheckAndCommit())
            {
                newDataSource = this._generalConfig.NdcsDatasource;
            }

            if (createSucceeded)
                newDataSource = this._generalConfig.NdcsDatasource;

            //Add the datasource
            newDataSource.NDCSSystem.AddDataSource(newDataSource);
            newDataSource.NDCSSystem.RefreshDataSources();
            this._datasourcesFolder.refreshTree();
            this._createForm.Close();

            string datasourceFullPath = this._datasourcesFolder.FullPath + "\\" + 
                (NDCSName.ExternalInternalFormSame(newDataSource.Name) ? NDCSName.InternalForm(newDataSource.Name) : NDCSName.ExternalForm(newDataSource.Name));

            this._connectivityTreeView.Select(this._connectivityTreeView.FindByFullPath(datasourceFullPath));
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void dsReload_TrafodionButton_Click(object sender, EventArgs e)
        {
            try
            {
                if (this._formType.Equals(DSConfigFormType.Create))
                {
                    NDCSDataSource dataSource = new NDCSDataSource(this._generalConfig.NdcsDatasource.NDCSSystem, "New Datasource");
                    dataSource.ResourceStats.connectInfo = true;
                    dataSource.ResourceStats.sessionSummary = true;

                    this._generalConfig.NdcsDatasource = dataSource;
                    this._defineSetConfig.NdcsDatasource = dataSource;
                    this._cqdConfig.NdcsDatasource = dataSource;
                    this._controlledTableConfig.NdcsDatasource = dataSource;

                    this._generalConfig.PopulateControls();
                    this._defineSetConfig.PopulateContols();
                    this._cqdConfig.PopulateControls();
                    this._controlledTableConfig.PopulateControls();
                }
                else
                {
                    //this._defineSetConfig.NdcsSystem;
                    this._generalConfig.NdcsDatasource.Refresh();
                    this._generalConfig.NdcsDatasource.PopulateDetail();

                    this._generalConfig.PopulateControls();
                    this._defineSetConfig.PopulateContols();
                    this._cqdConfig.PopulateControls();
                    this._controlledTableConfig.PopulateControls();

                    if (!this._formType.Equals(DSConfigFormType.CreateLike))
                        this.dsApply_TrafodionButton.Enabled = false;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

        }

        private void dsCancel_TrafodionButton_Click(object sender, EventArgs e)
        {
            this._createForm.Close();
        }
    }
}
