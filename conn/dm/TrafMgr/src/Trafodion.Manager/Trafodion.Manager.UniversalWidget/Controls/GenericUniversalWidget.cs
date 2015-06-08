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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;


namespace Trafodion.Manager.UniversalWidget.Controls
{
   
    public partial class GenericUniversalWidget : UserControl
    {
        private UniversalWidgetConfig       _theConfig;
        private UniversalWidgetContext      _theUniversalWidgetContext;
        private IDataDisplayControl         _theDataDisplayControl;
        private MSChartControl              _theChartControl = null;
        private GenericConfigurationPanel   _theConfigPanel;
        private ExceptionControl            _theExceptionControl;
        private OutputControl               _theConsoleControl;
        private DataProvider                _theDataProvider;
        private WidgetContainer             _theWidgetContainer;
        private Persistence.PersistenceHandler _thePersistenceHandler;
        private ILogger _theLogger;
        private bool _currentChartVisibility = false;
        private UniversalWidgetConfig.ChartPositions _currentChartPosition = UniversalWidgetConfig.ChartPositions.RIGHT;
        public delegate void UpdateStatus(Object obj, EventArgs e);
        public delegate void UpdateTimerStatus(Object obj, TimerEventArgs e);

        /// <summary>
        /// Define a chart configuration change event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public delegate void ChartConfigurationChanged(object sender, EventArgs e);
        public event ChartConfigurationChanged OnChartConfigurationChanged;

        public GenericUniversalWidget(UniversalWidgetConfig aConfig)
            : this()
        {
            UniversalWidgetConfiguration = aConfig;
        }
        public GenericUniversalWidget()
        {
            InitializeComponent();

            _thePersistenceHandler = new Persistence.PersistenceHandler(Persistence_PersistenceHandlers);
            Persistence.PersistenceHandlers += _thePersistenceHandler;

            //By default we display tablular data. This can be overridden by the widget creator
            DataDisplayControl = new Trafodion.Manager.UniversalWidget.Controls.TabularDataDisplayControl();

            //Default setting for the timer related display
            this._theRefreshTimerButton.Image = Properties.Resources.player_play;
            this._theRefreshTimerButton.ToolTipText = "Resume timer";
            this._theRefreshTimerButton.Tag = "pause_mode";
            this._theRefreshLabel.Text = TimerEventArgs.EventTypes[(int)TimerEventArgs.TimerEventTypes.Paused];
            this._theRefreshButton.Enabled = false;
            this._theExportButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.Export;
            this._theExportButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            createExportSubmenus();
            CreateShowChartSubmenus();

            //By default hide the error page. It will be shown if there is an error
            _theErrorTabPage.Hide();

            //For now remove the parameter and documentation tab pages
            this._theLowerTabControl.TabPages.Remove(_theDocumentationTabPage);
            this._theLowerTabControl.TabPages.Remove(_theParameterTabPage);


            _theExceptionControl = new ExceptionControl();
            _theExceptionControl.Dock = DockStyle.Fill;
            _theErrorTabPage.Controls.Add(_theExceptionControl);

            _theConsoleControl = new OutputControl();
            _theConsoleControl.Dock = DockStyle.Fill;
            _theOutputTab.Controls.Add(_theConsoleControl);

            //Force all of the inner controls creation. 
            this.CreateControl();

            Logger = new DefaultLogger();
        }

        public ILogger Logger
        {
            get { return _theLogger; }
            set
            {
                //Remove the handlers from the old logger
                if (_theLogger != null)
                {
                    if (_theLogger is DefaultLogger)
                    {
                        ((DefaultLogger)_theLogger).OnLogData -= _theConsoleControl.LogData;
                    }
                }

                //set the new logger
                _theLogger = value;

                //Make the console the event handler for the logger
                if (_theLogger is DefaultLogger)
                {
                    ((DefaultLogger)_theLogger).OnLogData += _theConsoleControl.LogData;
                }

            }
        }

        /// <summary>
        /// Set/Gets the data provider that is associated with the Universal widget.
        /// The DataProvider will be responsible from getting the data to display in the UW
        /// </summary>
        public DataProvider DataProvider
        {
            get { return _theDataProvider; }
            set 
            {
                //Remove the handlers from the old data provider 
                if (_theDataProvider != null)
                {
                    RemoveEventHandlers();
                }
                
                //Set the data provider
                _theDataProvider = value;
                
                //Add event handlers to the new provider
                AddEventHandlers();

                //Associate the provider to the config
                if (UniversalWidgetConfiguration != null)
                {
                    UniversalWidgetConfiguration.DataProvider = _theDataProvider;
                }

                //Associate the provider to the datadisplay control
                if (_theDataDisplayControl != null)
                {
                    _theDataDisplayControl.DataProvider = _theDataProvider;
                }

                if (_theChartControl != null)
                {
                    _theChartControl.DataProvider = _theDataProvider;
                }
                _theExceptionControl.DataProvider = _theDataProvider;
                _theConsoleControl.DataProvider = _theDataProvider;
            }
        }

        /// <summary>
        /// This is the view for the universal widget. The UW will use this user control to 
        /// display the data provided by the data provider.
        /// </summary>
        public IDataDisplayControl DataDisplayControl
        {
            get 
            {
                return _theDataDisplayControl; 
            }
            
            set 
            {
                //If we have an existing display control we remove it from the UI
                UserControl dataControl = _theDataDisplayControl as UserControl;
                if (dataControl != null)
                {
                    GetDataPanel().Controls.Remove(dataControl);
                }

                //Make this the current one
                _theDataDisplayControl = value;
                dataControl = _theDataDisplayControl as UserControl;
                if (dataControl != null)
                {
                    dataControl.Dock = DockStyle.Fill;
                    GetDataPanel().Controls.Add(dataControl);
                }
                if (UniversalWidgetConfiguration != null)
                {
                    _theDataDisplayControl.UniversalWidgetConfiguration = UniversalWidgetConfiguration;
                }
                if (DataProvider != null)
                {
                    _theDataDisplayControl.DataProvider = DataProvider;
                }
            }
        }


        /// <summary>
        /// The configuration tht will be used to create the universal widget.
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get 
            {
                return _theConfig;             
            }

