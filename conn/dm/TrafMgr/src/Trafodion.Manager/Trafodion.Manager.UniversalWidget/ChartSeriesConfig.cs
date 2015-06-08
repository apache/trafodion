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
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms.DataVisualization.Charting;
using System.Xml;
using System.Xml.Serialization;

namespace Trafodion.Manager.UniversalWidget
{

    /// <summary>
    /// Class representing a series in a chart area
    /// </summary>
    [Serializable]
    public class ChartSeriesConfig
    {
        #region Fields

        // Misc
        private string _theName;
        private string _theText;
        private string _theChartAreaName;
        private string _theLegendName;

        // Axes
        private string _theYValueColumnName;
        private System.Windows.Forms.DataVisualization.Charting.ChartValueType _theYValueColumnDataType;
        private string _theXValueColumnName;
        private System.Windows.Forms.DataVisualization.Charting.ChartValueType _theXValueColumnDataType;

        // Chart
        private System.Windows.Forms.DataVisualization.Charting.SeriesChartType _theChartType;

        // Appearance
        private bool _enabled = true;
        private Color _color;
        private ChartColorPalette _palette = ChartColorPalette.None;
        private string _toolTip = "Value #VALY at #VALX";

        // Label
        private bool _isValueShownAsLabel = false;
        private string _label = "";
        private string _labelFormat = "";

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Name - the name of the series
        /// </summary>
        [XmlElement("Name")]
        [Description("The name of the chart series.")]
        [Category("\tName")]
        public string Name
        {
            get { return _theName; }
            set { _theName = value; }
        }

        /// <summary>
        /// Property: ChartAreaName - the area name where the series is in
        /// </summary>
        [XmlElement("ChartAreaName")]
        [Browsable(false)]
        [ReadOnly(true)]
        public string ChartAreaName
        {
            get { return _theChartAreaName; }
            set { _theChartAreaName = value; }
        }

        /// <summary>
        /// Property: LegendName - the Chart Area's Legend name 
        /// </summary>
        [XmlElement("LegendName")]
        [Browsable(false)]
        [ReadOnly(true)]
        public string LegendName
        {
            get { return _theLegendName; }
            set { _theLegendName = value; }
        }

        /// <summary>
        /// Property: YValueColumnName
        /// </summary>
        [XmlElement("YValueColumnName")]
        [DisplayName("Y Axis Column Name")]
        [Description("Specifies the column name from the data grid to plot the Y Axis value.")]
        [Category("Data Source")]
        [TypeConverter(typeof(ColumnNameConverter))]
        public string YValueColumnName
        {
            get { return _theYValueColumnName; }
            set { _theYValueColumnName = value; }
        }

        /// <summary>
        /// Property: YValueColumnDataType
        /// </summary>
        [XmlElement("YValueColumnDataType")]
        [DisplayName("Y Axis Data Type")]
        [Description("Specifies the Y Axis value data type.")]
        [Category("Data Source")]
        public System.Windows.Forms.DataVisualization.Charting.ChartValueType YValueColumnDataType
        {
            get { return _theYValueColumnDataType; }
            set { _theYValueColumnDataType = value; }
        }

        /// <summary>
        /// Property: XValueColumnName
        /// </summary>
        [XmlElement("XValueColumnName")]
        [DisplayName("X Axis Column Name")]
        [Description("Specifies the X Axis column name.")]
        [Category("Data Source")]
        [TypeConverter(typeof(ColumnNameConverter))]
        public string XValueColumnName
        {
            get { return _theXValueColumnName; }
            set { _theXValueColumnName = value; }
        }

        /// <summary>
        /// Property: XValueColumnDataType
        /// </summary>
        [XmlElement("XValueColumnDataType")]
        [DisplayName("X Axis Data Type")]
        [Description("Specifies the X Axis value data type.")]
        [Category("Data Source")]
        public System.Windows.Forms.DataVisualization.Charting.ChartValueType XValueColumnDataType
        {
            get { return _theXValueColumnDataType; }
            set { _theXValueColumnDataType = value; }
        }

        /// <summary>
        /// Property: ChartType
        /// </summary>
        [XmlElement("ChartType")]
        [DisplayName("Chart Type")]
        [Description("Specifies the chart type of this chart series.")]
        [Category("Chart")]
        public System.Windows.Forms.DataVisualization.Charting.SeriesChartType ChartType
        {
            get { return _theChartType; }
            set { _theChartType = value; }
        }

        /// <summary>
        /// Property: Text 
        /// </summary>
        [XmlElement("Text")]
        [Browsable(false)]
        [ReadOnly(true)]
        public string Text
        {
            get { return _theText; }
            set { _theText = value; }
        }

        // Appearance

        [XmlElement("Enabled")]
        [DisplayName("Enabled")]
        [Description("To enable/disable this series in the Chart.")]
        [Category("Appearance")]
        public bool Enabled
        {
            get { return _enabled; }
            set { _enabled = value; }
        }

        [XmlIgnore()]
        [DisplayName("Color")]
        [Description("The color for this chart series.")]
        [Category("Appearance")]
        public Color Color
        {
            get { return _color; }
            set { _color = value; }
        }

        [XmlElement("XmlColorType")]
        [Browsable(false)]
        [ReadOnly(true)]
        public string XmlColorType
        {
            get { return XmlColor.SerializeColor(Color); }
            set { Color = XmlColor.DeserializeColor(value); }
        }

        [XmlElement("Palette")]
        [DisplayName("Palette")]
        [Description("The palette for this chart series")]
        [Category("Appearance")]
        public ChartColorPalette Palette
        {
            get { return _palette; }
            set { _palette = value; }
        }

        [XmlElement("ToolTip")]
        [DisplayName("Tool Tip")]
        [Description("Tool tip for this chart series")]
        [Category("Appearance")]
        [DefaultValue("Value #VALY at #VALX")]
        public string ToolTip
        {
            get { return _toolTip; }
            set { _toolTip = value; }
        }

        // Label

        [XmlElement("IsValueShownAsLabel")]
        [DisplayName("Is Value Shown As Label")]
        [Description("Values are shown in the series as labels.")]
        [Category("Label")]
        public bool IsValueShownAsLabel
        {
            get { return _isValueShownAsLabel; }
            set { _isValueShownAsLabel = value; }
        }

        [XmlElement("Label")]
        [DisplayName("Label")]
        [Description("Label to be shown for data points.")]
        [Category("Label")]
        public string Label
        {
            get { return _label; }
            set { _label = value; }
        }

        [XmlElement("LabelFormat")]
        [DisplayName("Label Format")]
        [Description("This is the string formatter for labels.")]
        [Category("Label")]
        public string LabelFormat
        {
            get { return _labelFormat; }
            set { _labelFormat = value; }
        }

        #endregion Properties
    }
}
