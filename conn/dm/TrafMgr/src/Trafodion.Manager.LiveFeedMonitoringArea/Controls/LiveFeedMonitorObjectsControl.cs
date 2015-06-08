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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.LiveFeedMonitoringArea.Controls.Tree;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    public partial class LiveFeedMonitorObjectsControl : TrafodionRightPaneControl
    {   
        #region Fields

        private MonitorLiveFeedConnectorsCanvas _theMonitorLiveFeedConnectorsCanvas = null;
        private TestLiveFeedConnectorCanvas _theTestLiveFeedConnectorCanvas = null;

        #endregion Fields

        #region public properties

        /// <summary>
        /// Property: TheLiveFeedMonitorNavigator
        /// </summary>
        public LiveFeedMonitorNavigator TheLiveFeedMonitorNavigator
        {
            get { return TheNavigationUserControl as LiveFeedMonitorNavigator;}
        }

        /// <summary>
        /// Property: TheLiveFeedMonitorTreeView
        /// </summary>
        public LiveFeedMonitorTreeView TheLiveFeedMonitorTreeView
        {
            get { return TheNavigationUserControl.TheNavigationTreeView as LiveFeedMonitorTreeView; }
        }

        /// <summary>
        /// Property: MonitorLiveFeedConnectorsCanvas
        /// </summary>
        public MonitorLiveFeedConnectorsCanvas MonitorLiveFeedConnectorsCanvas
        {
            get { return _theMonitorLiveFeedConnectorsCanvas; }
        }

        /// <summary>
        /// Property: TestLiveFeedConnectorCanvas
        /// </summary>
        public TestLiveFeedConnectorCanvas TestLiveFeedConnectorCanvas
        {
            get { return _theTestLiveFeedConnectorCanvas; }
        }

        #endregion public properties

        #region Constructors

        /// <summary>
        /// The constructor
        /// </summary>
        public LiveFeedMonitorObjectsControl()
            :base()
        {
            InitializeComponent();
            ConnectionDefinition.Changed += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
        }

        /// <summary>
        /// clone constructor
        /// </summary>
        /// <param name="aLiveFeedMonitorNavigator"></param>
        public LiveFeedMonitorObjectsControl(LiveFeedMonitorNavigator aLiveFeedMonitorNavigator)
            : base(aLiveFeedMonitorNavigator)
        {
            InitializeComponent();
            ConnectionDefinition.Changed += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
        }

        /// <summary>
        /// copy constructor
        /// </summary>
        /// <param name="anLiveFeedMonitorObjectsControl"></param>
        public LiveFeedMonitorObjectsControl(LiveFeedMonitorObjectsControl anLiveFeedMonitorObjectsControl)
        {
            InitializeComponent();

            if (anLiveFeedMonitorObjectsControl.MonitorLiveFeedConnectorsCanvas != null)
            {
                _theMonitorLiveFeedConnectorsCanvas = new MonitorLiveFeedConnectorsCanvas(anLiveFeedMonitorObjectsControl.MonitorLiveFeedConnectorsCanvas);
            }
            else
            {
                _theTestLiveFeedConnectorCanvas = new TestLiveFeedConnectorCanvas(anLiveFeedMonitorObjectsControl.TestLiveFeedConnectorCanvas);
            }

            ConnectionDefinition.Changed += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
        }

        #endregion Constructors

        #region Public methods
        /// <summary>
        /// To handle navigation tree node select event
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        public override void HandleNavigationTreeNodeSelect(NavigationTreeNode aNavigationTreeNode)
        {

            try
            {
                ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;

                //if ((theConnectionDefinition != null) && (theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded))
                //{
                //    AddControl(new FixSystemUserControl(theConnectionDefinition));
                //}
                //else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionsFolder)
                //{
                //    AddControl(new MySystemsUserControl(true));
                //}

                //else 
                if (aNavigationTreeNode is SystemConnectionFolder || aNavigationTreeNode is MonitorConnectorsFolder)
                {
                    if (null == _theMonitorLiveFeedConnectorsCanvas || !_theMonitorLiveFeedConnectorsCanvas.ConnectionDefn.Name.Equals(theConnectionDefinition.Name))
                    {
                        try
                        {
                            if (_theMonitorLiveFeedConnectorsCanvas != null)
                            {
                                _theMonitorLiveFeedConnectorsCanvas.Dispose();
                            }

                            _theMonitorLiveFeedConnectorsCanvas = new MonitorLiveFeedConnectorsCanvas(theConnectionDefinition, "Monitor Connectors");
                            _theMonitorLiveFeedConnectorsCanvas.Start();

                        }
                        catch (Exception ex)
                        {
                            AddControl(new Trafodion.Manager.Framework.Controls.TrafodionDisplayMessageUserControl("Unable to connect to LiveFeed broker on the system: " + ex.Message));
                            return;
                        }
                    }
                    else
                    {
                        _theMonitorLiveFeedConnectorsCanvas.Start();
                    }
                    AddControl(_theMonitorLiveFeedConnectorsCanvas);
                }
                else
                if (aNavigationTreeNode is TestingFolder)
                {
                    if (null == _theTestLiveFeedConnectorCanvas || !_theTestLiveFeedConnectorCanvas.ConnectionDefn.Name.Equals(theConnectionDefinition.Name))
                    {
                        if (_theTestLiveFeedConnectorCanvas != null)
                        {
                            _theTestLiveFeedConnectorCanvas.Stop(true);
                        }

                        try
                        {
                            _theTestLiveFeedConnectorCanvas = new TestLiveFeedConnectorCanvas(theConnectionDefinition, "LiveFeed Testing");
                        }
                        catch (Exception ex)
                        {
                            AddControl(new Trafodion.Manager.Framework.Controls.TrafodionDisplayMessageUserControl("Unable to connect to LiveFeed broker on the system: " + ex.Message));
                            return;
                        }
                    }
                    else
                    {
                        if (!_theTestLiveFeedConnectorCanvas.Started)
                            _theTestLiveFeedConnectorCanvas.Start();
                    }
                    AddControl(_theTestLiveFeedConnectorCanvas);
                }
            }
            catch (Exception anException)
            {
                // TODO: Deal with exception if object loaded into the tabcontrol is null
            }
        }

        public override TrafodionRightPaneControl DoClone()
        {
            return new LiveFeedMonitorObjectsControl(this);
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
                Cleanup();
            }
        }

        /// <summary>
        /// Event handler for connection definition changed event
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        private void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason == ConnectionDefinition.Reason.Removed)
            {
                Cleanup();
            }
        }

        /// <summary>
        /// final cleanup
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Cleanup()
        {
            if (_theMonitorLiveFeedConnectorsCanvas != null)
                _theMonitorLiveFeedConnectorsCanvas.Dispose();
            if (_theTestLiveFeedConnectorCanvas != null)
                _theTestLiveFeedConnectorCanvas.Dispose();
        }

        #endregion Private methods
    }
}
