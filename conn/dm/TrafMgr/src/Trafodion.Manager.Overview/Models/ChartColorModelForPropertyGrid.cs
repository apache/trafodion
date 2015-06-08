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

using System.ComponentModel;
using System.Data;
using System.Drawing;

namespace Trafodion.Manager.OverviewArea.Models
{
    class ChartColorModelForPropertyGrid
    {
        private string chartName;

        [CategoryAttribute("Chart Name"), ReadOnlyAttribute(true)]
        public string ChartName
        {
            get { return chartName; }
            set { chartName = value; }
        }
        private Color foreColor;

        [CategoryAttribute("Color Settings")]
        public Color ForeColor
        {
            get { return foreColor; }
            set { foreColor = value; }
        }
        private Color backColor;

        [CategoryAttribute("Color Settings")]
        public Color BackColor
        {
            get { return backColor; }
            set { backColor = value; }
        }
        private Color cursorColor;

        [CategoryAttribute("Color Settings")]
        public Color CursorColor
        {
            get { return cursorColor; }
            set { cursorColor = value; }
        }
        private Color gridlineColor;

        [CategoryAttribute("Color Settings")]
        public Color GridlineColor
        {
            get { return gridlineColor; }
            set { gridlineColor = value; }
        }
        private Color exceedThresholdColor;

        [DescriptionAttribute("Set the color when metric exceeds threshold value"), CategoryAttribute("Color Settings")]
        public Color ExceedThresholdColor
        {
            get { return exceedThresholdColor; }
            set { exceedThresholdColor = value; }
        }

        private Color avgLineforeColor;

        [DescriptionAttribute("Set the color of Average Line in Timeline Graph"), CategoryAttribute("Timeline Color Settings")]
        public Color AverageLineForeColor
        {
            get { return avgLineforeColor; }
            set { avgLineforeColor = value; }
        }

        private Color maxLineforeColor;

        [DescriptionAttribute("Set the color of Maximum Line in Timeline Graph"), CategoryAttribute("Timeline Color Settings")]
        public Color MaximumLineForeColor
        {
            get { return maxLineforeColor; }
            set { maxLineforeColor = value; }
        }

        private Color minLineforeColor;

         [DescriptionAttribute("Set the color of Minimum Line in Timeline Graph"), CategoryAttribute("Timeline Color Settings")]
        public Color MinimumLineForeColor
        {
            get { return minLineforeColor; }
            set { minLineforeColor = value; }
        }

        public ChartColorModelForPropertyGrid(SystemMetricModel.SystemMetrics  metric, DataTable chartColorTable)   
        {
            DataRow dr = chartColorTable.Rows.Find(metric);
            this.ChartName = SystemMetricModel.GetOverallSummaryTitle(metric);
            //this.ForeColor = (Color)dr[SystemMetricChartColorModel.SystemMetricsColorTableColumns.ColMetricColor.ToString()];
            //this.BackColor = (Color)dr[SystemMetricChartColorModel.SystemMetricsColorTableColumns.ColMetricBackColor.ToString()];
            //this.GridlineColor = (Color)dr[SystemMetricChartColorModel.SystemMetricsColorTableColumns.ColMetricGridLineColor.ToString()];
            //this.CursorColor = (Color)dr[SystemMetricChartColorModel.SystemMetricsColorTableColumns.ColMetricCursorColor.ToString()];            
            //this.ExceedThresholdColor = (Color)dr[SystemMetricChartColorModel.SystemMetricsColorTableColumns.ColMetricThresholdColor.ToString()];
            this.ForeColor = Color.FromName(dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricColor.ToString()].ToString());
            this.BackColor = Color.FromName(dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricBackColor.ToString()].ToString());
            this.GridlineColor = Color.FromName(dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricGridLineColor.ToString()].ToString());
            this.CursorColor =Color.FromName(dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricCursorColor.ToString()].ToString());
            this.ExceedThresholdColor = Color.FromName(dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricThresholdColor.ToString()].ToString());
            this.MaximumLineForeColor = Color.FromName(dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricMaxLineColor.ToString()].ToString());
            this.MinimumLineForeColor = Color.FromName(dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricMinLineColor.ToString()].ToString());
            this.AverageLineForeColor = Color.FromName(dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricAverageLineColor.ToString()].ToString());
        }

        //public ChartColorModelForPropertyGrid(string argChartName, Color argForeColor, Color argBackColor, Color argGridlineColor, Color argCursorColor, Color argExceedThresholdColor) 
        //{
        //    this.ChartName = argChartName;
        //    this.ForeColor = argForeColor;
        //    this.BackColor = argBackColor;
        //    this.GridlineColor = argGridlineColor;
        //    this.CursorColor = argCursorColor;
        //    this.ExceedThresholdColor = argExceedThresholdColor;        
        //}

    }
}
