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

using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionLineGraphUserControl : UserControl
    {
        private SystemMetric _activeSystemMetric;
        private ArrayList _rawDataArray = new ArrayList();
        private int _maxRecords = 50;
        private Dictionary<SystemMetric, bool> _settingDictionary = new Dictionary<SystemMetric, bool>();
        
        private ArrayList _activeSystemMetrics = new ArrayList();
        public Dictionary<SystemMetric, Pen> ColorDictionary = new Dictionary<SystemMetric, Pen>();


        public SystemMetric ActiveSystemMetric
        {
            get { return _activeSystemMetric; }
            set { _activeSystemMetric = value; }
        }


        public TrafodionLineGraphUserControl()
        {
            InitializeComponent();
            
            /*this.trackOption_TrafodionComboBox.Items.Add(MetricTypes.CPUBusy);
            this.trackOption_TrafodionComboBox.Items.Add(MetricTypes.DiskIO);
            this.trackOption_TrafodionComboBox.Items.Add(MetricTypes.CacheHits);
            this.trackOption_TrafodionComboBox.Items.Add(MetricTypes.Dispatch);
            this.trackOption_TrafodionComboBox.Items.Add(MetricTypes.Swap);
            this.trackOption_TrafodionComboBox.Items.Add(MetricTypes.FreeMemory);
            this.trackOption_TrafodionComboBox.Items.Add(MetricTypes.CPUQueueLength);
            */
            //this.trackOption_TrafodionComboBox.SelectedIndex = 01;

            //this._activeSystemMetric = aMetricType;
            //this.trackOption_TrafodionComboBox.SelectedItem = this._activeSystemMetric;

            //this.trackOption_TrafodionComboBox.
            //this.graphLabel_toolStripStatusLabel.Text = aMetricType.Label;
            //this.RealTimeGraph.GraphColor = new Pen(aMetricType.BarColor).Color;
            //UpdateGraphics();
        }

        public void AddGraphValues(Dictionary<string,float> valuesToAdd)
        {
            ArrayList temp = new ArrayList();
            temp.Add(valuesToAdd[ActiveSystemMetric.Type]);
            RealTimeGraph.AddGraphValues(temp);
        }

        public Trafodion.Manager.Framework.Controls.TrafodionRealTimeLineGraph RealTimeGraph
        {
            get { return graph_TrafodionRealTimeLineGraph; }
            set { graph_TrafodionRealTimeLineGraph = value; }
        }

        public void UpdateGraphics()
        {

            //dispose of the old pens
            ArrayList newColorList = new ArrayList();
            foreach (SystemMetric activeSysMetric in this._activeSystemMetrics)
            {
                newColorList.Add(ColorDictionary[activeSysMetric]);
            }
            this.RealTimeGraph.GraphColors = newColorList;
        }


        public void SetAggregationType(SystemMetric aSystemMetric, bool enabled)
        {
            //Toggle the given System Metric
        }

        public void SetMetricVisibility(SystemMetric aSystemMetric, bool aEnabled)
        {
            //Toggle the given System Metric

            //If this metric IS active and we DON'T want it to be:
            if (this._activeSystemMetrics.Contains(aSystemMetric) && !aEnabled)
                this._activeSystemMetrics.Remove(aSystemMetric);

            //If this metric ISN'T active and we WANT it to be:
            if (!this._activeSystemMetrics.Contains(aSystemMetric) && aEnabled)
                this._activeSystemMetrics.Add(aSystemMetric);

            this.RealTimeGraph.GraphValues = new ArrayList();
            this.UpdateGraphics();
            this.UpdateGraphDataFromRaw();
        }

        public void UpdateRawData(Dictionary<string, float> aRawDataObject)
        {
            //Update the array that holds the raw data
            this._rawDataArray.Add(aRawDataObject);
            if (this._rawDataArray.Count > this._maxRecords)
            {
                this._rawDataArray.RemoveAt(0);
            }

            //Calculate the new graph entry based on the current settings
            UpdateGraphData(aRawDataObject);
        }

        private void UpdateGraphDataFromRaw()
        {
            foreach (Dictionary<string, float> rawData in this._rawDataArray)
            {
                UpdateGraphData(rawData);            
            }
        }


        private void UpdateGraphData(Dictionary<string, float> aRawDataObject)
        {
            ArrayList timelineValues = new ArrayList();
            
            //For each ACTIVE metric type...
            foreach (SystemMetric activeSysMetric in this._activeSystemMetrics)
            {
                timelineValues.Add(aRawDataObject[activeSysMetric.Type]);
            }
            RealTimeGraph.AddGraphValues(timelineValues);
        }


    }
}
