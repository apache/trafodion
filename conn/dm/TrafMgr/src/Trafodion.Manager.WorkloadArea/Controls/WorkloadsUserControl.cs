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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WorkloadsUserControl : UserControl, ICloneToWindow
    {
        #region private member variables

        private ConnectionDefinition _connectionDefinition;
        private MonitorWorkloadCanvas _monitorWorkloadCanvas;
        public delegate void UpdateStatus(object obj, DataProviderEventArgs e);
        WorkloadsUserControl _workloadsUserControlClone = null;
        SummaryCountsDataProvider _summaryCountsDataProvider = null;

        #endregion private member variables

        #region Public Properties

        public ConnectionDefinition TheConnectionDefinition
        {
            get { return _connectionDefinition; }
            set
            {
                if (_connectionDefinition != null && value != null && !_connectionDefinition.Equals(value))
                {
                    _wmsSummaryUserControl.Reset();
                }

                _connectionDefinition = value;

                _monitorWorkloadCanvas.ConnectionDefn = _connectionDefinition;
            }
        }

        public MonitorWorkloadCanvas MonitorWorkloadCanvas
        {
            get { return _monitorWorkloadCanvas; }            
        }

        #endregion Public Properties


        public WorkloadsUserControl()
        {
            InitializeComponent();
        }

        public WorkloadsUserControl(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();  
            _connectionDefinition = aConnectionDefinition;           
            _monitorWorkloadCanvas = new MonitorWorkloadCanvas(aConnectionDefinition);
            _monitorWorkloadCanvas.Dock = DockStyle.Fill;
            _workloadsGridPanel.Controls.Add(_monitorWorkloadCanvas);
            if(_monitorWorkloadCanvas.TheMonitorWorkloadWidget != null && _monitorWorkloadCanvas.TheMonitorWorkloadWidget.DataProvider !=null)
            {
                _monitorWorkloadCanvas.TheMonitorWorkloadWidget.DataProvider.OnNewDataArrived += InvokeLiveView_NewDataArrived;
                _monitorWorkloadCanvas.TheMonitorWorkloadWidget.DataProvider.OnFetchingData += DataProvider_OnFetchingData;
            }

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.OpenCommand = "WMSOPEN";
            dbConfig.CloseCommand = "WMSCLOSE";
            dbConfig.ConnectionDefinition = aConnectionDefinition;

            _summaryCountsDataProvider = new SummaryCountsDataProvider(aConnectionDefinition, dbConfig);
            _summaryCountsDataProvider.OnNewDataArrived += InvokeSummary_NewDataArrived;
        }

        void DataProvider_OnFetchingData(object sender, DataProviderEventArgs e)
        {
            //kick off a fetch of the summary counts everytime the workload monitor refreshes
            _summaryCountsDataProvider.Start();
        }

        void InvokeSummary_NewDataArrived(object sender, DataProviderEventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(_summaryCountsDataProvider_OnNewDataArrived), new object[] { sender, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        void _summaryCountsDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            _wmsSummaryUserControl.LoadServiceAndPlatformCounters(_summaryCountsDataProvider);
        }

        private void InvokeLiveView_NewDataArrived(object obj, DataProviderEventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(LiveView_NewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        void LiveView_NewDataArrived(object sender, Trafodion.Manager.UniversalWidget.DataProviderEventArgs e)
        {
            MonitorWorkloadDataProvider workloaDataProvider = (MonitorWorkloadDataProvider)_monitorWorkloadCanvas.TheMonitorWorkloadWidget.DataProvider;
            _wmsSummaryUserControl.LoadCounters(workloaDataProvider);
        }

        /// <summary>
        /// private dispose method, which manually cleanup all of the report defintions.
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_monitorWorkloadCanvas.TheMonitorWorkloadWidget != null && _monitorWorkloadCanvas.TheMonitorWorkloadWidget.DataProvider != null)
                {
                    _monitorWorkloadCanvas.TheMonitorWorkloadWidget.DataProvider.OnNewDataArrived -= InvokeLiveView_NewDataArrived;
                    _monitorWorkloadCanvas.TheMonitorWorkloadWidget.DataProvider.OnFetchingData -= DataProvider_OnFetchingData;
                }
                if (_summaryCountsDataProvider != null)
                {
                    _summaryCountsDataProvider.OnNewDataArrived -= InvokeSummary_NewDataArrived;
                }
                _monitorWorkloadCanvas.Dispose();
            }
        }

        #region ICloneToWindow Members
        public Control Clone()
        {
            _workloadsUserControlClone = new WorkloadsUserControl(_connectionDefinition);
            _workloadsUserControlClone.MonitorWorkloadCanvas.LoadQueriesToTriageSpaceEvent += MonitorWorkloadCanvas_LoadQueriesToTriageSpaceEvent;
            _workloadsUserControlClone.MonitorWorkloadCanvas.GetSessionEvent += MonitorWorkloadCanvas_GetSessionEvent;
            _workloadsUserControlClone.MonitorWorkloadCanvas.TheMonitorWorkloadWidget.StartDataProvider();
            return _workloadsUserControlClone;
        }

        private void MonitorWorkloadCanvas_LoadQueriesToTriageSpaceEvent()
        {
            TrafodionIGrid theGrid = _workloadsUserControlClone.MonitorWorkloadCanvas.MonitorWorkloadIGrid;
            if (theGrid.SelectedRowIndexes.Count > 0)
            {
                TriageSpaceUserControl _theTriageSpaceUserControl = new TriageSpaceUserControl(TheConnectionDefinition);
                _theTriageSpaceUserControl.CreateControl();
                _theTriageSpaceUserControl.Dock = DockStyle.Fill;
                _theTriageSpaceUserControl.BackColor = Color.WhiteSmoke;                
                WindowsManager.PutInWindow(new Size(1087, 625), _theTriageSpaceUserControl, Properties.Resources.TriageSpace, TheConnectionDefinition);
                _theTriageSpaceUserControl.loadSelectedLiveViewQueries(theGrid);
            }
        }

        private void MonitorWorkloadCanvas_GetSessionEvent()
        {
            TrafodionIGrid theGrid = _workloadsUserControlClone.MonitorWorkloadCanvas.MonitorWorkloadIGrid;
            if (theGrid.SelectedRowIndexes.Count > 0)
            {
                TriageSpaceUserControl _theTriageSpaceUserControl = new TriageSpaceUserControl(TheConnectionDefinition);
                _theTriageSpaceUserControl.CreateControl();
                _theTriageSpaceUserControl.Dock = DockStyle.Fill;
                _theTriageSpaceUserControl.BackColor = Color.WhiteSmoke;                
                WindowsManager.PutInWindow(new Size(1087, 625), _theTriageSpaceUserControl, Properties.Resources.TriageSpace, TheConnectionDefinition);
                _theTriageSpaceUserControl.GetSession(theGrid);
            }
        }

        public string WindowTitle
        {
            get { return Properties.Resources.LiveWorkloads; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
        }

        #endregion ICloneToWindow Members
    }
}
