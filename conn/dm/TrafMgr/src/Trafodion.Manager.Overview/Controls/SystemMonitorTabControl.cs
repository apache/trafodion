//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class SystemMonitorTabControl : UserControl, IDataDisplayControl
    {

        public delegate void OnShowOffenderClick(ShowOffenderEventArgs args);
        public event OnShowOffenderClick OnShowOffenderClickImpl;

        public delegate void OnShowSQLOffenderClick();
        public event OnShowSQLOffenderClick OnShowSQLOffenderClickImpl;

        private ConnectionDefinition _theConnectionDefinition = null;
        private DynamicSummaryControl _theDynamicSummaryControl = null;
        private OverallSummaryControl _theOverallSummaryControl = null;
        TrafodionTabControl _theMonitoringTabCtl = new TrafodionTabControl();
        TrafodionTabPage _theRealTimeMonitoringTabPage = new TrafodionTabPage("Real Time");
        TrafodionTabPage _theTimelineMonitoringTabPage = new TrafodionTabPage("Timeline");

     
        private DataProvider _theDataProvider = null;
        private UniversalWidgetConfig _theWidgetConfig = null;
        private IDataDisplayHandler _theDataDisplayHandler = null;
        List<SystemMetricModel.SystemMetrics> _theOverallSummaryMetrics = null;
        bool _theShowHealthState=false;
        

        /// <summary>
        /// Property: The connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// Property: DataProvider - the data provider used by this widget
        /// </summary>
        public DataProvider DataProvider
        {
            get { return _theDataProvider; }
            set { _theDataProvider = value; }
        }

        /// <summary>
        /// Proerpty: UniversalWidgetConfiguration - the configuration of the widget
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _theWidgetConfig; }
            set { _theWidgetConfig = value; }
        }

        /// <summary>
        /// Property: DataDisplayHandler - the data display handler
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _theDataDisplayHandler; }
            set { _theDataDisplayHandler = value; }
        }

        /// <summary>
        /// Property: DataDisplayHandler - not used at this time
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        public OverallSummaryControl OverallSummary
        {
            get { return _theOverallSummaryControl; }
            set { _theOverallSummaryControl = value; }
        }


        public DynamicSummaryControl DynamicSummaryControl
        {
            get { return _theDynamicSummaryControl; }
            set { _theDynamicSummaryControl = value; }
        }



        public TrafodionTabControl TheMonitoringTabCtl
        {
            get { return _theMonitoringTabCtl; }
            set { _theMonitoringTabCtl = value; }
        }

        /// <summary>
        /// Persistent configruation - this is not used. But, for need to be here to fulfill the requirement of the interface
        /// </summary>
        public void PersistConfiguration()
        {
        }

        public SystemMonitorTabControl(ConnectionDefinition aConnectionDefinition,List<SystemMetricModel.SystemMetrics> aOverallSummaryMetrics, bool aShowHealthState)
        {
            InitializeComponent();
            this._theConnectionDefinition = aConnectionDefinition;
            _theOverallSummaryMetrics = aOverallSummaryMetrics;
            _theShowHealthState=aShowHealthState;
            Init();
        }



        private void Init()        
        {

            //Add realtime system monitoring control
            if (_theOverallSummaryControl == null)
            {
                _theOverallSummaryControl = new OverallSummaryControl(_theConnectionDefinition,_theOverallSummaryMetrics,_theShowHealthState);
                _theOverallSummaryControl.CreateControl();
                _theOverallSummaryControl.Dock = DockStyle.Fill;
                _theOverallSummaryControl.BackColor = Color.WhiteSmoke;
                _theOverallSummaryControl.OnShowOffenderClickImpl += _theOverallSummaryControl_OnShowOffenderClickImpl;
                _theOverallSummaryControl.OnShowSQLOffenderClickImpl += _theOverallSummaryControl_OnShowSQLOffenderClickImpl;
                _theOverallSummaryControl.OnSelectedTseMetricChanged += _theOverallSummaryControl_OnSelectedTseMetricChanged;
                _theRealTimeMonitoringTabPage.Controls.Clear();
                _theRealTimeMonitoringTabPage.Controls.Add(_theOverallSummaryControl);

            }
            else if (_theOverallSummaryControl.ConnectionDefn != ConnectionDefn && ConnectionDefn != null)
            {
                _theOverallSummaryControl.OnShowOffenderClickImpl -= _theOverallSummaryControl_OnShowOffenderClickImpl;
                _theOverallSummaryControl.OnSelectedTseMetricChanged -= _theOverallSummaryControl_OnSelectedTseMetricChanged;
                _theOverallSummaryControl.Dispose();
                _theOverallSummaryControl = new OverallSummaryControl(_theConnectionDefinition, _theOverallSummaryMetrics, _theShowHealthState);
                _theOverallSummaryControl.CreateControl();
                _theOverallSummaryControl.Dock = DockStyle.Fill;
                _theOverallSummaryControl.BackColor = Color.WhiteSmoke;
                _theOverallSummaryControl.OnShowOffenderClickImpl += _theOverallSummaryControl_OnShowOffenderClickImpl;
                _theOverallSummaryControl.OnShowSQLOffenderClickImpl += _theOverallSummaryControl_OnShowSQLOffenderClickImpl;
                _theOverallSummaryControl.OnSelectedTseMetricChanged += _theOverallSummaryControl_OnSelectedTseMetricChanged;
                _theRealTimeMonitoringTabPage.Controls.Clear();
                _theRealTimeMonitoringTabPage.Controls.Add(_theOverallSummaryControl);
            }
            else if (!this._theRealTimeMonitoringTabPage.Controls.Contains(_theOverallSummaryControl))
            {
                _theRealTimeMonitoringTabPage.Controls.Add(_theOverallSummaryControl);
            }

            if (!this._theMonitoringTabCtl.TabPages.Contains(_theRealTimeMonitoringTabPage))
            {
                _theMonitoringTabCtl.TabPages.Add(_theRealTimeMonitoringTabPage);
            }

            //Add history system monitoring control
            if (_theDynamicSummaryControl == null)
            {
                _theDynamicSummaryControl = new DynamicSummaryControl(ConnectionDefn, _theOverallSummaryMetrics);
                _theDynamicSummaryControl.CreateControl();
                _theDynamicSummaryControl.Dock = DockStyle.Fill;
                _theDynamicSummaryControl.BackColor = Color.WhiteSmoke;
                _theDynamicSummaryControl.OnSelectedTseMetricChanged += _theDynamicSummaryControl_OnSelectedTseMetricChanged;
                _theTimelineMonitoringTabPage.Controls.Clear();
                _theTimelineMonitoringTabPage.Controls.Add(_theDynamicSummaryControl);
            }
            else if (_theDynamicSummaryControl.ConnectionDefn != ConnectionDefn)
            {
                _theDynamicSummaryControl.OnSelectedTseMetricChanged -= _theDynamicSummaryControl_OnSelectedTseMetricChanged;
                _theDynamicSummaryControl.Dispose();
                _theDynamicSummaryControl = new DynamicSummaryControl(ConnectionDefn, _theOverallSummaryMetrics);
                _theDynamicSummaryControl.CreateControl();
                _theDynamicSummaryControl.Dock = DockStyle.Fill;
                _theDynamicSummaryControl.BackColor = Color.WhiteSmoke;
                _theDynamicSummaryControl.OnSelectedTseMetricChanged += _theDynamicSummaryControl_OnSelectedTseMetricChanged;
                _theTimelineMonitoringTabPage.Controls.Clear();
                _theTimelineMonitoringTabPage.Controls.Add(_theDynamicSummaryControl);

            }
            else if (!this._theTimelineMonitoringTabPage.Controls.Contains(_theDynamicSummaryControl))
            {
                _theTimelineMonitoringTabPage.Controls.Add(_theDynamicSummaryControl);
            }

            if (!this._theMonitoringTabCtl.TabPages.Contains(_theTimelineMonitoringTabPage))
            {
                _theMonitoringTabCtl.TabPages.Add(_theTimelineMonitoringTabPage);
            }
            _theMonitoringTabCtl.Dock = DockStyle.Fill;

            _theTrafodionPanel.Controls.Add(_theMonitoringTabCtl);
        }

        void _theDynamicSummaryControl_OnSelectedTseMetricChanged(object sender, EventArgs e)
        {
            if (_theOverallSummaryControl != null)
            {
                _theOverallSummaryControl.ResetTseSkew();
            }
        }

        void _theOverallSummaryControl_OnSelectedTseMetricChanged(object sender, EventArgs e)
        {
            if (_theDynamicSummaryControl != null)
            {
                _theDynamicSummaryControl.ResetTseSkew();
            }
        }

        void _theOverallSummaryControl_OnShowSQLOffenderClickImpl()
        {
            if (OnShowSQLOffenderClickImpl != null)
            {
                OnShowSQLOffenderClickImpl();
            }
        }

        void _theOverallSummaryControl_OnShowOffenderClickImpl(WorkloadArea.Controls.ShowOffenderEventArgs args)
        {
            if (OnShowOffenderClickImpl != null)
            {
                OnShowOffenderClickImpl(args);
            }
        }

        public void ResetLayout()
        {
             _theDynamicSummaryControl.ResetLayout();
             _theOverallSummaryControl.ResetLayout();
        }

        public string LockLayout()
        {
            _theDynamicSummaryControl.LockLayout();           
            return _theOverallSummaryControl.LockLayout();
           
        }

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                _theOverallSummaryControl.OnShowOffenderClickImpl -= _theOverallSummaryControl_OnShowOffenderClickImpl;
                _theOverallSummaryControl.OnShowSQLOffenderClickImpl -= _theOverallSummaryControl_OnShowSQLOffenderClickImpl;
                _theOverallSummaryControl.OnSelectedTseMetricChanged -= _theOverallSummaryControl_OnSelectedTseMetricChanged;
                _theDynamicSummaryControl.OnSelectedTseMetricChanged -= _theDynamicSummaryControl_OnSelectedTseMetricChanged;
                _theOverallSummaryControl.Dispose();
                _theDynamicSummaryControl.Dispose();
            }
        }
    }
}