            set 
                {

                _theConfig = value;
                this._currentChartVisibility = value.ShowChart;
                this._currentChartPosition = value.ChartPosition;
                
                //Set the configuration to the data display control
                if (_theDataDisplayControl != null)
                {
                    _theDataDisplayControl.UniversalWidgetConfiguration = value;
                }

                if (_theConfig != null && _theConfig.SupportCharts)
                {
                    if (_theChartControl == null)
                    {
                        // Chart support has just been turned on
                        _theChartControl = new MSChartControl(value);
                        _theChartControl.Dock = DockStyle.Fill;
                        GetChartPanel().Controls.Add(_theChartControl);
                        _theDisplayPanel.SplitterMoved += DisplayPanel_SplitterMoved;
                        _theChartControl.DataProvider = DataProvider;
                    }
                }
                else
                {
                    if (_theChartControl != null)
                    {
                        // Chart has just been turned off
                        GetChartPanel().Controls.Add(_theChartControl);
                        _theDisplayPanel.SplitterMoved -= DisplayPanel_SplitterMoved;
                        _theChartControl = null;
                    }
                }

                //if the chart config is not there we will add a default one here
                if (_theConfig.SupportCharts && _theConfig.ChartConfig == null)
                {
                    _theConfig.ChartConfig = new ChartConfig();
                }

                //Set the configuration panel
                _theConfigPanel = new GenericConfigurationPanel();
                _theConfigPanel.Dock = DockStyle.Fill;
                this._theConfigurationContainer.Controls.Add(_theConfigPanel);
                _theConfigPanel.Configuration = value;

                //based on the config shows the appropriate panels
                showAppropriatePanels();

                //Get the appropriate data provider
                if (_theDataProvider == null)
                {
                    _theDataProvider = DataProviderFactory.GetDataProvider(UniversalWidgetConfiguration.DataProviderConfig);
                    //Associate the event handlers
                    AddEventHandlers();
                }
                //set the default menu
                setDefaultRefreshRate(UniversalWidgetConfiguration.DataProviderConfig.RefreshRate);
                UniversalWidgetConfiguration.DataProvider = _theDataProvider;
                //Set the configuration to the data display control
                if (_theDataDisplayControl != null)
                {
                    _theDataDisplayControl.DataProvider = _theDataProvider;
                }

                if (_theChartControl != null)
                {
                    _theChartControl.DataProvider = _theDataProvider;
                }

                //Add the data exception panel listeners
                _theExceptionControl.DataProvider = _theDataProvider;
                _theConsoleControl.DataProvider = _theDataProvider;

                SetChartPosition(_theConfig.ChartPosition);
            }
        }


        public UniversalWidgetContext UniversalWidgetContext
        {
            get { return _theUniversalWidgetContext; }
            set { _theUniversalWidgetContext = value; }
        }

        /// <summary>
        /// The WidgetContainer that will house this widget. This gets set when the
        /// widget is placed on the canvas.
        /// </summary>
        public WidgetContainer WidgetContainer
        {
            get { return _theWidgetContainer; }
            set { _theWidgetContainer = value; }
        }

        public ToolStripTextBox RowCountTextBox
        {
            get { return _theRowCount; }
        }

        /// <summary>
        /// Property: Chartcontrol - the MS Chart control in the widget if charts are supported.
        /// Note: this could be a null if charts are not supported by the widget.  So, caller would
        ///       have to validate it.
        /// </summary>
        public MSChartControl ChartControl
        {
            get { return _theChartControl; }
        }

        /// <summary>
        /// Helper method to update the config title
        /// </summary>
        /// <param name="aTitle"></param>
        public void SetTitle(string aTitle)
        {
            //if (this.UniversalWidgetConfiguration != null)
            //{
            //    this.UniversalWidgetConfiguration.Title = aTitle;
            //}
            if (this.WidgetContainer != null)
            {
                this.WidgetContainer.Text = aTitle;
            }
        }

        /// <summary>
        /// Determine Chart location: first & second
        /// </summary>
        private bool IsChartControlFirst
        {
            get
            {                
                bool isChartControlFirst = _currentChartPosition == UniversalWidgetConfig.ChartPositions.TOP ||
                                           _currentChartPosition == UniversalWidgetConfig.ChartPositions.LEFT;
                return isChartControlFirst;
            }
        }

        public void StartDataProvider()
        {
            if (UniversalWidgetConfiguration != null)
            {
                //start the data provider after the widgets have associated themselves with it
                UniversalWidgetConfiguration.DataProviderConfig.MaxRowCount = RowCount;
                _theDataProvider.Start(_theConfig.PassedParameters);
            }
        }

        /// <summary>
        /// There might be a need to detach the tool bar from the UI so that this same
        /// control can be used in a parent control 
        /// </summary>
        /// <returns></returns>
        public ToolStrip GetDetachedToolStrip()
        {
            if (this.Controls.Contains(_theToolStrip))
            {
                this.Controls.Remove(_theToolStrip);
            }
            return _theToolStrip;
        }

        /// <summary>
        /// Populate the chart with the given datatable if the widget supports charts.
        /// </summary>
        /// <param name="aDataTable"></param>
        public void PopulateChart(DataTable aDataTable)
        {
            if (this._theChartControl != null && aDataTable != null)
            {
                this._theChartControl.PopulateChart(aDataTable);
            }
        }

        public ToolStripProgressBar StatusProgressBar
        {
            get
            {
                return _theStatusProgressBar;
            }
        }

        public ToolStripLabel StatusLabel
        {
            get
            {
                return _theStatusLabel;
            }
        }

        /// <summary>
        /// This will attach the tool bar to this control;
        /// </summary>
        public void ReAttachToolStrip()
        {
            if (! this.Controls.Contains(_theToolStrip))
            {
                this.Controls.Add(_theToolStrip);
            }
        }

        /// <summary>
        /// Adds a new toolstrip item
        /// </summary>
        /// <param name="aTrafodionIGridToolStripMenuItem"></param>
        public void AddToolStripItem(ToolStripItem aToolstripItem)
        {
            this._theToolStrip.Items.Add(aToolstripItem);
            _theCustomToolStripButtonSeparator.Visible = true;
        }


        public int RowCount
        {
            get
            {
                int rowCount = -1;
                if (_theRowCount.Visible)
                {
                    try
                    {
                        rowCount = int.Parse(_theRowCount.Text);
                    }
                    catch (Exception ex)
                    {
                    }
                }
                return rowCount;
            }

            set
            {
                _theRowCount.Text = value + "";
            }
        }

        protected ToolStrip TheToolStrip
        {
            get { return _theToolStrip; }
        }

        protected ToolStripButton TheRefreshButton
        {
            get { return _theRefreshButton; }
        }

        protected ToolStripButton TheRefreshTimerButton
        {
            get { return _theRefreshTimerButton; }
        }

        protected ToolStripButton TheStopQueryButton
        {
            get { return _theStopQuery; }
        }

        protected ToolStripDropDownButton TheExportButton
        {
            get { return _theExportButton; }
        }

        protected ToolStripButton TheHelpButton
        {
            get { return _theHelpButton; }
        }

        protected ToolStripDropDownButton TheTimerSetupButton
        {
            get { return _theTimerSetupButton; }
        }

        protected ToolStripDropDownButton TheShowGraphButton
        {
            get { return _theShowGraphButton; }
        }

        /// <summary>
        /// The persistence handler to persist the configurations associated with the UW.
        /// 
        /// TODO: Since we have a Hashtable to store different configs we can have thread contentions. 
        /// Need to revesit this.
        /// </summary>
        /// <param name="aDictionary"></param>
        /// <param name="aPersistenceOperation"></param>
        void Persistence_PersistenceHandlers(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aPersistenceOperation)
        {
            if ((aPersistenceOperation == Persistence.PersistenceOperation.Save) && (_theConfig != null) && (_theConfig.Name != null))
            {
                if (_theDataDisplayControl != null)
                {
                    _theDataDisplayControl.PersistConfiguration();
                }
                PersistConfiguration();
            }
        }


