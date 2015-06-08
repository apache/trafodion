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
using System.ComponentModel;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.Serialization;
using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;
using System.Windows.Forms;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;

namespace Trafodion.Manager.UniversalWidget
{
    [Serializable]
    public class ChartConfig
    {
        #region Fields

        // appearance 
        private Color _theBackColor = Color.WhiteSmoke;
        private Color _theBorderLineColor;
        private ChartDashStyle _theBorderLineDashStyle = ChartDashStyle.Solid;
        private int _theBorderLineWidth = 1;
        private ChartColorPalette _thePalette = ChartColorPalette.BrightPastel;

        private List<ChartAreaConfig> _theChartAreaConfigs = new List<ChartAreaConfig>();
        private List<LegendConfig> _theLegendConfigs = new List<LegendConfig>();
        private List<TitleConfig> _theTitleConfigs = new List<TitleConfig>();

        #endregion Fields

        #region Properties

        [XmlIgnore()]
        [DisplayName("Back Color")]
        [Description("The background color for the chart.")]
        [Category("Appearance")]
        public Color BackColor
        {
            get { return _theBackColor; }
            set { _theBackColor = value; }
        }

        [XmlElement("BackColorType")]
        [Browsable(false)]
        public string XMLBackColorType
        {
            get { return XmlColor.SerializeColor(BackColor); }
            set { BackColor = XmlColor.DeserializeColor(value); }
        }

        [XmlElement("BorderLineDashStyle")]
        [DisplayName("Border Line Dash Style")]
        [Description("The dash style for the chart's border line.")]
        [Category("Appearance")]
        public ChartDashStyle BorderLineDashStyle
        {
            get { return _theBorderLineDashStyle; }
            set { _theBorderLineDashStyle = value; }
        }

        [XmlElement("BorderLineWidth")]
        [DisplayName("Border Line Width")]
        [Description("The width for the chart's border line. Valid value ranges from 1 to Int32 max value.")]
        [TypeConverter(typeof(OneToHundredConverter))]
        [DefaultValue(1)]
        [Category("Appearance")]
        public int BorderLineWidth
        {
            get { return _theBorderLineWidth; }
            set {
                if (value >= 1 && value <= Int32.MaxValue)
                {
                    _theBorderLineWidth = value;
                }
                else
                {
                    _theBorderLineWidth = 1;
                }
            }
        }

        [XmlIgnore()]
        [DisplayName("Border Line Color")]
        [Description("The color for the chart's border line.")]
        [Category("Appearance")]
        public Color BorderLineColor
        {
            get { return _theBorderLineColor; }
            set { _theBorderLineColor = value; }
        }

        [XmlElement("BorderLineColorType")]
        [Browsable(false)]
        [ReadOnly(true)]
        public string XMLBorderLineColorType
        {
            get { return XmlColor.SerializeColor(BorderLineColor); }
            set { BorderLineColor = XmlColor.DeserializeColor(value); }
        }

        [XmlElement("ChartPalette")]
        [DisplayName("Chart Palette")]
        [Description("The chart's palette.")]
        [Category("Appearance")]
        public ChartColorPalette ChartPalette
        {
            get { return _thePalette; }
            set { _thePalette = value; }
        }

        /// <summary>
        /// Represents a chart area in the chart
        /// </summary>
        [Browsable(false)]
        [XmlArray("ChartAreaConfigs")]
        [XmlArrayItem("ChartAreaConfig")]
        public List<ChartAreaConfig> ChartAreaConfigs
        {
            get { return _theChartAreaConfigs; }
            set { _theChartAreaConfigs = value; }
        }

        /// <summary>
        /// Represents a legend in the chart
        /// </summary>
        [Browsable(false)]
        [XmlArray("LegendConfigs")]
        [XmlArrayItem("LegendConfig")]
        public List<LegendConfig> LegendConfigs
        {
            get { return _theLegendConfigs; }
            set { _theLegendConfigs = value; }
        }

        /// <summary>
        /// Represents a Title in the chart
        /// </summary>
        [Browsable(false)]
        [XmlArray("TitleConfigs")]
        [XmlArrayItem("TitleConfig")]
        public List<TitleConfig> TitleConfigs
        {
            get { return _theTitleConfigs; }
            set { _theTitleConfigs = value; }
        }

        #endregion Properties

        public ChartConfig ShallowCopy()
        {
            ChartConfig newObj = new ChartConfig();
            newObj.ChartPalette = this.ChartPalette;
            newObj.BackColor = this.BackColor;
            newObj.BorderLineWidth = this.BorderLineWidth;
            newObj.BorderLineColor = this.BorderLineColor;
            newObj.BorderLineDashStyle = this.BorderLineDashStyle;

            return newObj;
        }


        public ChartConfig DeepCopy()
        {
            MemoryStream m = new MemoryStream();
            BinaryFormatter b = new BinaryFormatter();
            b.Serialize(m, this);
            m.Position = 0;
            return (ChartConfig)b.Deserialize(m);
        }

