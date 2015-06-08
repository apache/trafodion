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
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.LiveFeedFramework.Models;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    /// <summary>
    /// Sample user control for using protobuf format live feed data provider
    /// </summary>
    public partial class DisplayProtoBufsUserControl : UserControl, IDataDisplayControl
    {
        #region Fields

        private string[] _theSubscriptions = null;
        private DataProvider _dataProvider = null;
        private UniversalWidgetConfig _widgetConfig = null;
        private IDataDisplayHandler _dataDisplayHandler = null;
        private Dictionary<string, TrafodionIGrid> _theGrids = new Dictionary<string, TrafodionIGrid>();
        private Dictionary<string, TrafodionTabPage> _thePages = new Dictionary<string,TrafodionTabPage>();
        private Dictionary<string, DataTable> _theTables = new Dictionary<string, DataTable>();
        private Dictionary<string, bool> _theCountControlAdded = new Dictionary<string, bool>();
        private int _theCacheSize = 120;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: DataProvider
        /// The data provider of this widget.
        /// </summary>
        public DataProvider DataProvider
        {
            get { return _dataProvider; }
            set { _dataProvider = value; }
        }

        /// <summary>
        /// Property: UniversalWidgetConfiguration
        /// The widget's configuration
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _widgetConfig; }
            set { _widgetConfig = value; }
        }

        /// <summary>
        /// Property: DataDisplayHandler
        /// The widget's data display handler if there is any
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _dataDisplayHandler; }
            set { _dataDisplayHandler = value; }
        }

        /// <summary>
        /// Property: DrillDownManager
        /// There is no drill down manager at this time. 
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }
        #endregion Properties

        #region Constructor

        /// <summary>
        /// constructor
        /// </summary>
        /// <param name="aSubscriptions"></param>
        /// <param name="aSize"></param>
        public DisplayProtoBufsUserControl(string[] aSubscriptions, int aSize)
        {
            _theSubscriptions = aSubscriptions;
            _theCacheSize = aSize;
            InitializeComponent();
            ShowWidgets();
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// not used; but interface requires it
        /// </summary>
        public void PersistConfiguration()
        {
        }

        /// <summary>
        /// To load new data to grid
        /// </summary>
        /// <param name="aDataStats"></param>
        public void LoadNewData(Dictionary<string, List<object>> aDataStats)
        {
            foreach (string key in aDataStats.Keys)
            {
                List<object> stats = aDataStats[key];
                if (stats.Count > 0)
                {
                    DataTable table = null;
                    _theTables.TryGetValue(key, out table);

                    foreach (object obj in stats)
                    {
                        if (table == null)
                        {
                            table = LiveFeedStatsTransformer.TransformStatsToDataTable(key, obj);
                            _theTables.Add(key, table);
                        }
                        else
                        {
                            if (table.Rows.Count == _theCacheSize)
                            {
                                table.Rows.RemoveAt(0);
                            }
                            LiveFeedStatsTransformer.TransformStatsToDataTable(key, obj, table);
                        }
                    }

                    TrafodionIGrid grid = null;
                    _theGrids.TryGetValue(key, out grid);
                    if (grid == null)
                    {
                        grid = new TrafodionIGrid();
                        grid.AddCountControlToParent("There is 0 entry", DockStyle.Top);
                        _theGrids.Add(key, grid);
                    }

                    grid.BeginUpdate();
                    grid.FillWithData(table);
                    grid.EndUpdate();
                    grid.ResizeGridColumns(table);

                    string gridHeaderText;
                    if (grid.Rows.Count == 0)
                    {
                        gridHeaderText = string.Format(Properties.Resources.TotalEntryCount,
                                                        "There is 0 entry",
                                                        Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(DateTime.Now));
                    }
                    else if (grid.Rows.Count == 1)
                    {
                        gridHeaderText = string.Format(Properties.Resources.TotalEntryCount,
                                                        "There is 1 entry",
                                                        Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(DateTime.Now));
                    }
                    else
                    {
                        gridHeaderText = string.Format(Properties.Resources.TotalEntryCount,
                                                        String.Format("There are {0} entries, {1} in last publication ",
                                                        table.Rows.Count, stats.Count),
                                                        Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(DateTime.Now));
                    }

                    if (_theCountControlAdded[key])
                    {
                        grid.UpdateCountControlText(gridHeaderText);
                    }
                    else
                    {
                        _theCountControlAdded[key] = true;
                        grid.AddCountControlToParent(gridHeaderText, DockStyle.Top);
                    }
                }
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To display the widgets
        /// </summary>
        private void ShowWidgets()
        {
            foreach (string sub in _theSubscriptions)
            {
                TrafodionIGrid grid = new TrafodionIGrid();
                grid.Dock = DockStyle.Fill;
                _theGrids.Add(sub, grid);
                _thePages.Add(sub, AddToTabControl(grid, sub));
                _theCountControlAdded.Add(sub, false);
            }
        }

        /// <summary>
        /// Helper method to add controls into a tab into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aUserControl"></param>
        private TrafodionTabPage AddToTabControl(TrafodionIGrid aGrid, string aTabText)
        {
            // Create the tab page with the user control dock filled
            TrafodionTabPage theTabPage = new TrafodionTabPage(aTabText);
            aGrid.Dock = DockStyle.Fill;
            theTabPage.Controls.Add(aGrid);
            _theTabControl.TabPages.Add(theTabPage);
            return theTabPage;
        }

        #endregion Private methods
    }
}
