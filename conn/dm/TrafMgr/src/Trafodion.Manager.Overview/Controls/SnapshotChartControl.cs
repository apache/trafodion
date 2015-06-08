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
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class SnapshotChartControl : UserControl
    {
        #region Fields
        private static readonly string RepositoryConfigName = "Repository_Widget";

        private DateTime _theTimestamp;
        private SystemMetricModel.SystemMetrics _theMetric;
        private TrafodionIGrid _snapshotIGrid = null;

        private ConnectionDefinition _theConnectionDefinition;

        private UniversalWidgetConfig _widgetConfig = null;
        private GenericUniversalWidget repositoryWidget;
        private RepositoryDataProvider _dataProvider = null;

        //public delegate void loadTriage();
        public event EventHandler OnLoadTriage;

        #endregion Fields


        #region Properties

        /// <summary>
        /// Property: Chart - the MS Chart itself
        /// </summary>
        public Chart Chart
        {
            get { return _theChart; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
        }

        public DateTime TheTimeStamp 
        {
            get { return _theTimestamp; }
            set { _theTimestamp = value; }
        }

        public SystemMetricModel.SystemMetrics TheMetric
        {
            get { return _theMetric; }
            set { _theMetric = value; }
        }

        public TrafodionIGrid SnapshotIGrid 
        {
            get { return _snapshotIGrid; }
        }

        #endregion Properties

        #region Constructor
        public SnapshotChartControl() 
        {
            InitializeComponent();
        }

        public SnapshotChartControl(ConnectionDefinition aConnectionDefinition, DateTime aTimestamp, SystemMetricModel.SystemMetrics aMetric)
        {
            this._theConnectionDefinition = aConnectionDefinition;
            this._theTimestamp = aTimestamp;
            this._theMetric = aMetric;
            InitializeComponent();
            _theChart.DoubleClick += new EventHandler(_theChart_DoubleClick);
        }

        void _theChart_DoubleClick(object sender, EventArgs e)
        {

            if (_theConnectionDefinition.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded)
            {
                ShowWidget();
                string TrafodionTime = Utilities.GetTrafodionSQLDateTime(_theTimestamp);
                switch (_theMetric)
                {
                    case SystemMetricModel.SystemMetrics.Core:
                        ((DatabaseDataProviderConfig)_dataProvider.DataProviderConfig).SQLText = _dataProvider.sqltext1;
                        break;
                    case SystemMetricModel.SystemMetrics.Memory:
                    case SystemMetricModel.SystemMetrics.Swap:
                    case SystemMetricModel.SystemMetrics.File_System:
                        ((DatabaseDataProviderConfig)_dataProvider.DataProviderConfig).SQLText = _dataProvider.sqltext3;
                        break;
                    case SystemMetricModel.SystemMetrics.Load_Avg:
                        break;
                    case SystemMetricModel.SystemMetrics.Disk:
                        ((DatabaseDataProviderConfig)_dataProvider.DataProviderConfig).SQLText = string.Format(_dataProvider.sqltext2, TrafodionTime, TrafodionTime);
                        break;
                    case SystemMetricModel.SystemMetrics.Network_Rcv:
                        break;
                    case SystemMetricModel.SystemMetrics.Network_Txn:
                        break;
                    case SystemMetricModel.SystemMetrics.Virtual_Memory:
                        break;
                    default:
                        break;
                }

                _dataProvider.Start();
            }
            else 
            {
                MessageBox.Show("Please connect to the system.");
            }            
        }


        #endregion Constructor

        #region Private Methods

        private void ShowWidget()
        {
            _widgetConfig = WidgetRegistry.GetConfigFromPersistence(RepositoryConfigName);
            if (_widgetConfig == null)
            {
                _widgetConfig = new UniversalWidgetConfig();
                _widgetConfig.Name = RepositoryConfigName;
                _widgetConfig.Title = "Repository";
                _widgetConfig.ShowProperties = false;
                _widgetConfig.ShowToolBar = true;
                _widgetConfig.ShowChart = false;
                _widgetConfig.ShowRowCount = true;
                _widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }
            DatabaseDataProviderConfig dbConfig = new DatabaseDataProviderConfig();
            if (_widgetConfig.DataProviderConfig == null)
            {
                _widgetConfig.DataProviderConfig = dbConfig;
                _widgetConfig.DataProviderConfig.RefreshRate = 30;
            }
            else
            {
                dbConfig = (DatabaseDataProviderConfig)_widgetConfig.DataProviderConfig;
            }

            dbConfig.ConnectionDefinition = _theConnectionDefinition;

            //_dataProvider = (RepositoryDataProvider)dbConfig.GetDataProvider();
            _dataProvider = new RepositoryDataProvider(dbConfig);
            //Create the Universal widget to diaplay the data
            repositoryWidget = new GenericUniversalWidget();
            repositoryWidget.DataProvider = _dataProvider;
            repositoryWidget.UniversalWidgetConfiguration = _widgetConfig;
            repositoryWidget.Dock = DockStyle.Fill;

            TabularDataDisplayControl dataDisplayControl = (TabularDataDisplayControl)repositoryWidget.DataDisplayControl;
            _snapshotIGrid = dataDisplayControl.DataGrid;

            _snapshotIGrid.RowMode = true;

            TrafodionIGridToolStripMenuItem _loadTriageMenuItem = new TrafodionIGridToolStripMenuItem();
            _loadTriageMenuItem.Text = "Load Triage...";
            _loadTriageMenuItem.Enabled = true;
            _loadTriageMenuItem.Click += new EventHandler(loadTriageButton_Click);
            dataDisplayControl.AddMenuItem(_loadTriageMenuItem);

            //ToolStripButton _loadTriageButton = new ToolStripButton();
            //_loadTriageButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            //_loadTriageButton.Image = (System.Drawing.Image)Trafodion.Manager.Properties.Resources.LoadTriageIcon;
            //_loadTriageButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            //_loadTriageButton.Name = "loadTriageButton";
            //_loadTriageButton.Size = new System.Drawing.Size(23, 22);
            //_loadTriageButton.Text = "Load Triage";
            //_loadTriageButton.ToolTipText = "Load the Triage Space with the selected queries";
            //_loadTriageButton.Click += new EventHandler(loadTriageButton_Click);
            //_loadTriageButton.Enabled = false;  
            //repositoryWidget.AddToolStripItem(_loadTriageButton);


            this._gridPanel.Controls.Add(repositoryWidget);
            this._gridPanel.Visible = true;
            this.Height += 300;
        }

        void loadTriageButton_Click(object sender, EventArgs e)
        {
            EventHandler loadtriage = OnLoadTriage;
            if (loadtriage != null) 
            {
                OnLoadTriage(this, e);
            }
            
        }

        #endregion Private Methods
    }
}