        //Helper method to add a legend to the chart
        public void AddLegend(string aName, string aText)
        {
            if ((aName == null) || (aText == null) || (aName.Trim().Length == 0) || (aText.Trim().Length == 0))
            {
                return;
            }

            foreach (LegendConfig config in LegendConfigs)
            {
                if (config.Name.Equals(aName))
                {
                    return;
                }
            }
            _theLegendConfigs.Add(new LegendConfig(aName, aText));
        }


        //Helper method to add a title to the chart
        public void AddTitle(string aName, string aText)
        {
            if ((aName == null) || (aText == null) || (aName.Trim().Length == 0) || (aText.Trim().Length == 0))
            {
                return;
            }

            foreach (TitleConfig config in TitleConfigs)
            {
                if (config.Name.Equals(aName))
                {
                    return;
                }
            }
            _theTitleConfigs.Add(new TitleConfig(aName, aText));
        }


    
        /// <summary>
        /// Subclasses must override this method to return appropriate data provider
        /// </summary>
        /// <returns></returns>
        public virtual ChartRenderer GetChartRenderer()
        {
            return new LineChartRenderer();
        }
    }




    /// <summary>
    /// Class representing a Legend in a chart
    /// </summary>
    [Serializable]
    public class LegendConfig
    {
        string _theName;
        string _theText;
        string _theDockedChartAreaName; 

        public LegendConfig() { }
        public LegendConfig(string aName, string aText) 
        {
            _theName = aName;
            _theText = aText;
        }

        [XmlElement("Name")]
        public string Name
        {
            get { return _theName; }
            set { _theName = value; }
        }

        [XmlElement("Text")]
        public string Text
        {
            get { return _theText; }
            set { _theText = value; }
        }

        [XmlElement("DockedChartAreaName")]
        public string DockedChartAreaName
        {
            get { return _theDockedChartAreaName; }
            set { _theDockedChartAreaName = value; }
        }
    }


    /// <summary>
    /// Class representing a Title in a chart
    /// </summary>
    [Serializable]
    public class TitleConfig
    {
        string _theName;
        string _theText;
        string _theDockedChartAreaName; 

        public TitleConfig() { }
        public TitleConfig(string aName, string aText) 
        {
            _theName = aName;
            _theText = aText;
        }

        [XmlElement("Name")]
        public string Name
        {
            get { return _theName; }
            set { _theName = value; }
        }

        [XmlElement("Text")]
        public string Text
        {
            get { return _theText; }
            set { _theText = value; }
        }

        [XmlElement("DockedChartAreaName")]
        public string DockedChartAreaName
        {
            get { return _theDockedChartAreaName; }
            set { _theDockedChartAreaName = value; }
        }
    }

    public class ColumnNameConverter : StringConverter
    {
        private static string[] _values = null;

        public static string[] StandardValues
        {
            get { return _values; }
            set { _values = value; }
        }

        public override bool GetStandardValuesSupported(ITypeDescriptorContext context)
        {
            return true;
        }

        public override StandardValuesCollection
                             GetStandardValues(ITypeDescriptorContext context)
        {
            return new StandardValuesCollection(StandardValues);
        } 
    }

    public class XmlColor
    {
        public enum ColorFormat
        {
            NamedColor,
            ARGBColor
        }

        public static string SerializeColor(Color color)
        {
            if (color == null)
                return null;

            if (color.IsNamedColor)
                return string.Format("{0}:{1}",
                    ColorFormat.NamedColor, color.Name);
            else
                return string.Format("{0}:{1}:{2}:{3}:{4}:{5}",
                    ColorFormat.ARGBColor,
                    color.A, color.R, color.G, color.B, color.IsEmpty);
        }

        public static Color DeserializeColor(string color)
        {
            byte a, r, g, b;
            bool isEmpty;
            Color c = Color.Empty;

            if (string.IsNullOrEmpty(color))
                return c;

            string[] pieces = color.Split(new char[] { ':' });

            ColorFormat colorType = (ColorFormat)
                Enum.Parse(typeof(ColorFormat), pieces[0], true);

            switch (colorType)
            {
                case ColorFormat.NamedColor:
                    return Color.FromName(pieces[1]);

                case ColorFormat.ARGBColor:
                    a = byte.Parse(pieces[1]);
                    r = byte.Parse(pieces[2]);
                    g = byte.Parse(pieces[3]);
                    b = byte.Parse(pieces[4]);
                    isEmpty = bool.Parse(pieces[5]);

                    if (!isEmpty)
                    {
                        c = Color.FromArgb(a, r, g, b);
                    }

                    return c;
            }

            return c;
        }

    }

    public class OneToHundredConverter : Int32Converter
    {
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == typeof(string))
            {
                return true;
            }

            return false;
        }

        public override object ConvertFrom(ITypeDescriptorContext context, System.Globalization.CultureInfo culture, object value)
        {

            if (value is string)
            {
                int depth = int.Parse(value as string);
                if (depth < 1 || depth > Int32.MaxValue)
                {
                    throw new ArgumentException("The valid values are from 1 to " + Int32.MaxValue, context.PropertyDescriptor.DisplayName);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }

        public override bool IsValid(ITypeDescriptorContext context, object value)
        {
            int depth = (int)value;
            return (depth >= 1 && depth <= Int32.MaxValue);
        }
    }
}
