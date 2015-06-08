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

namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// Class representing a chart area in a chart
    /// </summary>
    [Serializable]
    public class ChartAreaConfig
    {
        #region Fields

        // Chart Area attributes
        private string _theName = "";
        private string _theXAxisTitle = "";
        private string _theYAxisTitle = "";
        private string _theXAxisLabelStyleFormat = "";
        private string _theYAxisLabelStyleFormat = "";
        private Font _theXAxisLabelStyleFont = null;
        private Font _theYAxisLabelStyleFont = null;

        private List<ChartSeriesConfig> _theChartSeriesConfigs = new List<ChartSeriesConfig>();

        // 3D attributes
        private bool _enable3D = false;
        private int _inclination = 30;
        private bool _isClustered = false;
        private bool _isRightAngleAxes = false;
        private LightStyle _lightStyle = LightStyle.None;
        private int _perspective = 0;
        private int _pointDepth = 100;
        private int _pointGapDepth = 100;
        private int _rotation = 30;
        private int _wallWidth = 7;

        // Appearance attributes
        private Color _backColor;
        private Color _borderColor;
        private ChartDashStyle _borderDashStyle = ChartDashStyle.Solid;
        private int _borderWidth = 1;
        private Font _areaFont = new System.Drawing.Font("Tahoma", 8.25F); 

        // Legend attributes
        private bool _enableLegend = false;
        private string _legendTitle = "";
        private string _legendName = "";
        private Docking _legendDocking = Docking.Right;
        private bool _legendInsideChart = false;

        // Title attributes
        private bool _enabledTitle = false;
        private string _theTitle = "";
        private string _theTitleName = "";
        private Docking _theTitleDocking = Docking.Top;
        private bool _theTitleInsideChart = false;

        // alignment attributes
        private AreaAlignmentOrientations _alignmentOrientation;
        private AreaAlignmentStyles _alignmentStyle = AreaAlignmentStyles.All;
        private string _alignWithChartAreaName = "";

        #endregion Fields

        #region Properties

        // Misc properties

        [XmlElement("Name")]
        [DisplayName("Chart Area Name")]
        [Description("The name of the chart area.")]
        [Category("\tName")]
        public string Name
        {
            get { return _theName; }
            set { _theName = value; }
        }

        [XmlElement("EnableTitle")]
        [DisplayName("\tEnable Title")]
        [Description("To enable title")]
        [Category("Title")]
        [DefaultValue(false)]
        public bool EnableTitle
        {
            get { return _enabledTitle; }
            set { _enabledTitle = value; }
        }


        [XmlElement("Title")]
        [DisplayName("Chart Area Title")]
        [Description("The title to be displayed for the chart area.")]
        [Category("Title")]
        [DefaultValue("")]
        public string Title
        {
            get { return _theTitle; }
            set { _theTitle = value; }
        }

        [XmlElement("TitleName")]
        [Browsable(false)]
        [ReadOnly(true)]
        [DefaultValue("")]
        [Category("Title")]
        public string TitleName
        {
            get { return _theTitleName; }
            set { _theTitleName = value; }
        }

        [XmlElement("TitleDocking")]
        [DisplayName("Docking")]
        [Description("Title docking position.")]
        [DefaultValue(Docking.Top)]
        [Category("Title")]
        public Docking TitleDocking
        {
            get { return _theTitleDocking; }
            set { _theTitleDocking = value; }
        }

        [XmlElement("IsTitleInsideChart")]
        [DisplayName("Is Inside Chart")]
        [Description("Is the title to be placed inside the chart area?")]
        [DefaultValue(false)]
        [Category("Title")]
        public bool IsTitleInsideChart
        {
            get { return _theTitleInsideChart; }
            set { _theTitleInsideChart = value; }
        }

        /// <summary>
        /// Represents a series in a chart area
        /// </summary>
        [Browsable(false)]
        [XmlArray("ChartSeriesConfigs")]
        [XmlArrayItem("ChartSeriesConfig")]
        public List<ChartSeriesConfig> ChartSeriesConfigs
        {
            get { return _theChartSeriesConfigs; }
            set { _theChartSeriesConfigs = value; }
        }

        // Axes category 

        [XmlElement("XAxisTitle")]
        [DisplayName("X Axis Title")]
        [Description("The title for the X Axis.")]
        [DefaultValue("")]
        [Category("Axes")]
        public string XAxisTitle
        {
            get { return _theXAxisTitle; }
            set { _theXAxisTitle = value; }
        }

        [XmlElement("YAxisTitle")]
        [DisplayName("Y Axis Title")]
        [Description("The title for the Y Axis.")]
        [DefaultValue("")]
        [Category("Axes")]
        public string YAxisTitle
        {
            get { return _theYAxisTitle; }
            set { _theYAxisTitle = value; }
        }

        [XmlElement("XAxisLabelStyleFormat")]
        [DisplayName("X Axis Label Style Format")]
        [Description("The X Axis label style format string.")]
        [DefaultValue("")]
        [Category("Axes")]
        public string XAxisLabelStyleFormat
        {
            get { return _theXAxisLabelStyleFormat; }
            set { _theXAxisLabelStyleFormat = value; }
        }

        [XmlIgnore()]
        [DisplayName("X Axis Label Style Font")]
        [Description("X Axis label font.")]
        [DefaultValue(null)]
        [Category("Axes")]
        public Font XAxisLabelStyleFont
        {
            get { return _theXAxisLabelStyleFont; }
            set { _theXAxisLabelStyleFont = value; }
        }

        [XmlElement("XAxisLabelStyleFont")]
        [Browsable(false)]
        public XmlFont XAxisLabelStyleFontType
        {
            get { return new XmlFont(XAxisLabelStyleFont); }
            set { XAxisLabelStyleFont = value.ToFont(); }
        }


        [XmlElement("YAxisLabelStyleFormat")]
        [DisplayName("Y Axis Label Style Format")]
        [Description("The Y Axis label style format string.")]
        [DefaultValue("")]
        [Category("Axes")]
        public string YAxisLabelStyleFormat
        {
            get { return _theYAxisLabelStyleFormat; }
            set { _theYAxisLabelStyleFormat = value; }
        }

        [XmlIgnore()]
        [DisplayName("Y Axis Label Style Font")]
        [Description("Y Axis label font.")]
        [DefaultValue(null)]
        [Category("Axes")]
        public Font YAxisLabelStyleFont
        {
            get { return _theYAxisLabelStyleFont; }
            set { _theYAxisLabelStyleFont = value; }
        }

        [XmlElement("YAxisLabelStyleFont")]
        [Browsable(false)]
        public XmlFont YAxisLabelStyleFontType
        {
            get { return new XmlFont(YAxisLabelStyleFont); }
            set { YAxisLabelStyleFont = value.ToFont(); }
        }

        // Category 3D Style

        [XmlElement("Enable3D")]
        [DisplayName("Enable 3D")]
        [Description("To enable 3D appearance.")]
        [DefaultValue(false)]
        [Category("3D Style")]
        public bool Enable3D
        {
            get { return _enable3D; }
            set { _enable3D = value; }
        }

        [XmlElement("Inclination")]
        [DisplayName("Inclination")]
        [Description("Is the angle of rotation around the horizontal axes for 3D chart areas. A valid angle must range from -90 to 90 degrees.")]
        [TypeConverter(typeof(InclinationConverter))]
        [DefaultValue(30)]
        [Category("3D Style")]
        public int Inclination
        {
            get { return _inclination; }
            set 
            {
                if (value >= -90 && value <= 90)
                {
                    _inclination = value;
                }
                else
                {
                    _inclination = 30;
                }
            }
        }

        [XmlElement("IsClustered")]
        [DisplayName("Is Clustered")]
        [Description("Determines whether or not data series for a bar or column chart are clustered, i.e. displayed along distinct rows.")]
        [DefaultValue(false)]
        [Category("3D Style")]
        public bool IsClustered
        {
            get { return _isClustered; }
            set { _isClustered = value; }
        }

        [XmlElement("IsRightAngleAxes")]
        [DisplayName("Is Right Angle Axes")]
        [Description("Determines whether or not the 3D chart area is displayed using an isometric projection.")]
        [DefaultValue(false)]
        [Category("3D Style")]
        public bool IsRightAngleAxes
        {
            get { return _isRightAngleAxes; }
            set { _isRightAngleAxes = value; }
        }

        [XmlElement("3DLightStyle")]
        [DisplayName("Light Style")]
        [Description("Is the style of lighting for the 3D chart area.")]
        [DefaultValue((object)LightStyle.None)]
        [Category("3D Style")]
        public LightStyle LightStyleProperty
        {
            get { return _lightStyle; }
            set { _lightStyle = value; }
        }

        [XmlElement("Perspective")]
        [DisplayName("Perspective")]
        [Description("Is the percent of perspective for the 3D chart area. Valid value ranges from 0 to 100.")]
        [TypeConverter(typeof(PerspectiveConverter))]
        [DefaultValue(0)]
        [Category("3D Style")]
        public int Perspective
        {
            get { return _perspective; }
            set 
            {
                if (value >= 0 && value <= 100)
                {
                    _perspective = value;
                }
                else
                {
                    _perspective = 0;
                }
            }
        }

        [XmlElement("PointDepth")]
        [DisplayName("Point Depth")]
        [Description("Is the depth of data points displayed in the 3D chart area. Valid points depth must range from 0 to 1000.")]
        [TypeConverter(typeof(DepthConverter))]
        [DefaultValue(100)]
        [Category("3D Style")]
        public int PointDepth
        {
            get { return _pointDepth; }
            set 
            {
                if (value >= 0 && value <= 1000)
                {
                    _pointDepth = value;
                }
                else
                {
                    _pointDepth = 100;
                }
            }
        }

        [XmlElement("PointGapDepth")]
        [DisplayName("Point Gap Depth")]
        [Description("Is the distance between series rows in the 3D chart area. Valid distance must range from 0 to 1000.")]
        [DefaultValue(100)]
        [TypeConverter(typeof(DepthConverter))]
        [Category("3D Style")]
        public int PointGapDepth
        {
            get { return _pointGapDepth; }
            set 
            {
                if (value >= 0 && value <= 1000)
                {
                    _pointGapDepth = value;
                }
                else
                {
                    _pointGapDepth = 100;
                }
            }
        }

        [XmlElement("Rotation")]
        [DisplayName("Rotation")]
        [Description("Is the angle of rotation around the vertical axes for the 3D chart areas. Valid angle must range from -180 to 180 degrees.")]
        [DefaultValue(30)]
        [TypeConverter(typeof(DegreeConverter))]
        [Category("3D Style")]
        public int Rotation
        {
            get { return _rotation; }
            set 
            {
                if (value >= -180 && value <= 180)
                {
                    _rotation = value;
                }
                else
                {
                    _rotation = 30;
                }
            }
        }

        [XmlElement("WallWidth")]
        [DisplayName("Wall Width")]
        [Description("Is the width of the walls displayed in the 3D chart area. Valid wall width must range from 0 to 30 pixels.")]
        [TypeConverter(typeof(WallWidthConverter))]
        [DefaultValue(7)]
        [Category("3D Style")]
        public int WallWidth
        {
            get { return _wallWidth; }
            set 
            {
                if (value >= 0 && value <= 30)
                {
                    _wallWidth = value;
                }
                else
                {
                    _wallWidth = 7;
                }
            }
        }

        // Category Appearance 

        [XmlIgnore()]
        [DisplayName("Border Color")]
        [Description("The color for the chart area border.")]
        [Category("Appearance")]
        public Color BorderColor
        {
            get { return _borderColor; }
            set { _borderColor = value; }
        }

        [XmlElement("BorderColorType")]
        [Browsable(false)]
        [ReadOnly(true)]
        public string XMLBorderColorType
        {
            get { return XmlColor.SerializeColor(BorderColor); }
            set { BorderColor = XmlColor.DeserializeColor(value); }
        }

        [XmlElement("BorderDashStyle")]
        [DisplayName("Border Dash Style")]
        [Description("The dash style for the chart area border.")]
        [Category("Appearance")]
        public ChartDashStyle BorderDashStyle
        {
            get { return _borderDashStyle; }
            set { _borderDashStyle = value; }
        }

        [XmlElement("BorderWidth")]
        [DisplayName("Border Width")]
        [Description("The width for the chart area border.")]
        [Category("Appearance")]
        public int BorderWidth
        {
            get { return _borderWidth; }
            set { _borderWidth = value; }
        }

        // Category Legend
        [XmlElement("EnableLegend")]
        [DisplayName("\tEnable Legend")]
        [Description("To enable Legend in this chart area.")]
        [DefaultValue(false)]
        [Category("Legend")]
        public bool EnableLegend
        {
            get { return _enableLegend; }
            set { _enableLegend = value; }
        }
        [XmlElement("LegendTitle")]
        [DisplayName("\tLegend Title")]
        [Description("The title of the Legend in this chart area.")]
        [DefaultValue("")]
        [Category("Legend")]
        public string LegendTitle
        {
            get { return _legendTitle; }
            set { _legendTitle = value; }
        }

        [XmlElement("LegendName")]
        [Browsable(false)]
        [ReadOnly(true)]
        [Category("Legend")]
        public string LegendName
        {
            get { return _legendName; }
            set { _legendName = value; }
        }

        [XmlElement("LegendDocking")]
        [DisplayName("Docking")]
        [Description("Legend docking position.")]
        [DefaultValue(Docking.Right)]
        [Category("Legend")]
        public Docking LegendDocking
        {
            get { return _legendDocking; }
            set { _legendDocking = value; }
        }

        [XmlElement("IsLegendInsideChart")]
        [DisplayName("Is Inside Chart")]
        [Description("Is the legend inside the chart?")]
        [DefaultValue(false)]
        [Category("Legend")]
        public bool IsLegendInsideChart
        {
            get { return _legendInsideChart; }
            set { _legendInsideChart = value; }
        }

        [XmlIgnore()]
        [DisplayName("Back Color")]
        [Description("The background color for the chart area.")]
        [Category("Appearance")]
        public Color BackColor
        {
            get { return _backColor; }
            set { _backColor = value; }
        }

        [XmlElement("BackColorType")]
        [Browsable(false)]
        public string XMLBackColorType
        {
            get { return XmlColor.SerializeColor(BackColor); }
            set { BackColor = XmlColor.DeserializeColor(value); }
        }

        [XmlIgnore()]
        [DisplayName("Chart Font")]
        [Category("Appearance")]
        public Font ChartFont
        {
            get { return _areaFont; }
            set { _areaFont = value; }
        }

        [XmlElement("ChartFontType")]
        [Browsable(false)]
        public XmlFont ChartFontType
        {
            get { return new XmlFont(ChartFont); }
            set { ChartFont = value.ToFont(); }
        }

        #endregion Properties

        [Serializable]
        public struct XmlFont
        {
            public string family;
            //public GraphicsUnit unit;
            public float size;
            public FontStyle style;

            public XmlFont(Font f)
            {
                if (f == null)
                {
                    family = null;
                    size = 0;
                    style = FontStyle.Regular;
                }
                else
                {
                    family = f.FontFamily.Name;
                    //unit = f.Unit;
                    size = f.Size;
                    style = f.Style;
                }
            }

            public Font ToFont()
            {
                //return new Font(family, size, style, unit);
                if (string.IsNullOrEmpty(family))
                {
                    return null;
                }

                return new Font(family, size, style);
            }

        }

        //// alignment attributes
        //[XmlElement("AlignmentOrientation")]
        //[DisplayName("Alignment Orientation")]
        //[Description("The alignment orientation.")]
        //[Category("Alignment")]
        //public AreaAlignmentOrientations AlignmentOrientation
        //{
        //    get { return _alignmentOrientation; }
        //    set { _alignmentOrientation = value; }
        //}

        //[XmlElement("AlignmentStyle")]
        //[DisplayName("Alignment Style")]
        //[Description("The alignment style.")]
        //[Category("Alignment")]
        //public AreaAlignmentStyles AlignmentStyle
        //{
        //    get { return _alignmentStyle; }
        //    set { _alignmentStyle = value; }
        //}

        //[XmlElement("AlignWithChartArea")]
        //[DisplayName("Align With Chart Area")]
        //[Description("align with the chart area.")]
        //[Category("Alignment")]
        //public string AlignWithChartArea
        //{
        //    get { return _alignWithChartAreaName; }
        //    set { _alignWithChartAreaName = value; }
        //}
    }

    /// <summary>
    /// 
    /// </summary>
    public class InclinationConverter : Int32Converter
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
                int degree = int.Parse(value as string);
                if (degree < -90 || degree > 90)
                {
                    throw new ArgumentException("The valid values are from -180 to 180 degrees.", context.PropertyDescriptor.DisplayName);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }

        public override bool IsValid(ITypeDescriptorContext context, object value)
        {
            int degree = (int)value;
            return (degree >= -90 && degree <= 90);
        }
    }


    /// <summary>
    /// 
    /// </summary>
    public class DegreeConverter : Int32Converter
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
                int degree = int.Parse(value as string);
                if (degree < -180 || degree > 180)
                {
                    throw new ArgumentException("The valid values are from -180 to 180 degrees.", context.PropertyDescriptor.DisplayName);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }

        public override bool IsValid(ITypeDescriptorContext context, object value)
        {
            int degree = (int)value;
            return (degree >= -180 && degree <= 180);
        }
    }

    
    /// <summary>
    /// 
    /// </summary>
    public class DepthConverter : Int32Converter
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
                if (depth < 0 || depth > 1000)
                {
                    throw new ArgumentException("The valid values are from 0 to 1000.", context.PropertyDescriptor.DisplayName);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }

        public override bool IsValid(ITypeDescriptorContext context, object value)
        {
            int depth = (int)value;
            return (depth >= 0 && depth <= 1000);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    public class WallWidthConverter : Int32Converter
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
                int degree = int.Parse(value as string);
                if (degree < 0 || degree > 30)
                {
                    throw new ArgumentException("The valid values are from -180 to 180 degrees.", context.PropertyDescriptor.DisplayName);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }

        public override bool IsValid(ITypeDescriptorContext context, object value)
        {
            int degree = (int)value;
            return (degree >= 0 && degree <= 30);
        }
    }

    /// <summary>
    /// 
    /// </summary>
    public class PerspectiveConverter : Int32Converter
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
                if (depth < 0 || depth > 100)
                {
                    throw new ArgumentException("The valid values are from 0 to 100 percent.", context.PropertyDescriptor.DisplayName);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }

        public override bool IsValid(ITypeDescriptorContext context, object value)
        {
            int depth = (int)value;
            return (depth >= 0 && depth <= 100);
        }
    }
}
