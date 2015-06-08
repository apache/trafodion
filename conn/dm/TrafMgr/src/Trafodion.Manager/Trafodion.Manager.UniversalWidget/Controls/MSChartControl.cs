// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Collections;
//using ZedGraph;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;

using System.Windows.Forms.DataVisualization.Charting;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    public partial class MSChartControl : UserControl
    {
        private UniversalWidgetConfig _theConfig;
        ChartRenderer _theChartRenderer;
        DataProvider _theDataProvider = null;
        ChartConfig _theChartConfig = null;
        private delegate void HandleEvent(object obj, DataProviderEventArgs e);
        public delegate void HandleMouseClickEvent(object sender, MouseEventArgs e);
        private HandleMouseClickEvent _theMouseClickHandler = null;

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

        public System.Windows.Forms.DataVisualization.Charting.Chart TheChart
        {
            get { return _theChart; }
        }

        public MSChartControl()
        {
            InitializeComponent();
        }

        public MSChartControl(UniversalWidgetConfig aConfig)
            : this()
        {
            //ChartConfiguration = aConfig.ChartConfig;
            UniversalWidgetConfig = aConfig;
        }

        public MSChartControl(ChartConfig aChartConfig)
            : this()
        {
            ChartConfiguration = aChartConfig;
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
                ChartConfiguration = _theConfig.ChartConfig;         
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
                InitializeGraphControl();
            }
        }

        public HandleMouseClickEvent MouseClickHandler
        {
            get { return _theMouseClickHandler; }
            set { _theMouseClickHandler = value; }
        }


        //Can be called to populate the chart with the data passed
        public void PopulateChart(DataTable aDataTable)
        {
            if (aDataTable != null)
            {
                try
                {
                    _theChartRenderer.RenderChart(_theChart, _theChartConfig, aDataTable);
                }
                catch (Exception ex)
                {
                    Console.WriteLine("Exception = " + ex.Message);
                }
            }
        }

        /// <summary>
        /// Re-draws the graph in the UI
        /// </summary>
        private void Populate()
        {
            if (_theDataProvider != null)
            {
                try
                {
                    _theChartRenderer.RenderChart(_theChart, _theConfig, _theDataProvider);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error in rendering chart " + ex.Message + "\nPlease check your chart configuration.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleError), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW, ex.StackTrace);
            }
        }

        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void HandleError(Object obj, EventArgs e)
        {
            //Update status bar with exception
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            //Populate the chart
            Populate();
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
            //TODO: show the status bar 
        }

        private void InitializeGraphControl()
        {
            //Set the appropriate renderer
            _theChartRenderer = ChartRendererFactory.GetChartRenderer(_theChartConfig);
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

        private void _theChart_Click(object sender, EventArgs e)
        {
            EventArgs evt = e;
            Object obj = sender;
        }

        private void _theChart_DragDrop(object sender, DragEventArgs e)
        {
            //string columnName = (string)e.Data.GetData(DataFormats.Text);
            //ChartDesigner dialog = new ChartDesigner(_theConfig.ChartConfig);
            //dialog.Text = "Add Series";
            //dialog.DataTable = _theDataProvider.GetDataTable();
            //dialog.SelectedColumn = columnName;
            //dialog.PopulateUI();
            //dialog.ShowDialog();
            //if (dialog.DialogResult == DialogResult.OK)
            //{
            //    //((LineChartRenderer)_theChartRenderer).RenderChartForColumn(_theChart, _theConfig, _theDataProvider, columnName, true);
            //    ((LineChartRenderer)_theChartRenderer).RenderChart(_theChart, _theConfig, _theDataProvider);
            //}
        }

        private void _theChart_DragEnter(object sender, DragEventArgs e)
        {
            e.Effect = DragDropEffects.Link;
        }

        private void _theChart_DragOver(object sender, DragEventArgs e)
        {

        }

        private void populateSeries(string aColumn)
        {

            
        }

        private void _theChart_MouseClick(object sender, MouseEventArgs e)
        {
            if (_theMouseClickHandler != null)
            {
                _theMouseClickHandler(sender, e);
            }
        }

        private void _theChart_PrePaint(object sender, ChartPaintEventArgs e)
        {
            int i = 1;
        }

        private void _theChart_PostPaint(object sender, ChartPaintEventArgs e)
        {
            int i = 1;
        }
    }
}
