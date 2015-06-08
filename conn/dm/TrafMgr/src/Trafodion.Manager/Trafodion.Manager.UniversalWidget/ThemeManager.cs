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
using System.Text;
using System.Drawing;
using ZedGraph;

namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// Manages all themes in the system
    /// </summary>
    public class ThemeManager
    {
        private static ThemeManager _theInstance = new ThemeManager();
        private Dictionary<string, ChartTheme> _theThemes = new Dictionary<string, ChartTheme>();
        public static ThemeManager Instance
        {
            get { return _theInstance; }
            set { _theInstance = value; }
        }

        /// <summary>
        /// Add a theme to the theme library
        /// </summary>
        /// <param name="aTheme"></param>
        public void AddTheme(ChartTheme aTheme)
        {
            _theThemes.Add(aTheme.ThemeName, aTheme);
        }

        /// <summary>
        /// Given a theme name returns the theme
        /// </summary>
        /// <param name="aThemeName"></param>
        public ChartTheme GetTheme(string aThemeName)
        {
            return (_theThemes.ContainsKey(aThemeName)) ? _theThemes[aThemeName] : null;
        }



        private ThemeManager()
        {
            AddTheme(new DefaultLineChartTheme());
        }
    }

    /// <summary>
    /// Maintains the visual attributes of a chart
    /// </summary>
    public abstract class ChartTheme
    {
        public ChartTheme()
        {
        }

        protected string _theThemeName;
        
        protected SymbolType[] _theSymbolTypes;
        protected Fill[] _theSymbolFills;
        protected Color[] _theLineColors;
        protected Fill[] _theLineFills;
        protected Fill _theChartFill;

        /// <summary>
        /// Unique name associated with each theme
        /// </summary>
        public string ThemeName
        {
            get { return _theThemeName; }
            set { _theThemeName = value; }
        }

        /// <summary>
        /// The symbols that would be associated to each line of the chart
        /// </summary>
        public SymbolType[] SymbolTypes
        {
            get { return _theSymbolTypes; }
            set { _theSymbolTypes = value; }
        }

        /// <summary>
        /// Describes how each symbol will be filled
        /// </summary>
        public Fill[] SymbolFills
        {
            get { return _theSymbolFills; }
            set { _theSymbolFills = value; }
        }

        /// <summary>
        /// The colors associated with each line
        /// </summary>
        public Color[] LineColors
        {
            get { return _theLineColors; }
            set { _theLineColors = value; }
        }

        /// <summary>
        /// Describes how each line will be filled
        /// </summary>
        public Fill[] LineFills
        {
            get { return _theLineFills; }
            set { _theLineFills = value; }
        }

        /// <summary>
        /// The background fill of the chart
        /// </summary>
        public Fill ChartFill
        {
            get { return _theChartFill; }
            set { _theChartFill = value; }
        }
    }


    public class DefaultLineChartTheme : ChartTheme
    {
        public static string DefaultLineChartThemeName = "DefaultLineChartTheme";

        public DefaultLineChartTheme()
        {
            _theThemeName = DefaultLineChartThemeName;
            SymbolTypes = new SymbolType[] { 
                SymbolType.Default, 
                SymbolType.Circle, 
                SymbolType.Square, 
                SymbolType.Diamond, 
                SymbolType.Plus, 
                SymbolType.Star,
                SymbolType.Triangle, 
                SymbolType.TriangleDown, 
                SymbolType.VDash, 
                SymbolType.HDash, 
                SymbolType.XCross };

            SymbolFills = new Fill[] { 
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White)};

            LineColors = new Color[] { 
                Color.Black,
                Color.Red,
                Color.Green,
                Color.Blue,
                Color.Brown,
                Color.Yellow,
                Color.Orange,
                Color.Violet,
                Color.Indigo,
                Color.Gold,
                Color.Silver};

            LineFills = new Fill[] { 
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White),
                new Fill(Color.White)};

            ChartFill = new Fill(Color.LightBlue, Color.White);
        }
    }   
    

}