        void DisplayPanel_SplitterMoved(object sender, SplitterEventArgs e)
        {
            TrafodionSplitContainer splitter = (TrafodionSplitContainer)sender;
            _theConfig.ChartSplitterDistance = splitter.SplitterDistance;
        }

        public void PersistConfiguration()
        {
            try
            {
                //Under the UniversalWidgetPersistenceKey, we have a hashtable that has the name of the widget and the config
                //associated to that widget.
                Hashtable uwPersistence = Trafodion.Manager.Framework.Persistence.Get(UniversalWidgetConfig.UniversalWidgetPersistenceKey) as Hashtable;

                //This is the first time when a UW is being persisted
                if (uwPersistence == null)
                {
                    uwPersistence = new Hashtable();
                    Trafodion.Manager.Framework.Persistence.Put(UniversalWidgetConfig.UniversalWidgetPersistenceKey, uwPersistence);
                }

                //Add the config to the persistence
                if (uwPersistence[_theConfig.Name] != null)
                {
                    uwPersistence.Remove(_theConfig.Name);
                }
                uwPersistence.Add(_theConfig.Name, UniversalWidgetConfiguration);
            }
            catch (Exception)
            {
            }
        }

        /// <summary>
        /// Associate all the event handlers
        /// </summary>
        private void AddEventHandlers()
        {
            if (_theDataProvider != null)
            {
                _theDataProvider.OnErrorEncountered += InvokeHandleError;
                _theDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _theDataProvider.OnFetchingData += InvokeHandleFetchingData;
                _theDataProvider.OnFetchCancelled += InvokeHandleFetchCancelled;
                _theDataProvider.OnTimerStateChanged += InvokeHandleTimerStateChanged;
                _theDataProvider.OnInitDataproviderForFetch += InvokeHandleInitDataproviderForFetch;
                _theDataProvider.OnBeforeFetchingData +=InvokeHandleOnBeforeFetchingData;
            }
        }

        /// <summary>
        /// Remove all the event handlers
        /// </summary>
        private void RemoveEventHandlers()
        {
            if (_theDataProvider != null)
            {
                _theDataProvider.OnErrorEncountered -= InvokeHandleError;
                _theDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theDataProvider.OnFetchingData -= InvokeHandleFetchingData;
                _theDataProvider.OnFetchCancelled -= InvokeHandleFetchCancelled;
                _theDataProvider.OnTimerStateChanged -= InvokeHandleTimerStateChanged;
                _theDataProvider.OnInitDataproviderForFetch -= InvokeHandleInitDataproviderForFetch;
                _theDataProvider.OnBeforeFetchingData -= InvokeHandleOnBeforeFetchingData;
            }
        }
        
        /// <summary>
        /// Shows/hides panels, buttons, etc based on the configuration
        /// </summary>
        private void showAppropriatePanels()
        {
            //Based on the configuration display the status and properties
            if (_theConfig.ChartSplitterDistance != -1)
            {
                _theDisplayPanel.SplitterDistance = _theConfig.ChartSplitterDistance;
            }

            // Determine Chart location: first & second
            bool isChartControlFirst = _currentChartPosition == UniversalWidgetConfig.ChartPositions.TOP ||
                                       _currentChartPosition == UniversalWidgetConfig.ChartPositions.LEFT;
            if (isChartControlFirst)
            {
                _theDisplayPanel.Panel2Collapsed = !_theConfig.ShowTable;
                _theDisplayPanel.Panel1Collapsed = !this._currentChartVisibility;
            }
            else
            {
                _theDisplayPanel.Panel1Collapsed = !_theConfig.ShowTable;
                _theDisplayPanel.Panel2Collapsed = !this._currentChartVisibility;
            }

            TrafodionSplitContainer1.Panel2Collapsed = !_theConfig.ShowProviderStatus;
            TrafodionSplitContainer2.Panel2Collapsed = !_theConfig.ShowProperties;


            _theConnectionLabel.Visible = _theConfig.ShowConnections;
            _theConnections.Visible = _theConfig.ShowConnections;
            _theCatalogLabel.Visible = _theConfig.ShowCatalogs;
            _theCatalogs.Visible = _theConfig.ShowCatalogs;
            _theSchemaLabel.Visible = _theConfig.ShowSchemas;
            _theSchemas.Visible = _theConfig.ShowSchemas;
            _theRowCount.Visible = _theConfig.ShowRowCount;
            _theRowCountLabel.Visible = _theConfig.ShowRowCount;

            _theToolStrip.Visible = _theConfig.ShowToolBar;
            _theShowGraphButton.Visible = _theConfig.ShowChartToolBarButton;
            _theShowDetails.Visible = _theConfig.ShowProviderToolBarButton;
            _theShowProperties.Visible = _theConfig.ShowPropertiesToolBarButton;
            _theGraphDetailsSplitter.Visible = (_theConfig.ShowChartToolBarButton || _theConfig.ShowProviderToolBarButton);
            _thePropertiesSplitter.Visible = _theConfig.ShowPropertiesToolBarButton;

            _theSplitter1.Visible = (_theConfig.ShowConnections || _theConfig.ShowCatalogs || _theConfig.ShowSchemas);


            //the refresh timer stuff
            _theTimerSetupButton.Visible = _theConfig.ShowTimerSetupButton;
            _theRefreshTimerButton.Visible = (_theConfig.ShowRefreshTimerButton) && (_theConfig.DataProviderConfig.RefreshRate > 0);
            _theRefreshLabel.Visible = (_theConfig.DataProviderConfig.RefreshRate > 0);
            _theRefreshButton.Visible = _theConfig.ShowRefreshButton;

            //The export button
            _theExportButton.Visible = _theConfig.ShowExportButtons;
            _theExportButtonSeparator.Visible = _theConfig.ShowExportButtons;// && (_theConfig.ShowChartToolBarButton || _theConfig.ShowProviderToolBarButton);

            //the help button
            _theHelpButton.Visible = _theConfig.ShowHelpButton;
        }

