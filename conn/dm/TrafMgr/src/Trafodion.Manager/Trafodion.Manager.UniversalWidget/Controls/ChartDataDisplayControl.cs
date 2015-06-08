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
using Dundas.Charting.WinControl.Utilities;
using Dundas.Charting.WinControl;
//using Trafodion.Manager.AMQPWidgetCommon;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    public partial class ChartDataDisplayControl : UserControl, IDataDisplayControl
    {
        UniversalWidgetConfig _theUniversalWidgetConfig;
        DataProvider _theDataProvider;
        IDataDisplayHandler _theDataDisplayHandler;
        DrillDownManager _theDrillDownManager;
        DataTable _theDataTable;

        public ChartDataDisplayControl()
        {
            InitializeComponent();
        }

        public ChartDataDisplayControl(UniversalWidgetConfig aConfig)
            : this()
        {
            UniversalWidgetConfiguration = aConfig;
        }


        public virtual DataProvider DataProvider
        {
            get { return _theDataProvider; }
            set { _theDataProvider = value; }
        }

        public virtual UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _theUniversalWidgetConfig; }
            set 
            { 
                _theUniversalWidgetConfig = value;
                plotChart(_theUniversalWidgetConfig, null, _theChart);
            }
        }

        public virtual IDataDisplayHandler DataDisplayHandler
        {
            get { return _theDataDisplayHandler; }
            set { _theDataDisplayHandler = value; }
        }

        public virtual DrillDownManager DrillDownManager
        {
            get { return _theDrillDownManager; }
            set { _theDrillDownManager = value; }
        }

        public virtual void PersistConfiguration()
        { }

        private void AddHandlers()
        {
            if (_theDataProvider != null)
            {
                //Associate the event handlers
                _theDataProvider.OnErrorEncountered += HandleError;
                _theDataProvider.OnNewDataArrived += HandleNewDataArrived;
                _theDataProvider.OnFetchingData += HandleFetchingData;
            }
        }
        private void RemoveHandlers()
        {
            if (_theDataProvider != null)
            {
                //Remove the event handlers
                _theDataProvider.OnErrorEncountered -= HandleError;
                _theDataProvider.OnNewDataArrived -= HandleNewDataArrived;
                _theDataProvider.OnFetchingData -= HandleFetchingData;
            }
        }


        protected virtual void HandleError(Object obj, EventArgs e)
        {
            this._theChart.Invalidate();
            this._theChart.Series.Clear();
        }

        protected virtual void HandleNewDataArrived(Object obj, EventArgs e)
        {
            //Populate the chart
            Populate();

            //Update the status bar with chart status
        }

        protected virtual void HandleFetchingData(Object obj, EventArgs e)
        {
            //Just prior to fetching data, save the user config
            PopulateConfigToPersist();
        }

        /// <summary>
        /// Re-draws the graph in the UI
        /// </summary>
        protected virtual void Populate()
        {
            _theDataTable = _theDataProvider.GetDataTable();
            try
            {
                //DataDisplayHandler.DoPopulate(UniversalWidgetConfiguration, _theDataTable, _theDataGrid);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error encountered while populating chart - " + ex.Message, "Error in chart population", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Trafodion.Manager.Framework.Logger.OutputErrorLog(ex.StackTrace);
            }
        }

        /// <summary>
        /// This method populates the config with information that needs to be persisted
        /// </summary>
        protected virtual void PopulateConfigToPersist()
        {
            //Things to store
            //1. Chart Type
            //2. Legend type
            //3. etc            
        }


        //private Series GetSeriesForColumn(int Col)
        //{
        //    dataProvider = _theDataProvider as StatsAggregateAMQPDataProvider;
        //    Series series = new Series(dataProvider.GetSeriesName(Col));

        //    return series;
        //}


        private void plotChart(UniversalWidgetConfig aUniversalWidgetConfig, DataTable aDataTable, Chart aChart)
        {
            aChart.Invalidate();
            aChart.Series.Clear();
            DataView dataView = new DataView(aDataTable);
            Series series1 = new Series("s1");
            series1.ChartType = "StackedColumn";
            series1.Points.DataBindXY(new string[] { "0", "1", "2", "3", "4", "5", "6", "7", "8" },
                new int[] { 5, 23, 5, 9, 14, 2, 10, 4, 100 });

            aChart.Series.Add(series1);

            Series series2 = new Series();
            series2.ChartType = "StackedColumn";
            series2.Name = "Series2";
            series2.Points.DataBindXY(new string[] { "xxx", "February", "March", "April", "May", "June", "July", "August" },
                new int[] { 5, 23, 5, 9, 14, 2, 10, 4 });

            aChart.Series.Add(series2);

            Series series3 = new Series();
            series3.ChartType = "StackedColumn";
            series3.Name = "Series3";
            series3.Points.DataBindXY(new string[] { "0", "1", "2", "3", "4", "5", "6", "7", "8" },
                new int[] { 5, 23, 5, 9, 14, 2, 10, 4, 100 });

            aChart.Series.Add(series3);

            aChart.Update();
        }
    }
}
