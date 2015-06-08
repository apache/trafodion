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
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using System.Drawing;
using ZedGraph;

namespace Trafodion.Manager.UniversalWidget
{
    [Serializable]
    public class LineChartConfig : ChartConfig
    {

        List<ChartAreaConfig> _theChartAreaConfigs = new List<ChartAreaConfig>();
        /// <summary>
        /// Represents a chart area in the chart
        /// </summary>
        [XmlArray("ChartAreaConfigs")]
        [XmlArrayItem("ChartAreaConfig")]
        private List<ChartAreaConfig> ChartAreaConfigs
        {
            get { return _theChartAreaConfigs; }
            set { _theChartAreaConfigs = value; }
        } 



        string _theXaxisTitle;
        string _theYAxisTitle;
        bool _theUseLineSymbols;
        bool _theUseLineSymbolFills;
        bool _theUseLineFills;
        bool _theUseChartFill;


        AxisType _theXaxisType;
        List<ChartLine> _theChartLines = new List<ChartLine>();
        Fill _theChartFill;

        [XmlElement("XaxisTitle")]
        public string XaxisTitle
        {
            get { return _theXaxisTitle; }
            set { _theXaxisTitle = value; }
        }

        [XmlElement("YAxisTitle")]
        public string YAxisTitle
        {
            get { return _theYAxisTitle; }
            set { _theYAxisTitle = value; }
        }

        /// <summary>
        /// Indicates if symbols should be associated to lines
        /// </summary>
        [XmlElement("UseLineSymbols")]
        public bool UseLineSymbols
        {
            get { return _theUseLineSymbols; }
            set { _theUseLineSymbols = value; }
        }

        /// <summary>
        /// Indicates if symbols should be filled
        /// </summary>
        [XmlElement("UseLineSymbolFills")]
        public bool UseLineSymbolFills
        {
            get { return _theUseLineSymbolFills; }
            set { _theUseLineSymbolFills = value; }
        }

        /// <summary>
        /// Indicates if line fills should be used
        /// </summary>
        [XmlElement("UseLineFills")]
        public bool UseLineFills
        {
            get { return _theUseLineFills; }
            set { _theUseLineFills = value; }
        }

        /// <summary>
        /// Indicates if chart fills should be used
        /// </summary>
        [XmlElement("UseChartFill")]       
        public bool UseChartFill
        {
            get { return _theUseChartFill; }
            set { _theUseChartFill = value; }
        }

        /// <summary>
        /// Indicates if chart fills should be used
        /// </summary>
        [XmlElement("XaxisType")]
        public AxisType XaxisType
        {
            get { return _theXaxisType; }
            set { _theXaxisType = value; }
        }

        /// <summary>
        /// Represents lines on a line chart
        /// </summary>
        [XmlArray("ChartLines")]
        [XmlArrayItem("ChartLine")]
        public List<ChartLine> ChartLines
        {
            get { return _theChartLines; }
            set { _theChartLines = value; }
        }

        [XmlIgnore]
        public Fill ChartFill
        {
            get { return _theChartFill; }
            set { _theChartFill = value; }
        }

        [XmlIgnore]
        public List<PointConfig> PointConfigs
        {
            get
            {
                List<PointConfig> ret = new List<PointConfig>();
                foreach (ChartLine line in ChartLines)
                {
                    ret.Add(line.PointConfig);
                }
                return ret;
            }
        }


        public ChartLine GetChartLineForPointConfig(PointConfig aPointConfig)
        {
            foreach (ChartLine line in ChartLines)
            {
                if (line.PointConfig.Equals(aPointConfig))
                {
                    return line;
                }
            }
            return null;
        }

        /// <summary>
        /// Gets a specific chart renderer for this config type
        /// </summary>
        /// <returns></returns>
        public override ChartRenderer GetChartRenderer()
        {
            return new LineChartRenderer();
        }