        private void _theHelpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(_theConfig.HelpTopic);
        }

        private void createExportSubmenus()
        {
            _theExportButton.DropDownItems.Add(getToolStripMenuItemForExport("Clipboard", 1));
            _theExportButton.DropDownItems.Add(getToolStripMenuItemForExport("Browser", 2));
            _theExportButton.DropDownItems.Add(getToolStripMenuItemForExport("Spreadsheet", 3));
            _theExportButton.DropDownItems.Add(getToolStripMenuItemForExport("File", 4));            
        }

        protected ToolStripMenuItem getToolStripMenuItemForExport(string text, int type)
        {
            ToolStripMenuItem exportMenu = new ToolStripMenuItem(text);
            exportMenu.Tag = type;
            exportMenu.Name = text;
            exportMenu.Click += new EventHandler(exportMenu_Click);
            return exportMenu;
        }

        /// <summary>
        /// Temporarily hide Chart panel for Explain Mode, so the Chart Visibility status should not be record.
        /// When switch to Execute mode, the Chart Visibility should be restored to original.
        /// </summary>
        public void HideChartPanel()
        {
            SetChartVisibility(false);
        }

        /// <summary>
        /// Fill in current Chart status:
        /// e.g. fill into UniversalWidgetConfig for persistence purpose
        /// </summary>
        public void FillInCurrentChartStatus(UniversalWidgetConfig config)
        {
            config.ShowChart = this._currentChartVisibility;
            config.ChartPosition = this._currentChartPosition;
        }

        void exportMenu_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem exportMenu = sender as ToolStripMenuItem;
            //Currently we only export for TabularDataDisplayControl. We might want to extend the framework
            //to allow export for other display controls as well.
            if (DataDisplayControl is TabularDataDisplayControl)
            {
                TabularDataDisplayControl dataDisplayControl = DataDisplayControl as TabularDataDisplayControl;
                if ((exportMenu != null) && (dataDisplayControl != null) && ((dataDisplayControl.DataGrid != null)))
                {
                    int exportType = (int)exportMenu.Tag;
                    switch (exportType)
                    {
                        case 1:
                            {
                                dataDisplayControl.DataGrid.ExportToClipboard();
                            }
                            break;
                        case 2:
                            {
                                dataDisplayControl.DataGrid.ExportToBrowser();
                            }
                            break;
                        case 3:
                            {
                                dataDisplayControl.DataGrid.ExportToSpreadsheet();
                            }
                            break;
                        case 4:
                            {
                                dataDisplayControl.DataGrid.ExportToFile();
                            }
                            break;
                    }
                }
            }
        }

        private void CreateShowChartSubmenus()
        {
            TrafodionToolStripMenuItem showHideGraphMenu = new TrafodionToolStripMenuItem();
            showHideGraphMenu.Name = showHideGraphMenu.Text = "Show/Hide Charts";
            _theShowGraphButton.DropDownItems.Add(showHideGraphMenu);
            AddShowHideSubMenuItem(showHideGraphMenu);

            TrafodionToolStripMenuItem chartDesignerGraphMenu = new TrafodionToolStripMenuItem();
            chartDesignerGraphMenu.Name = chartDesignerGraphMenu.Text = "Chart Designer...";
            chartDesignerGraphMenu.Click += ChartDesignerGraphMenu_Click;
            _theShowGraphButton.DropDownItems.Add(chartDesignerGraphMenu);

            TrafodionToolStripMenuItem printingGraphMenu = new TrafodionToolStripMenuItem();
            printingGraphMenu.Name =  printingGraphMenu.Text = "Printing";
            _theShowGraphButton.DropDownItems.Add(printingGraphMenu);
            printingGraphMenu.DropDownItems.Add(GetToolStripMenuItemForPrintingGraph("Page Setup...", 1));
            printingGraphMenu.DropDownItems.Add(GetToolStripMenuItemForPrintingGraph("Print Preview...", 2));
            printingGraphMenu.DropDownItems.Add(GetToolStripMenuItemForPrintingGraph("Print...", 3));
        }

        private void AddShowHideSubMenuItem(TrafodionToolStripMenuItem miShowHideChart)
        {
            // Add Hide sub menu item
            TrafodionToolStripMenuItem miHide = new TrafodionToolStripMenuItem();
            miHide.Name = "miHide";
            miHide.Text = "&Hide";
            miHide.Click += HideSubMenuItem_Click;
            miShowHideChart.DropDownItems.Add(miHide);

            // Add Separator
            miShowHideChart.DropDownItems.Add(new TrafodionToolStripSeparator());

            // Add Top sub menu item
            TrafodionToolStripMenuItem miTop = new TrafodionToolStripMenuItem();
            miTop.Name = "miTop";
            miTop.Text = "&Top";
            miTop.Click += TopSubMenuItem_Click;
            miShowHideChart.DropDownItems.Add(miTop);

            // Add Bottom sub menu item
            TrafodionToolStripMenuItem miBottom = new TrafodionToolStripMenuItem();
            miBottom.Name = "miBottom";
            miBottom.Text = "&Bottom";
            miBottom.Click += BottomSubMenuItem_Click;
            miShowHideChart.DropDownItems.Add(miBottom);

            // Add Left sub menu item
            TrafodionToolStripMenuItem miLeft = new TrafodionToolStripMenuItem();
            miLeft.Name = "miLeft";
            miLeft.Text = "&Left";
            miLeft.Click += LeftSubMenuItem_Click;
            miShowHideChart.DropDownItems.Add(miLeft);

            // Add Right sub menu item
            TrafodionToolStripMenuItem miRight = new TrafodionToolStripMenuItem();
            miRight.Name = "miRight";
            miRight.Text = "&Right";
            miRight.Click += RightSubMenuItem_Click;
            miShowHideChart.DropDownItems.Add(miRight);
        }

        private void SetChartVisibility(bool isVisible)
        {
            // Determine Chart location: first & second
            bool isChartControlFirst = _currentChartPosition == UniversalWidgetConfig.ChartPositions.TOP ||
                                       _currentChartPosition == UniversalWidgetConfig.ChartPositions.LEFT;

            if (isChartControlFirst)
            {
                _theDisplayPanel.Panel1Collapsed = !isVisible;
            }
            else
            {
                _theDisplayPanel.Panel2Collapsed = !isVisible;
            }
        }

        private void SetChartPosition(UniversalWidgetConfig.ChartPositions chartPosition)
        {
            _currentChartPosition = chartPosition;

            // Set Splitter Horizontal or Vertical
            if ( chartPosition == UniversalWidgetConfig.ChartPositions.TOP ||
                 chartPosition == UniversalWidgetConfig.ChartPositions.BOTTOM )
            {
                SetChartSplitter(Orientation.Horizontal);
            }
            else if ( chartPosition == UniversalWidgetConfig.ChartPositions.LEFT ||
                      chartPosition == UniversalWidgetConfig.ChartPositions.RIGHT)
            {
                SetChartSplitter(Orientation.Vertical);
            }

            SetChartPrecedence(chartPosition);
        }

        private void SetChartPrecedence(UniversalWidgetConfig.ChartPositions chartPosition)
        { 
            // Determine Chart first & second
            bool isChartControlFirst = IsChartControlFirst;

            Control dataControl = _theDataDisplayControl as UserControl;
            Control chartControl = _theChartControl;

            Control firstControl = isChartControlFirst ? chartControl : dataControl;
            Control secondControl = isChartControlFirst ? dataControl : chartControl;

            _theDisplayPanel.Panel1.Controls.Clear();
            if (firstControl != null)
            {
                _theDisplayPanel.Panel1.Controls.Add(firstControl);
            }

            _theDisplayPanel.Panel2.Controls.Clear();
            if (secondControl != null)
            {
                _theDisplayPanel.Panel2.Controls.Add(secondControl);
            }
        }

        /// <summary>
        /// Set Splitter according to the Orientation
        /// </summary>
        /// <param name="splitterOrientation"></param>
        private void SetChartSplitter(Orientation splitterOrientation)
        {
            _theDisplayPanel.Orientation = splitterOrientation;

            switch (splitterOrientation)
            {
                case Orientation.Horizontal:
                    _theDisplayPanel.SplitterDistance = (_theDisplayPanel.Height / 2) - _theDisplayPanel.SplitterWidth;
                    break;

                case Orientation.Vertical:
                    _theDisplayPanel.SplitterDistance = (_theDisplayPanel.Width / 2) - _theDisplayPanel.SplitterWidth;
                    break;
            }

            _theConfig.ChartSplitterDistance = _theDisplayPanel.SplitterDistance;
        }

        /// <summary>
        /// Get the Panel of Chart from SplitterContainer
        /// </summary>
        /// <returns></returns>
        private SplitterPanel GetChartPanel()
        {
            return IsChartControlFirst ? _theDisplayPanel.Panel1 : _theDisplayPanel.Panel2;
        }
        
        /// <summary>
        /// Get the Panel of data grid from SplitterContainer
        /// </summary>
        /// <returns></returns>
        private SplitterPanel GetDataPanel()
        {
            return IsChartControlFirst ? _theDisplayPanel.Panel2 : _theDisplayPanel.Panel1;
        }

        protected TrafodionToolStripMenuItem GetToolStripMenuItemForPrintingGraph(string text, int type)
        {
            TrafodionToolStripMenuItem printMenu = new TrafodionToolStripMenuItem();
            printMenu.Tag = type;
            printMenu.Name = text;
            printMenu.Text = text;
            printMenu.Click += new EventHandler(PrintingMenu_Click);
            return printMenu;
        }

        void PrintingMenu_Click(object sender, EventArgs e)
        {
            TrafodionToolStripMenuItem printingMenu = sender as TrafodionToolStripMenuItem;

            if (_theChartControl != null && _theChartControl.TheChart != null && printingMenu != null)
            {
                int type = (int)printingMenu.Tag;
                try
                {
                    switch (type)
                    {
                        case 1:
                            _theChartControl.TheChart.Printing.PageSetup();
                            break;
                        case 2:
                            _theChartControl.TheChart.Printing.PrintPreview();
                            break;
                        case 3:
                            _theChartControl.TheChart.Printing.Print(true);
                            break;
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(string.Format("Print error - {0}", ex.Message), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void HideSubMenuItem_Click(object sender, EventArgs e)
        {
            this._currentChartVisibility = false;
            SetChartVisibility(this._currentChartVisibility);
        }

        private void TopSubMenuItem_Click(object sender, EventArgs e)
        {
            this._currentChartVisibility = true;
            SetChartVisibility(this._currentChartVisibility);
            SetChartPosition(UniversalWidgetConfig.ChartPositions.TOP);
        }

        private void BottomSubMenuItem_Click(object sender, EventArgs e)
        {
            this._currentChartVisibility = true;
            SetChartVisibility(this._currentChartVisibility);
            SetChartPosition(UniversalWidgetConfig.ChartPositions.BOTTOM);
        }

        private void LeftSubMenuItem_Click(object sender, EventArgs e)
        {
            this._currentChartVisibility = true;
            SetChartVisibility(this._currentChartVisibility);
            SetChartPosition(UniversalWidgetConfig.ChartPositions.LEFT);
        }

        private void RightSubMenuItem_Click(object sender, EventArgs e)
        {
            this._currentChartVisibility = true;
            SetChartVisibility(this._currentChartVisibility);
            SetChartPosition(UniversalWidgetConfig.ChartPositions.RIGHT);
        }

        private void ChartDesignerGraphMenu_Click(object sender, EventArgs e)
        {
            if (DataProvider != null)
            {
                DataTable dataTable = DataProvider.GetDataTable();
                List<string> columns = new List<string>();

                if (dataTable != null)
                {
                    foreach (DataColumn dc in dataTable.Columns)
                    {
                        columns.Add(dc.ColumnName);
                    }
                }
                else if (UniversalWidgetConfiguration != null && 
                         UniversalWidgetConfiguration.DataProviderConfig != null && 
                         UniversalWidgetConfiguration.DataProviderConfig.ColumnMappings != null)
                {
                    foreach (ColumnMapping map in UniversalWidgetConfiguration.DataProviderConfig.ColumnMappings)
                    {
                        columns.Add(map.InternalName);
                    }
                }
                else
                {
                     MessageBox.Show("The report is not ready for chart design.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Information);
                     return;
                }

                ColumnNameConverter.StandardValues = columns.ToArray();
                ChartDesignerDialog dialog = new ChartDesignerDialog(this);
                dialog.DataTable = dataTable;
                if (dialog.ShowDialog() == DialogResult.OK)
                {
                    FireOnChartConfigurationChanged(new EventArgs());
                }
            }
        }

        /// <summary>
        /// Fire the chart configuration change event
        /// </summary>
        /// <param name="e"></param>
        private void FireOnChartConfigurationChanged(EventArgs e)
        {
            if (OnChartConfigurationChanged != null)
            {
                OnChartConfigurationChanged(this, e);
            }
        }

        protected virtual void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (this.IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleError), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        protected virtual void HandleError(Object obj, EventArgs e)
        {
            _theErrorTabPage.Show();

            //Update status bar with 
            this._theStatusLabel.Text = "Error Encountered           ";
            this._theStatusProgressBar.Visible = false;
            _theCancelQueryLabel.Visible = false;
            _theStopQuery.Enabled = false;
            _theRefreshButton.Enabled = !_theStopQuery.Enabled;
            _theRowCount.Enabled = true;
            _theLowerTabControl.SelectedTab = _theErrorTabPage;
            _theConfig.ShowProviderStatus = true;
            ShowStatus(true);// (_theConfig.ShowStatusStrip == UniversalWidgetConfig.ShowStatusStripEnum.Show);
            showAppropriatePanels();
        }

        protected virtual void InvokeHandleFetchCancelled(Object obj, EventArgs e)
        {
            try
            {
                if (this.IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleFetchCancelled), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        protected virtual void HandleFetchCancelled(Object obj, EventArgs e)
        {
            //Update status bar with 
            this._theStatusLabel.Text = "Cancelled           ";
            this._theStatusProgressBar.Visible = false;
            _theCancelQueryLabel.Visible = false;
            _theStopQuery.Enabled = false;
            _theRowCount.Enabled = true;
            _theRefreshButton.Enabled = !_theStopQuery.Enabled;
            ShowStatus(_theConfig.ShowStatusStrip == UniversalWidgetConfig.ShowStatusStripEnum.Show);
        }

        protected virtual void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (this.IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        protected virtual void HandleNewDataArrived(Object obj, EventArgs e)
        {
            
            //Populate the chart
            this._theStatusLabel.Text = "Successful           ";
            this._theStatusProgressBar.Visible = false;
            _theCancelQueryLabel.Visible = false;
            _theStopQuery.Enabled = false;
            _theRowCount.Enabled = true;
            _theRefreshButton.Enabled = !_theStopQuery.Enabled;
            ShowStatus(_theConfig.ShowStatusStrip == UniversalWidgetConfig.ShowStatusStripEnum.Show);
        }

        protected virtual void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (this.IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        protected virtual void HandleFetchingData(Object obj, EventArgs e)
        {
            _theErrorTabPage.Hide();
            ShowStatus((_theConfig.ShowStatusStrip == UniversalWidgetConfig.ShowStatusStripEnum.Show) ||
                (_theConfig.ShowStatusStrip == UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation));

            //Set the maximum number of rows to return
            int maxRowCount = 0;
            try
            {
                maxRowCount = RowCount;
            }
            catch (Exception ex)
            {
                //do nothing
            }
            UniversalWidgetConfiguration.DataProviderConfig.MaxRowCount = maxRowCount;
            _theRowCount.Enabled = false;
            //TODO: show the status bar 
            this._theStatusLabel.Text = "Fetching Data...           ";
            this._theStatusProgressBar.Visible = true;
            _theCancelQueryLabel.Visible = false;
            _theStopQuery.Enabled = true;
            _theRefreshButton.Enabled = !_theStopQuery.Enabled;
        }

        //Can be overridden to change the behavior of the ShowStatus
        protected virtual void ShowStatus(bool showStatus)
        {
            if (showStatus)
            {
                statusStrip1.Visible = showStatus;
                _theStatusLabel.Visible = showStatus;
            }
            else
            {
                //if the refresh time has not been set, hide the status bar
                if (_theConfig.DataProviderConfig.RefreshRate <= 0)
                {
                    statusStrip1.Visible = showStatus;
                }
                else
                {
                    if (_theConfig.ShowStatusStrip == UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation)
                    {
                        statusStrip1.Visible = true;
                        //code to hide only the progress and not the entire status
                        _theStatusLabel.Visible = showStatus;
                    }
                    else
                    {
                        statusStrip1.Visible = showStatus;
                    }
                }
            }
        }

        protected virtual void InvokeHandleTimerStateChanged(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateTimerStatus(HandleTimerStateChanged), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
            //HandleTimerStateChanged(obj, (TimerEventArgs)e);
        }

        protected virtual void HandleTimerStateChanged(Object obj, TimerEventArgs e)
        {
            //This will make the lable and the pause/resume buttons visible based on the
            //refresh time setup
            _theRefreshLabel.Visible = (_theConfig.DataProviderConfig.RefreshRate > 0);
            _theRefreshTimerButton.Visible = (_theConfig.ShowRefreshTimerButton) && (_theConfig.DataProviderConfig.RefreshRate > 0);

            if (e.EventType == TimerEventArgs.TimerEventTypes.Paused)
            {
                _theRefreshLabel.Text = TimerEventArgs.EventTypes[(int)e.EventType];
                this._theRefreshTimerButton.Image = Properties.Resources.player_play;
                this._theRefreshTimerButton.ToolTipText = "Resume timer";
                this._theRefreshTimerButton.Tag = "pause_mode";
            }
            else if (e.EventType == TimerEventArgs.TimerEventTypes.Started)
            {
                _theRefreshLabel.Text = (e.RefreshRate - e.TickValue) + " secs";
                _theRefreshTimerButton.Image = Properties.Resources.player_pause;
                this._theRefreshTimerButton.ToolTipText = "Pause timer";
                this._theRefreshTimerButton.Tag = "play_mode";

                DataProvider.RefreshTimer.Start();
            }
            else if (e.EventType == TimerEventArgs.TimerEventTypes.Ticked)
            {
                _theRefreshLabel.Text = (e.RefreshRate - e.TickValue) + " secs";
                _theRefreshTimerButton.Image = Properties.Resources.player_pause;
                this._theRefreshTimerButton.ToolTipText = "Pause timer";
                this._theRefreshTimerButton.Tag = "play_mode";
            }
        }

        protected virtual void InvokeHandleInitDataproviderForFetch(Object obj, EventArgs e)
        {
            try
            {
                if (this.IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleInitDataproviderForFetch), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        protected virtual void HandleInitDataproviderForFetch(Object obj, EventArgs e)
        {
            _theErrorTabPage.Hide();
            this._theStatusLabel.Text = "Ready                 ";
            this._theStatusProgressBar.Visible = true;
            _theCancelQueryLabel.Visible = false;
            _theStopQuery.Enabled = true;
            _theRefreshButton.Enabled = !_theStopQuery.Enabled;
            _theConfig.ShowProviderStatus = false;
            showAppropriatePanels();
        }

        protected virtual void InvokeHandleOnBeforeFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (this.IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleOnBeforeFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        protected virtual void HandleOnBeforeFetchingData(Object obj, EventArgs e)
        {
            //Set the maximum number of rows to return
            int maxRowCount = 0;
            try
            {
                maxRowCount = RowCount;
            }
            catch (Exception ex)
            {
                //do nothing
            }
            if (UniversalWidgetConfiguration != null && UniversalWidgetConfiguration.DataProviderConfig != null)
            {
                UniversalWidgetConfiguration.DataProviderConfig.MaxRowCount = maxRowCount;
            }

        }
        
        protected virtual void SetStopQueryButton(bool enable)
        {
            _theStopQuery.Visible = _theStopQuery.Enabled = enable;
            _theRefreshButton.Visible = _theRefreshButton.Enabled = !enable;
        }

        public void ResetProviderStatus()
        {
            _theErrorTabPage.Hide();
            this._theStatusLabel.Text = "Ready                 ";
            this._theStatusProgressBar.Visible = false;
            _theCancelQueryLabel.Visible = false;
            _theStopQuery.Enabled = false;
            _theRefreshButton.Enabled = !_theStopQuery.Enabled;
            _theConfig.ShowProviderStatus = false;
            showAppropriatePanels();
        }

        public void ResetChartControl()
        {
            if (_theConfig != null && _theConfig.SupportCharts)
            {
                SplitterPanel chartPanel = GetChartPanel();
                chartPanel.Controls.Remove(_theChartControl);
                _theChartControl = new MSChartControl(this.UniversalWidgetConfiguration);
                _theChartControl.Dock = DockStyle.Fill;
                _theChartControl.DataProvider = this.DataProvider;
                chartPanel.Controls.Add(_theChartControl);
            }
        }

        private void TrafodionSplitContainer1_KeyDown(object sender, KeyEventArgs e)
        {

        }

        private void TrafodionSplitContainer1_MouseDoubleClick(object sender, MouseEventArgs e)
        {

        }


        //private void statusStrip1_MouseClick(object sender, MouseEventArgs e)
        //{
        //    clickCount = (clickCount > 3) ? 0 : ++clickCount;
        //    switch (clickCount)
        //    {
        //        case 0:
        //            {
        //                _theConfig.ShowTable = true;
        //                _theConfig.ShowChart = false;
        //                _theConfig.ShowProperties = false;
        //                _theConfig.ShowProviderStatus = false;
        //            }
        //            break;
        //        case 1:
        //            {
        //                _theConfig.ShowTable = true;
        //                _theConfig.ShowChart = true;
        //                _theConfig.ShowProperties = false;
        //                _theConfig.ShowProviderStatus = false;
        //            }
        //            break;
        //        case 2:
        //            {
        //                _theConfig.ShowTable = true;
        //                _theConfig.ShowChart = true;
        //                _theConfig.ShowProperties = true;
        //                _theConfig.ShowProviderStatus = false;
        //            }
        //            break;
        //        case 3:
        //            {
        //                _theConfig.ShowTable = true;
        //                _theConfig.ShowChart = true;
        //                _theConfig.ShowProperties = true;
        //                _theConfig.ShowProviderStatus = true;
        //            }
        //            break;

        //    }
        //    showAppropriatePanels();
        //}

        //private void _theShowGraph_Click(object sender, EventArgs e)
        //{
        //    _theConfig.ShowChart = !_theConfig.ShowChart;
        //    showAppropriatePanels();
        //}

        private void _theRefreshButton_Click(object sender, EventArgs e)
        {
            //TODO: provide some call to refresh in the dataprovider
            //DataProvider.Start(_theConfig.PassedParameters);
            UniversalWidgetConfiguration.DataProviderConfig.MaxRowCount = RowCount;
            DataProvider.RefreshData();
        }

        private void _theShowProperties_Click(object sender, EventArgs e)
        {
            _theConfig.ShowProperties = !_theConfig.ShowProperties;
            showAppropriatePanels();
        }

        private void _theShowDetails_Click(object sender, EventArgs e)
        {
            _theConfig.ShowProviderStatus = !_theConfig.ShowProviderStatus;
            showAppropriatePanels();
        }

        private void _theCancelQuery_Click(object sender, EventArgs e)
        {
            DataProvider.Stop();
        }

        private void _theStopQuery_Click(object sender, EventArgs e)
        {
            DataProvider.Stop();
        }

        private void Minute1MenuItem_Click(object sender, EventArgs e)
        {
            setTimerRefreshRate(sender as ToolStripDropDownItem);
        }

        private void Sec30MenuItem_Click(object sender, EventArgs e)
        {
            setTimerRefreshRate(sender as ToolStripDropDownItem);
        }

        private void Minute2MenuItem_Click(object sender, EventArgs e)
        {
            setTimerRefreshRate(sender as ToolStripDropDownItem);
        }

        private void Minute5MenuItem_Click(object sender, EventArgs e)
        {
            setTimerRefreshRate(sender as ToolStripDropDownItem);
        }

        private void Minute10MenuItem_Click(object sender, EventArgs e)
        {
            setTimerRefreshRate(sender as ToolStripDropDownItem);
        }

        private void Minute15MenuItem_Click(object sender, EventArgs e)
        {
            setTimerRefreshRate(sender as ToolStripDropDownItem);
        }

        private void Minute30MenuItem_Click(object sender, EventArgs e)
        {
            setTimerRefreshRate(sender as ToolStripDropDownItem);
        }

        private void CustomMenuItem_Click(object sender, EventArgs e)
        {
            setTimerRefreshRate(sender as ToolStripDropDownItem);
        }


        private void TimerStopMenuItem_Click(object sender, EventArgs e)
        {
            setTimerRefreshRate(sender as ToolStripDropDownItem);
        }

        private void setTimerRefreshRate(ToolStripDropDownItem aTimerMenu)
        {
            //Only the image that is clicked will have the check mark next to it.
            //The rest will be removed
            foreach (ToolStripItem toolStripItem in this._theTimerSetupButton.DropDownItems)
            {
                toolStripItem.Image = null;
            }

            aTimerMenu.Image = Properties.Resources.apply;
            if (aTimerMenu != null)
            {
                
                int refreshRate = Int32.Parse(aTimerMenu.Tag.ToString());
                DataProvider.RefreshRate = refreshRate;
            }
        }

        /// <summary>
        /// This method will be used to set the default refresh rate check mark for the 
        /// menu items
        /// </summary>
        /// <param name="aRefreshRate"></param>
        private void setDefaultRefreshRate(int aRefreshRate)
        {
            //set default refresh rates based on configuration
            if (_theConfig.DataProviderConfig.DefaultRefreshRates != null)
            {
                string[] defaultRefreshRates = _theConfig.DataProviderConfig.DefaultRefreshRates.Split(',');

                ArrayList currentList = null;
                int defaultRate = 0;
                for (int i = 0; i < defaultRefreshRates.Length; i++)
                {
                    try
                    {
                        defaultRate = Int32.Parse(defaultRefreshRates[i]);
                    }
                    catch
                    {
                    }
                    bool isContained = false;
                    if (currentList == null)
                    {
                        currentList = new ArrayList();
                        foreach (ToolStripItem toolStripItem in this._theTimerSetupButton.DropDownItems)
                        {
                            int refreshRate = 0;
                            try
                            {
                                if (toolStripItem.Tag != null)
                                {
                                    refreshRate = Int32.Parse(toolStripItem.Tag.ToString());
                                    currentList.Add(refreshRate);
                                }
                            }
                            catch (Exception ex)
                            {
                                //do nothing
                            }
                            if (defaultRate == refreshRate)
                            {
                                isContained = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (currentList.Contains(defaultRate))
                        {
                            isContained = true;
                        }
                    }
                    //add refresh rate to dropdown list
                    if (!isContained)
                    {
                        //Create a custom menu item for the unhandled time
                        ToolStripMenuItem customMenuItem = new ToolStripMenuItem("Custom");
                        customMenuItem.Name = "CustomMenuItem";
                        customMenuItem.Tag = defaultRefreshRates[i] + "";
                        customMenuItem.Text = GetMenuText(defaultRate);
                        customMenuItem.Click += new System.EventHandler(this.CustomMenuItem_Click);

                        //Add the menu at the appropriate place
                        int idx = 0;
                        bool isInserted = false;
                        foreach (ToolStripItem toolStripItem in this._theTimerSetupButton.DropDownItems)
                        {
                            int refreshRate = 0;
                            try
                            {
                                if (toolStripItem.Tag != null)
                                {
                                    refreshRate = Int32.Parse(toolStripItem.Tag.ToString());
                                }
                                if (refreshRate > defaultRate)
                                {
                                    this._theTimerSetupButton.DropDownItems.Insert(idx, customMenuItem);
                                    isInserted = true;
                                    break;
                                }
                            }
                            catch (Exception ex)
                            {
                                //do nothing
                            }
                            idx++;
                        }

                        //if we reached here, then we need to append it to the end
                        if (!isInserted)
                        {
                            this._theTimerSetupButton.DropDownItems.Add(customMenuItem);
                        }
                    }
                }
            }

            //Only the image that is clicked will have the check mark next to it.
            //The rest will be removed
            foreach (ToolStripItem toolStripItem in this._theTimerSetupButton.DropDownItems)
            {
                toolStripItem.Image = null;
            }

            bool set = false;
            if (aRefreshRate > 0)
            {
                foreach (ToolStripItem toolStripItem in this._theTimerSetupButton.DropDownItems)
                {
                    int refreshRate = 0;
                    try
                    {
                        if (toolStripItem.Tag != null)
                        {
                            refreshRate = Int32.Parse(toolStripItem.Tag.ToString());
                        }
                    }
                    catch (Exception ex)
                    {
                        //do nothing
                    }
                    if (refreshRate == aRefreshRate)
                    {
                        toolStripItem.Image = Properties.Resources.apply;
                        set = true;
                    }
                }
            }

            //It's entirely possible for someone to set any arbitrary value in the 
            //refresh time as part of the config. In a ituation like that, we have 
            //to dynamically add a menu item with the value specified in the config
            if (!set)
            {
                if (aRefreshRate > 0)
                {
                    //Create a custom menu item for the unhandled time
                    ToolStripMenuItem customMenuItem = new ToolStripMenuItem("Custom");
                    customMenuItem.Name = "CustomMenuItem";
                    customMenuItem.Tag = aRefreshRate + "";
                    customMenuItem.Text = GetMenuText(aRefreshRate);
                    customMenuItem.Click += new System.EventHandler(this.CustomMenuItem_Click);
                    customMenuItem.Image = Properties.Resources.apply;
                    
                    //Add the menu at the appropriate place
                    int idx = 0;
                    foreach (ToolStripItem toolStripItem in this._theTimerSetupButton.DropDownItems)
                    {
                        int refreshRate = 0;
                        try
                        {
                            if (toolStripItem.Tag != null)
                            {
                                refreshRate = Int32.Parse(toolStripItem.Tag.ToString());
                            }
                            if (refreshRate > aRefreshRate )
                            {
                                this._theTimerSetupButton.DropDownItems.Insert(idx, customMenuItem);
                                return;
                            }
                        }
                        catch (Exception ex)
                        {
                            //do nothing
                        }
                        idx++;
                    }

                    //if we reached here, then we need to append it to the end
                    this._theTimerSetupButton.DropDownItems.Add(customMenuItem);
                }
            }
        }

        /// <summary>
        /// Helper method to get the text to be displayed in a menu
        /// </summary>
        /// <param name="time"></param>
        /// <returns></returns>
        private static string GetMenuText(int time)
        {
            string text = "";
            int min = (time / 60);
            int sec = (time % 60);
            
            if ((min > 0) && (sec > 0))
            {
                text = string.Format("{0} Minutes {1} Secs", min, sec);
            }
            else if (min > 0)
            {
                text = string.Format("{0} Minutes", min);
            }
            else if (sec > 0)
            {
                text = string.Format("{0} Secs", sec);
            }
            return text;
        }

        private void _theRefreshTimerButton_Click(object sender, EventArgs e)
        {
            if (this._theRefreshTimerButton.Tag.Equals("pause_mode"))
            {
                DataProvider.StartTimer();
            }
            else if (this._theRefreshTimerButton.Tag.Equals("play_mode"))
            {
                DataProvider.StopTimer();
            }

        }

        private void _theChartDesignerButton_Click(object sender, EventArgs e)
        {
            if (DataProvider != null)
            {
                DataTable dataTable = DataProvider.GetDataTable();
                if (dataTable != null)
                {
                    List<string> columns = new List<string>();
                    foreach (DataColumn dc in dataTable.Columns)
                    {
                        columns.Add(dc.ColumnName);
                    }

                    ColumnNameConverter.StandardValues = columns.ToArray();
                    ChartDesignerDialog dialog = new ChartDesignerDialog(this);
                    dialog.Text = "Chart Designer";
                    dialog.DataTable = dataTable;
                    dialog.ShowDialog();
                }
                else
                {
                    MessageBox.Show("The statement has not been run.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
        }

        //This will show the bottom panel. If there is error, the error tab will
        //be displayed by default, otherwise the out tab shall be displayed
        private void _theStatusLabel_Click(object sender, EventArgs e)
        {
            _theConfig.ShowProviderStatus = ! _theConfig.ShowProviderStatus;
            showAppropriatePanels();
        }

        //Do the dispose
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Remove the event handlers
                this.RemoveEventHandlers();

                if (_theConfig != null && _theConfig.SupportCharts)
                {
                    this._theDisplayPanel.SplitterMoved -= DisplayPanel_SplitterMoved;
                }
                //Persist the configuration
                PersistConfiguration();

                //Remove the Persistence handler
                Persistence.PersistenceHandlers -= _thePersistenceHandler;
                if (_theChartControl != null)
                {
                    _theChartControl.Dispose();
                }
                if (_theExceptionControl != null)
                {
                    _theExceptionControl.Dispose();
                }

                if (_theConsoleControl != null)
                {
                    _theConsoleControl.Dispose();
                }
                if (_theDataDisplayControl != null)
                {
                    UserControl displayControl = _theDataDisplayControl as UserControl;
                    if (displayControl != null)
                    {
                        displayControl.Dispose();
                    }
                }
            }
        }

        private void _theRowCount_KeyPress(object sender, KeyPressEventArgs e)
        {
            e.Handled = !(Char.IsDigit(e.KeyChar) || Char.IsControl(e.KeyChar));
        }

        private void _theRowCount_Leave(object sender, EventArgs e)
        {
            int rowCount = -1;
            if (_theRowCount.Visible)
            {
                try
                {
                    rowCount = int.Parse(_theRowCount.Text.Trim());
                }
                catch (Exception ex)
                {
                    _theRowCount.Text = "0";
                    RowCount = 0;
                }
            }
        }
    }


    //This class will be used to inject the context into the UniversalWidget
    public class UniversalWidgetContext
    {
        private ConnectionDefinition _theConnectionDefinition;
        private Connection _theConnection;
        private Dictionary<string, string> _theSystemProperties = new Dictionary<string, string>();
        private Dictionary<string, string> _theApplicationProperties = new Dictionary<string, string>();
        private bool SystemPropertiesPopulated = false;

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set 
            {
                if (_theConnection != null)
                {
                    _theConnectionDefinition = value;
                }
            }
        }

        public Connection Connection
        {
            get { return _theConnection; }
            set 
            {
                _theConnection = value;
                if (_theConnection != null)
                {
                    _theConnectionDefinition = _theConnection.TheConnectionDefinition;
                }
            }
        }

        private void PopulateSystemProperties()
        {
            if (!SystemPropertiesPopulated)
            {
                if (_theConnection == null)
                {
                    _theConnection = new Connection(_theConnectionDefinition);
                }
                _theSystemProperties.Add("SystemCatalogName", Connection.SystemCatalogName);
                _theSystemProperties.Add("SystemCatalogName", Connection.SystemCatalogName);
            }
            SystemPropertiesPopulated = true;
        }

        public Dictionary<string, string> SystemProperties
        {
            get 
            {
                PopulateSystemProperties();
                return _theSystemProperties; 
            }
        }

        public Dictionary<string, string> ApplicationProperties
        {
            get { return _theApplicationProperties; }
        }

        public void AddApplicationProperty(string propertyName, string propertyValue)
        {
            if ((propertyName != null) && (propertyValue != null))
            {
                if (_theApplicationProperties.ContainsKey(propertyName))
                {
                    _theApplicationProperties.Remove(propertyName);
                }
                _theApplicationProperties.Add(propertyName, propertyValue);
            }
        }

        public string GetApplicationProperty(string propertyName)
        {
            string ret = null;
            if (_theApplicationProperties.ContainsKey(propertyName))
            {
                ret = _theApplicationProperties[propertyName];
            }
            return ret;
        }
    }
}
