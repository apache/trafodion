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
using System.Drawing;
using System.Windows.Forms;
using System.Text.RegularExpressions;
using Trafodion.Manager.ConnectivityArea.Controls.Tree;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// User control that's displayed in the right pane
    /// </summary>
    public partial class ConnectivityAreaErrorUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        private ConnectivityTreeView _connectivityTreeView;
        private NavigationTreeNode _activeTreeNode;

        // At System node, in Error tab - 
        private ConnectivityAreaErrorTabUserControl _theErrorTabUserControl;
        private TrafodionTabPage _theErrorSubTabPage;

        #endregion Fields

        # region Properties

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
        public NavigationTreeNode ActiveTreeNode
        {
            get { return _activeTreeNode; }
            set { _activeTreeNode = value; }
        }

        #endregion Properties

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            ConnectivityAreaErrorUserControl theClonedConnectivityAreaErrorUserControl =
                new ConnectivityAreaErrorUserControl(this._connectivityTreeView);

            theClonedConnectivityAreaErrorUserControl.ActiveTreeNode = this.ActiveTreeNode;
            theClonedConnectivityAreaErrorUserControl.SetLabels(theTopPanelLowerLabel.Text, theTopPanelLowerLabelRight.Text);

            if (theClonedConnectivityAreaErrorUserControl.ActiveTreeNode is TreeNode)
            {
                theClonedConnectivityAreaErrorUserControl.TabSelected();
            }
            //else if (theClonedConnectivityAreaMonitoringUserControl is Exception)
            //{
            //    // raise the exception here
            //}

            theClonedConnectivityAreaErrorUserControl.Size = Size;

            return theClonedConnectivityAreaErrorUserControl;

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return "Error" + " | " + theTopPanelLowerLabel.Text; }
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
        public ConnectivityAreaErrorUserControl(ConnectivityTreeView theConnectivityTreeView)
        {
            InitializeComponent();

            if (theConnectivityTreeView != null)
            {
                this._connectivityTreeView = theConnectivityTreeView;
            }

            // Initially non-blank to make them easy to see in the designer so set them empty now
            theTopPanelLowerLabel.Text = "";
            theTopPanelLowerLabelRight.Text = "";
        }

        /// <summary>
        /// 
        /// </summary>
        public void TabSelected()
        {
            if (Trafodion.Manager.Framework.Utilities.InUnselectedTabPage(this))
            {
                return;
            }

            this.ConnectivityTreeControlSelected(this._activeTreeNode);
        }

        /// <summary>
        /// Handles clicks to the Sub TabControl tabs
        /// </summary>
        /// 
        void ConnectivityMonitoring_PropogateTabSelectionChange(object sender, EventArgs e)
        {
        }

        
        /// <summary>
        /// Handles clicks to the  Connectivity Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        public void ConnectivityTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
            string theTopPanelUpperText = (theConnectionDefinition != null) ? theConnectionDefinition.Description : "";
            string theTopPanelLowerText = aNavigationTreeNode.LongerDescription;
            string theTopPanelLowerLabelRightText = aNavigationTreeNode.LongerDescriptionLink;
            object theTopPanelLowerLabelRightTextTag = aNavigationTreeNode.LongerDescriptionLinkTag;

            if (aNavigationTreeNode is ConnectivitySystemFolder)
            {
                // System Folder has been selected, if logged in, show the Connectivity Monitoring tab 
                //with both "HPDCS Services" and "Datasources" Summary tabs only.
                theTopPanelUpperText = (theConnectionDefinition != null) ? theConnectionDefinition.Description : "";
                theTopPanelLowerText = aNavigationTreeNode.LongerDescription + " - " + Properties.Resources.AreaName;

                this._connectivityErrorSubTabControl.TabPages.Clear();

                ConnectivitySystemFolder theSystemFolder = (ConnectivitySystemFolder)aNavigationTreeNode;

                //if (this._theErrorTabUserControl == null)
                //{
                string exceptionMessage = Environment.NewLine;
                if (theSystemFolder.ConnectivitySystem.TestException != null)
                {
                    exceptionMessage = "Exception : " + theSystemFolder.ConnectivitySystem.TestException.Message;
                    if (exceptionMessage.Contains("syntax error occurred") &&
                        exceptionMessage.ToUpper().Contains("CMDOPEN"))
                    {
                        // Do not display the error message if it just reported CMDOPEN not supported
                        exceptionMessage = Environment.NewLine;
                    }
                    else
                    {
                        exceptionMessage = string.Format("{0}{1}{2}", Environment.NewLine + Environment.NewLine, exceptionMessage, Environment.NewLine + Environment.NewLine);
                    }
                }

                //string notSupportedMessage = Properties.Resources.ConnectivityNotSupportedWarning;
                //if (theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.R25)
                //{
                    string notSupportedMessage = string.Format(Properties.Resources.ConnectivityNotSupportedException, theConnectionDefinition.Name, exceptionMessage, "\n");
                //}

                this._theErrorTabUserControl =
                    new ConnectivityAreaErrorTabUserControl(theSystemFolder.ConnectivitySystem,
                            notSupportedMessage);
                //}
                AddToTabControl(this._theErrorTabUserControl, "Connectivity");
            }
                      
            SetLabels(theTopPanelLowerText, theTopPanelLowerLabelRightText);
        }


        /// <summary>
        /// Helper method to add a control into a new tab into the right pane
        /// </summary>
        /// <param name="aUserControl"></param>
        private void AddNewTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabPage theTabPage = new TrafodionTabPage(aTabText);
            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);
            //TheConnectivityAreaTabControl.TabPages.Add(theTabPage);
            AddNewTabControl(theTabPage);
        }

        private void AddNewTabControl(TrafodionTabPage aTabPage)
        {
            if (TheConnectivityAreaTabControl == null)
                TheConnectivityAreaTabControl = new TrafodionTabControl();

            aTabPage.Size = new System.Drawing.Size(1, 1); //To prevent a small square from being shown until the tab page resizes
            TheConnectivityAreaTabControl.TabPages.Add(aTabPage);
        }

        /// <summary>
        /// Helper method to add controls into a tab into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aUserControl"></param>
        /// <param name="aTabText"></param>
        private void AddToTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabControl MonitoringSubTabControl = new TrafodionTabControl();

            // Create the tab page with the user control dock filled
            TrafodionTabPage theTabPage = new TrafodionTabPage(aTabText);
            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);
            MonitoringSubTabControl.TabPages.Add(theTabPage);
            TheConnectivityAreaTabControl = MonitoringSubTabControl;
        }

        /// <summary>
        /// Helper method to add a tab page into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aTabPage"></param>
        private void AddToTabControl(TrafodionTabPage aTabPage)
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
                return _connectivityErrorSubTabControl;
            }
            set
            {
                _ConnectivityMainBodyPanel.Controls.Clear();
                _connectivityErrorSubTabControl = value;
                _connectivityErrorSubTabControl.Dock = DockStyle.Fill;
                _ConnectivityMainBodyPanel.Controls.Add(_connectivityErrorSubTabControl);
            }
        }

        private delegate void SetLabelsDelegate(string aTopPanelLowerText, string aTopPanelLowerLabelRightText);
        private void SetLabels(string aTopPanelLowerText, string aTopPanelLowerLabelRightText)
        {
            if (InvokeRequired)
            {
                Invoke(new SetLabelsDelegate(SetLabels), new object[] { aTopPanelLowerText, aTopPanelLowerLabelRightText });
            }
            else
            {
                theTopPanelLowerLabel.Text = aTopPanelLowerText;

                theTopPanelLowerLabelRight.Text = aTopPanelLowerLabelRightText;

            }
        }

        private void _refreshButton_Click(object sender, EventArgs e)
        {

        }
    }
}
