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
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using ZedGraph;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    public partial class ChartControl : UserControl
    {
        private ZedGraph.ZedGraphControl zgc = new ZedGraph.ZedGraphControl();
        private UniversalWidgetConfig _theConfig;
        ChartRenderer _theChartRenderer;
        DataProvider _theDataProvider = null;
        ChartConfig _theChartConfig = null;
        private delegate void HandleEvent(object obj, DataProviderEventArgs e); 

        public DataProvider DataProvider
        {
            get { return _theDataProvider; }
            set
            {
                if (_theDataProvider != null)
                {
                    RemoveHandlers();
                }
                _theDataProvider = value;
                AddHandlers();
            }
        }

        public ChartControl()
        {
            InitializeComponent();            
        }

        public ChartControl(UniversalWidgetConfig aConfig)
        {
            InitializeComponent();
            ChartConfiguration = aConfig.ChartConfig;
        }

        /// <summary>
        /// Associate the configuration to the chart. 
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfig
        {
            get { return _theConfig; }
            set
            {
                _theConfig = value;
                InitializeGraphControl();
            }
        }


        /// <summary>
        /// Associate the configuration to the chart. 
        /// </summary>
        public ChartConfig ChartConfiguration
        {
            get 
            { 
                return _theChartConfig; 
            
            }
            set
            {
                _theChartConfig = value;
            }
        }

        /// <summary>
        /// Re-draws the graph in the UI
        /// </summary>
        private void Populate()
        {
            _theGraphPanel.Controls.Clear();

            zgc.Dock = DockStyle.Fill;            
            GraphPane graphPane = zgc.GraphPane;

            _theChartRenderer.RenderChart(graphPane, _theConfig, _theDataProvider);

            // Calculate the Axis Scale Ranges
            zgc.AxisChange();

            _theGraphPanel.Controls.Add(zgc);
        }

        private void InvokeHandleError(Object obj, EventArgs e)
        {
            if (IsHandleCreated)
            {
                Invoke(new HandleEvent(HandleError), new object[] { obj, e });
            }
        }

        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            if (IsHandleCreated)
            {
                Invoke(new HandleEvent(HandleNewDataArrived), new object[] { obj, e });
            }
        }

        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            if (IsHandleCreated)
            {
                Invoke(new HandleEvent(HandleFetchingData), new object[] { obj, e });
            }
        }

        private void HandleError(Object obj, EventArgs e)
        {
            //Update status bar with exception
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            if (_theConfig.ShowChart)
            {
                //Populate the chart
                Populate();
            }
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
            //TODO: show the status bar 
        }

        private void InitializeGraphControl()
        {
            //Set the appropriate renderer
            _theChartRenderer = ChartRendererFactory.GetChartRenderer(_theConfig);
        }

        private void AddHandlers()
        {
            if (_theDataProvider != null)
            {
                //Associate the event handlers
                _theDataProvider.OnErrorEncountered += InvokeHandleError;
                _theDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _theDataProvider.OnFetchingData += InvokeHandleFetchingData;
            }
        }
        private void RemoveHandlers()
        {
            if (_theDataProvider != null)
            {
                //Remove the event handlers
                _theDataProvider.OnErrorEncountered -= InvokeHandleError;
                _theDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theDataProvider.OnFetchingData -= InvokeHandleFetchingData;
            }
        }
        //Do the dispose
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Remove the event handlers
                this.RemoveHandlers();
            }
        }

    } 
}
