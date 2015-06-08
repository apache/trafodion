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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using TenTec.Windows.iGridLib;


namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// This class is a wrapper class to display the Live View and the Triage 
    /// Space as two tabs.
    /// </summary>
    /// 
    public partial class TabbedWorkloadUserControlWrapper : UserControl
    {
        #region private member variables
        private ConnectionDefinition _connectionDefinition;
        private WorkloadsUserControl _theWorkloadsUserControl;
        private TriageSpaceUserControl _theTriageSpaceUserControl;
        private OffenderWorkloadCanvas _offenderWorkloadCanvas;

        #endregion private member variables

        #region Public Properties

        public ConnectionDefinition TheConnectionDefinition
        {
            get { return _connectionDefinition; }
            set
            {
                _connectionDefinition = value;
                _theWorkloadsUserControl.TheConnectionDefinition = value;
                if (_theTriageSpaceUserControl == null)
                {
                    _theTriageSpaceUserControl = new TriageSpaceUserControl(_connectionDefinition);
                    _theTriageSpaceUserControl.Dock = DockStyle.Fill;
                    _theTriageTab.Controls.Add(_theTriageSpaceUserControl);
                }
                else
                {
                    _theTriageSpaceUserControl.TheConnectionDefinition = value;
                }
                if (_offenderWorkloadCanvas == null)
                {
                    _offenderWorkloadCanvas = new OffenderWorkloadCanvas(_connectionDefinition);
                    _offenderWorkloadCanvas.Dock = DockStyle.Fill;
                    _theSystemOffenderTabPage.Controls.Add(_offenderWorkloadCanvas);
                }
                else
                {
                    _offenderWorkloadCanvas.ConnectionDefinition = value;
                }
            }
        }

        public OffenderWorkloadCanvas OffenderWorkloadCanvas
        {
            get { return _offenderWorkloadCanvas; }
        }

        public TrafodionTabControl TheTabControl
        {
            get { return _theTabControl; }
        }

        #endregion Public Properties

        public TabbedWorkloadUserControlWrapper()
        {
            InitializeComponent();
        }
        public TabbedWorkloadUserControlWrapper(ConnectionDefinition aConnectionDefinition) : this()
        {
            _connectionDefinition = aConnectionDefinition;
            _theWorkloadsUserControl = new WorkloadsUserControl(aConnectionDefinition);
            _theWorkloadsUserControl.CreateControl();
            _theWorkloadsUserControl.Dock = DockStyle.Fill;
            _theLiveViewTab.Controls.Add(_theWorkloadsUserControl);
            _theWorkloadsUserControl.MonitorWorkloadCanvas.TheMonitorWorkloadWidget.StartDataProvider();

            _theTriageSpaceUserControl = new TriageSpaceUserControl(aConnectionDefinition);
            _theTriageSpaceUserControl.CreateControl();
            _theTriageSpaceUserControl.Dock = DockStyle.Fill;
            _theTriageTab.Controls.Add(_theTriageSpaceUserControl);

            _offenderWorkloadCanvas = new OffenderWorkloadCanvas(aConnectionDefinition);
            _offenderWorkloadCanvas.CreateControl();
            _offenderWorkloadCanvas.Dock = DockStyle.Fill;
            _theSystemOffenderTabPage.Controls.Add(_offenderWorkloadCanvas);
            
            _theWorkloadsUserControl.MonitorWorkloadCanvas.LoadQueriesToTriageSpaceEvent += 
                new MonitorWorkloadCanvas.loadQueriesToTriageSpace(loadQueriesToTriageSpace);
            _theWorkloadsUserControl.MonitorWorkloadCanvas.GetSessionEvent += new MonitorWorkloadCanvas.GetSession(GetSession);
        }

        void _theLiveViewTab_SwitchToTriage() 
        {
            _theTabControl.SelectedTab = _theTriageTab;
        }

        private void loadQueriesToTriageSpace() 
        {
            TrafodionIGrid theGrid = this._theWorkloadsUserControl.MonitorWorkloadCanvas.MonitorWorkloadIGrid;
            if (theGrid.SelectedRowIndexes.Count > 0)
            {
                _theTriageSpaceUserControl.loadSelectedLiveViewQueries(theGrid);

                //Focus on Triage Tab
                _theTabControl.SelectedTab = _theTriageTab;
            }
        }

        private void GetSession() 
        {
            TrafodionIGrid theGrid = this._theWorkloadsUserControl.MonitorWorkloadCanvas.MonitorWorkloadIGrid;
            if (theGrid.SelectedRowIndexes.Count > 0)
            {
                _theTriageSpaceUserControl.GetSession(theGrid);
                _theTabControl.SelectedTab = _theTriageTab;
            }
        }

        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_theWorkloadsUserControl != null)
                {
                    _theWorkloadsUserControl.Dispose();
                }
                if (_theTriageSpaceUserControl != null)
                {
                    _theTriageSpaceUserControl.Dispose();
                }
                if (_offenderWorkloadCanvas != null)
                {
                    _offenderWorkloadCanvas.Dispose();
                }
            }
        }
    }
}