        public  void ApplyTheme(ChartTheme aTheme)
        {

            int symbolIdx = 0;
            int symbolFillIdx = 0;
            int lineFillIdx = 0;
            int lineColorIdx = 0;
            foreach (ChartLine chartLine in ChartLines)
            {
                if (UseLineSymbols)
                {
                    chartLine.TheChartLineSymbol = aTheme.SymbolTypes[symbolIdx++];
                    symbolIdx = (symbolIdx > (aTheme.SymbolTypes.Length - 1)) ? 0 : symbolIdx;
                }

                if (UseLineSymbolFills)
                {
                    chartLine.SymbolFill = aTheme.SymbolFills[symbolFillIdx++];
                    symbolFillIdx = (symbolFillIdx > (aTheme.SymbolFills.Length - 1)) ? 0 : symbolFillIdx;
                }

                if (UseLineFills)
                {
                    chartLine.ChartLineFill = aTheme.LineFills[lineFillIdx++];
                    lineFillIdx = (lineFillIdx > (aTheme.LineFills.Length - 1)) ? 0 : lineFillIdx;
                }

                if (aTheme.LineColors != null)
                {
                    chartLine.ChartLineColor = aTheme.LineColors[lineColorIdx++];
                    lineColorIdx = (lineColorIdx > (aTheme.LineColors.Length - 1)) ? 0 : lineColorIdx;
                }

                if (UseChartFill)
                {
                    this.ChartFill = aTheme.ChartFill;
                }
            }
        }
    }



    /// <summary>
    /// Class representing a single line in a line chart
    /// </summary>
    [Serializable]
    public class ChartLine
    {
        [XmlElement("ChartLineLabel")]
        public string ChartLineLabel
        {
            get { return _theChartLineLabel; }
            set { _theChartLineLabel = value; }
        }

        [XmlElement("PointConfig")]
        public PointConfig PointConfig
        {
            get { return _thePointConfig; }
            set { _thePointConfig = value; }
        }

        [XmlIgnore]
        public SymbolType TheChartLineSymbol
        {
            get { return _theChartLineSymbol; }
            set { _theChartLineSymbol = value; }
        }
        [XmlIgnore]
        public Fill SymbolFill
        {
            get { return _theSymbolFill; }
            set { _theSymbolFill = value; }
        }
        [XmlIgnore]
        public Color ChartLineColor
        {
            get { return _theChartLineColor; }
            set { _theChartLineColor = value; }
        }

        [XmlIgnore]
        public Fill ChartLineFill
        {
            get { return _theChartLineFill; }
            set { _theChartLineFill = value; }
        }
        string _theChartLineLabel;
        SymbolType _theChartLineSymbol;
        Fill _theSymbolFill;
        Color _theChartLineColor;
        Fill _theChartLineFill;
        PointConfig _thePointConfig;
    }

    /// <summary>
    /// Has information to map a row of returned data to the PointList
    /// </summary>
    [Serializable]
    public class PointConfig
    {
        String _theXColName;
        String _theYColName;
        bool _theXColIsLinear;

        [XmlElement("XColIsLinear")]
        public bool XColIsLinear
        {
            get { return _theXColIsLinear; }
            set { _theXColIsLinear = value; }
        }

        [XmlElement("YColName")]
        public String YColName
        {
            get { return _theYColName; }
            set { _theYColName = value; }
        }

        [XmlElement("XColName")]
        public String XColName
        {
            get { return _theXColName; }
            set { _theXColName = value; }
        }


        public override bool Equals(Object obj)
        {
            bool ret = false;
            PointConfig config = obj as PointConfig;
            if (config != null)
            {
                //If it's linear then the y column name must be the same
                if ((_theXColIsLinear) && (config._theXColIsLinear == _theXColIsLinear))
                {
                    if ((_theYColName != null) && (config.YColName != null) && (_theYColName.Equals(config.YColName, StringComparison.InvariantCultureIgnoreCase)))
                    {
                        return true;
                    }
                }
                //Both X and y column must be the same
                else if ((_theYColName != null)
                    && (config.YColName != null)
                    && (_theYColName.Equals(config.YColName, StringComparison.InvariantCultureIgnoreCase))
                    && (_theXColName != null)
                    && (config.XColName != null)
                    && (_theXColName.Equals(config.XColName, StringComparison.InvariantCultureIgnoreCase)))
                {
                    return true;
                }
            }

            return ret;
        }

    }

    /// <summary>
    /// Overloaded PointPairList that saves the PointConfig along with the PointPairList
    /// </summary>
    public class TrafodionPointPairList : PointPairList
    {
        PointConfig _thePointConfig;

        public PointConfig PointConfig
        {
            get { return _thePointConfig; }
            set { _thePointConfig = value; }
        }

        public TrafodionPointPairList()
            : base()
        {
        }

        public TrafodionPointPairList(PointConfig aPointConfig)
            : base()
        {
            this._thePointConfig = aPointConfig;
        }
    }
}
