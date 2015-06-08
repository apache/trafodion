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
using System.IO;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.DatabaseArea.Controls;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Queries;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class MetricMinerReportTabContent : UserControl, IConnectionDefinitionSelector
    {
        UniversalWidgetConfig _theConfig = null;
        protected TabularDataDisplayControl _theTabularDataDisplayControl;
        protected TextBoxDataDisplayControl _theTextBoxDataDisplayControl;
        protected QueryPlanDataDisplayControl _theQueryPlanDataDisplayControl;
        private ConnectionDefinition _theSelectedConnectionDefinition = null;
        protected GenericUniversalWidget _theWidget = null;
        public delegate void DoClose(MetricMinerReportTabContent aMetricMinerReportTabContent);
        private DoClose _theDoCloseImpl;
        public delegate void AfterSaveAs(MetricMinerReportTabContent aMetricMinerReportTabContent, UniversalWidgetConfig aUniversalWidgetConfig);
        public event AfterSaveAs OnAfterSaveAs;
        public delegate void OnReportSuccess(MetricMinerReportTabContent aMetricMinerReportTabContent, TabPage aTabPage);
        private OnReportSuccess _theReportSuccessImpl;
        private TrafodionCatalog _theSelectedTrafodionCatalog = null;
        private TrafodionSchema _theSelectedTrafodionSchema = null;
        protected ToolStripButton runButton;
        protected ToolStripButton explainButton = null;
        TrafodionIGridToolStripMenuItem _drillDownMenuItem = null;

        public delegate void ExpandRequested(MetricMinerReportTabContent tabContent);
        public event ExpandRequested OnExpandRequested;

        public delegate void UpdateReportTab(object sender, EventArgs e);
        public event UpdateReportTab OnUpdateReportTab;
        public delegate void UpdateStatus(Object obj, DataProviderEventArgs e);
        private bool editMode = false;
        private bool runFlag = false;

        private void fireUpdateReportTab(EventArgs e)
        {
            if (OnUpdateReportTab != null)
            {

                OnUpdateReportTab(this, e);
            }
        }

        public MetricMinerReportTabContent()
        {
            InitializeComponent();
            this.Disposed += new EventHandler(MetricMinerReportTabContent_Disposed);
            _theShowHideQuery.Tag = this._theQuerySplitContainer.Panel1Collapsed ? "hide" : "show";
            setShowHideValues();

            //By default we don't want to discplay the parameter and the row details tab
            _theQueryPropertyTabPanel.TabPages.Remove(_theStatusTab);
            _theQueryPropertyTabPanel.TabPages.Remove(_theParameterTab);
            _theQueryPropertyTabPanel.TabPages.Remove(_theRowDetailsTab);

            //TheQueryTextBox.TextChanged += new EventHandler(TheQueryTextBoxTextChanged);

            if (_theSystemsCombo.Items.Count > 0)
            {
                if (TrafodionContext.Instance.CurrentConnectionDefinition != null)
                {
                    if (TrafodionContext.Instance.CurrentConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                    {
                        _theSystemsCombo.SelectedConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;
                        SelectConnectionDefinition(_theSystemsCombo.SelectedConnectionDefinition);
                    }
                    else
                    {
                        _theSystemsCombo.SelectedIndex = -1;
                    }
                }
                else
                {
                    _theSystemsCombo.SelectedIndex = -1;
                }
            }


            //_theConnectionDefinitionChangedHandler = new ConnectionDefinition.ChangedHandler(ConnectionDefinitionChanged);
            //ConnectionDefinition.Changed += _theConnectionDefinitionChangedHandler;

            //SetDefaultMaxRows();

            UpdateControls();

        }

        void MetricMinerReportTabContent_Disposed(object sender, EventArgs e)
        {
            RemoveHandlers();
        }

        public MetricMinerReportTabContent(UniversalWidgetConfig aUniversalWidgetConfig)
            : this()
        {
            Config = aUniversalWidgetConfig;
        }

        //    {
        //widgetLinker.AdditionalParameters.Add("CONNECTION", this._theSystemsCombo.SelectedConnectionDefinition);
        //widgetLinker.AdditionalParameters.Add("CATALOG", this._theSelectedTrafodionCatalog);
        //widgetLinker.AdditionalParameters.Add("SCHEMA", this._theSelectedTrafodionSchema);

        public void SelectCatalogAndSchema(WidgetLinkerObject aWidgetLinker)
        {
            if ((aWidgetLinker != null) && (aWidgetLinker.AdditionalParameters != null) && (aWidgetLinker.AdditionalParameters["CONNECTION"] != null) && (aWidgetLinker.AdditionalParameters["CATALOG"] != null) && (aWidgetLinker.AdditionalParameters["SCHEMA"] != null))
            {

                ConnectionDefinition connectionDefinition = aWidgetLinker.AdditionalParameters["CONNECTION"] as ConnectionDefinition;
                _theSystemsCombo.SelectedCatalogName = (string)aWidgetLinker.AdditionalParameters["CATALOG"];
                _theSystemsCombo.SelectedSchemaName = (string)aWidgetLinker.AdditionalParameters["SCHEMA"];
                SelectConnectionDefinition(connectionDefinition);
            }
        }

        public TrafodionCatalog SelectedTrafodionCatalog
        {
            get { return _theSelectedTrafodionCatalog; }
            set { _theSelectedTrafodionCatalog = value; }
        }


        public TrafodionSchema SelectedTrafodionSchema
        {
            get { return _theSelectedTrafodionSchema; }
            set { _theSelectedTrafodionSchema = value; }
        }

        public GenericUniversalWidget Widget
        {
            get { return _theWidget; }
            set { _theWidget = value; }
        }

        /// <summary>
        /// Will handle the activities needed to be done when this control is closed
        /// </summary>
        public DoClose DoCloseImpl
        {
            get { return _theDoCloseImpl; }
            set { _theDoCloseImpl = value; }
        }

        public OnReportSuccess ReportSuccessImpl
        {
            get { return _theReportSuccessImpl; }
            set { _theReportSuccessImpl = value; }
        }

        public QueryPlanDataDisplayControl QueryPlanDataDisplayControl
        {
            get
            {
                return _theQueryPlanDataDisplayControl;
            }
        }

        public UniversalWidgetConfig Config
        {
            get { return _theConfig; }
            set
            {
                _theConfig = value;

                //We make sure that the row count is always displayed
                _theConfig.ShowRowCount = true;
                //We want to show the chart button at all times
                //Hide the chart button in 2.5
                //Config.ShowChartToolBarButton = false;
                //Config.ShowChart = true;
                Config.ShowChartToolBarButton = true;
                Config.SupportCharts = true;

                //Create the widget
                //_theWidget = new GenericUniversalWidget(_theConfig);
                _theWidget = new GenericUniversalWidget();
                _theWidget.DataProvider = new DatabaseDataProvider(_theConfig.DataProviderConfig);
                _theWidget.UniversalWidgetConfiguration = _theConfig;

                //set the properties of the default tabular data display control
                _theTabularDataDisplayControl = (TabularDataDisplayControl)_theWidget.DataDisplayControl;
                _theTabularDataDisplayControl.DataGrid.DoubleClickHandler = this.ShowRowDetailsImpl;
                _theTabularDataDisplayControl.DataGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(DataGrid_CellClick);
                _theTabularDataDisplayControl.DataGrid.RowMode = true;
                _theTabularDataDisplayControl.DrillDownManager.PopulateCalledWidgetImpl = this.PopulateCalledWidgetImpl;

                //We also need a textbox display control that will display the non query displays
                _theTextBoxDataDisplayControl = new TextBoxDataDisplayControl();
                _theTextBoxDataDisplayControl.UniversalWidgetConfiguration = _theConfig;
                _theTextBoxDataDisplayControl.DataProvider = _theWidget.DataProvider;

                _theQueryPlanDataDisplayControl = new QueryPlanDataDisplayControl();
                _theQueryPlanDataDisplayControl.UniversalWidgetConfiguration = _theConfig;
                _theQueryPlanDataDisplayControl.DataProvider = _theWidget.DataProvider;


                RepositoryReportsWidgetDataHandler displayDataHandler = new RepositoryReportsWidgetDataHandler();
                displayDataHandler.DoPostPopulateImpl = this.DoPostPopulateImpl;
                _theWidget.DataDisplayControl.DataDisplayHandler = displayDataHandler;

                if (_theWidget.ChartControl != null)
                {
                    _theWidget.ChartControl.MouseClickHandler = ChartControl_MouseClick;
                }

                _theWidget.OnChartConfigurationChanged += new GenericUniversalWidget.ChartConfigurationChanged(_theWidget_OnChartConfigurationChanged);

                //Add it to the UI
                _theUWPanel.Controls.Clear();
                _theWidget.Dock = DockStyle.Fill;
                _theUWPanel.Controls.Add(_theWidget);

                //Set the query text
                DatabaseDataProviderConfig dbDataProviderConfig = _theConfig.DataProviderConfig as DatabaseDataProviderConfig;
                if (dbDataProviderConfig != null)
                {
                    //Appending the name and version to the query so that it gets saved in the repository
                    //TODO: Check if we need to diplay it or if it should silently be added prior to executing
                    //      the query
                    StringBuilder sb = new StringBuilder();
                    //sb.AppendLine(String.Format("-- {0}", _theConfig.Name));
                    //if ((_theConfig.WidgetVersion != null) && (_theConfig.WidgetVersion.Trim().Length > 0))
                    //{
                    //    sb.AppendLine(String.Format("-- Version {0}", _theConfig.WidgetVersion));
                    //}
                    //sb.AppendLine();
                    if (!string.IsNullOrEmpty(dbDataProviderConfig.SQLText))
                    {
                        sb.AppendLine(dbDataProviderConfig.SQLText);
                    }
                    _theQueryInputControl.QueryText = sb.ToString();
                }

                //Populate the description panel to display the description for this configuration
                _theDescriptionDisplayUserControl.SetDescription(_theConfig);

                //We are getting the tool strip from the widget and using it in our UI
                _theHeaderPanel.Controls.Add(_theWidget.GetDetachedToolStrip());

                //Add cuntom buttons to the tool strip
                AddToolStripButtons();

                initReportStatus();

                //Add the data arrival handler
                AddHandlers();

                //Add the menu items
                AddMenuItems();

            }
        }

        void _theWidget_OnChartConfigurationChanged(object sender, EventArgs e)
        {
            // Go ahead to serialize the chart configuraton.
            if (this is AdhocReportTabContent)
            {
                if (!string.IsNullOrEmpty(_theConfig.ReportPath) && !string.IsNullOrEmpty(_theConfig.ReportFileName))
                {
                    saveAsButton_Click(sender, e);
                }
            }
            else
            {
                saveReport(false, true, true);
                fireUpdateReportTab(null);
            }
        }

        private void ChartControl_MouseClick(object sender, MouseEventArgs e)
        {
            //if (e.Button == System.Windows.Forms.MouseButtons.Right)
            //{
            //    //ShowChartDesigner();
            //    DataTable dataTable = _theWidget.DataProvider.GetDataTable();
            //    if (dataTable != null)
            //    {
            //        ExpertChartDesignerDialog dialog = new ExpertChartDesignerDialog(_theWidget);
            //        dialog.Text = "Expert Chart Designer";
            //        dialog.DataTable = dataTable;
            //        dialog.ShowDialog();
            //    }
            //}
        }

        private void AddHandlers()
        {
            if (this._theWidget.DataProvider != null)
            {
                //Associate the event handlers
                this._theWidget.DataProvider.OnNewDataArrived += InvokeDataProvider_OnNewDataArrived;
                this._theWidget.DataProvider.OnFetchingData += InvokeDataProvider_OnFetchingData;
                this._theWidget.DataProvider.OnErrorEncountered += InvokeDataProvider_OnErrorEncountered;
                this._theWidget.DataProvider.OnFetchCancelled += InvokeDataProvider_OnErrorEncountered;
                this._theWidget.DataProvider.OnInitDataproviderForFetch += InvokeDataProvider_OnInitDataproviderForFetch;
            }
        }

        private void RemoveHandlers()
        {
            if (this._theWidget.DataProvider != null)
            {
                //Remove the event handlers
                this._theWidget.DataProvider.OnNewDataArrived -= InvokeDataProvider_OnNewDataArrived;
                this._theWidget.DataProvider.OnFetchingData -= InvokeDataProvider_OnFetchingData;
                this._theWidget.DataProvider.OnErrorEncountered -= InvokeDataProvider_OnErrorEncountered;
                this._theWidget.DataProvider.OnFetchCancelled -= InvokeDataProvider_OnErrorEncountered;
                this._theWidget.DataProvider.OnInitDataproviderForFetch -= InvokeDataProvider_OnInitDataproviderForFetch;
            }
        }

        //init button status.
        private void initReportStatus()
        {
            editMode = false;
            runFlag = false;
            //undoStack.Clear();
            //redoStack.Clear();
            setButtonStatus(false);
            _theQueryInputControl.ReadOnly = true;

            if (_theQueryInputControl.QueryText.Trim().Length == 0)
            {
                this.explainButton.Enabled = false;
                this.runButton.Enabled = false;
            }

        }

        //We want to update the row details when a row is clicked
        void DataGrid_CellClick(object sender, TenTec.Windows.iGridLib.iGCellClickEventArgs e)
        {
            if (e.RowIndex < 0)
            {
                return; //Header clicked.
            }
            ShowRowDetailsImpl(e.RowIndex);
        }

        private void InvokeDataProvider_OnInitDataproviderForFetch(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(DataProvider_OnInitDataproviderForFetch), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.MetricMinner,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// When a new fetch is starting we want to hide the parameters and the row details tabs
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void DataProvider_OnInitDataproviderForFetch(object sender, DataProviderEventArgs e)
        {
            _theQueryPropertyTabPanel.TabPages.Remove(_theStatusTab);
            _theQueryPropertyTabPanel.TabPages.Remove(_theParameterTab);
            _theQueryPropertyTabPanel.TabPages.Remove(_theRowDetailsTab);
            DatabaseDataProvider dbDataProvider = this._theWidget.DataProvider as DatabaseDataProvider;
            if (dbDataProvider != null && dbDataProvider.ExplainMode)
            {
                _theWidget.HideChartPanel();
            }
        }


        public void ShowQuery(bool showFlag)
        {
            this._theQuerySplitContainer.Panel1Collapsed = !showFlag;
            _theShowHideQuery.Tag = (showFlag) ? "show" : "hide";
            setShowHideValues();
        }

        public void RestoreCatalogAndSchema()
        {
            if (_theConfig.CmbCatalogName != null && _theConfig.CmbCatalogName.Length > 0)
            {
                _theCatalogsComboBox.SelectExternalName(_theConfig.CmbCatalogName);
            }

            if (_theConfig.CmbSchemaName != null && _theConfig.CmbSchemaName.Length > 0)
            {
                _theSchemasComboBox.SelectExternalName(_theConfig.CmbSchemaName);
            }
        }

        public void StartDataProvider()
        {
            //Set the catalog and schema specified by the user as params
            string defaultCatalogName = (_theSelectedTrafodionCatalog == null) ? "" : _theSelectedTrafodionCatalog.ExternalName;
            string defaultSchemaName = (_theSelectedTrafodionSchema == null) ? "" : _theSelectedTrafodionSchema.ExternalName;

            if (_theConfig.PassedParameters != null)
            {
                _theConfig.PassedParameters.Clear();
            }
            else
            {
                _theConfig.PassedParameters = new System.Collections.Hashtable();
            }
            _theConfig.PassedParameters.Add(ReportParameterProcessorBase.CATALOG_NAME, defaultCatalogName);
            _theConfig.PassedParameters.Add(ReportParameterProcessorBase.SCHEMA_NAME, defaultSchemaName);
            _theConfig.PassedParameters.Add(ReportParameterProcessorBase.SESSION_NAME, "METRICMINER");
            _theConfig.PassedParameters.Add(ReportParameterProcessorBase.REPORT_NAME, _theConfig.Name);
            _theConfig.PassedParameters.Add(ReportParameterProcessorBase.ROW_COUNT, _theWidget.RowCount);
            _theConfig.PassedParameters.Add(ReportParameterProcessorBase.SYSTEM_CATALOG_NAME, _theSelectedConnectionDefinition.SystemCatalogName);


            DatabaseDataProvider databaseDataProvider = Widget.DataProvider as DatabaseDataProvider;
            if (databaseDataProvider != null)
            {
                databaseDataProvider.TheConnectionProvider.SetSessionName("METRICMINER");
            }
            Widget.StartDataProvider();
        }

        public ConnectionDefinition TheSelectedConnectionDefinition
        {
            get { return _theSystemsCombo.SelectedConnectionDefinition; }
            set
            {
                this._theSystemsCombo.SelectedConnectionDefinition = value;
                SelectConnectionDefinition(value);
            }
        }
        /// <summary>
        /// This method is used for implementing the IConnectionDefinitionSelector interface
        /// The current connection definition 
        /// </summary>
        public ConnectionDefinition CurrentConnectionDefinition
        {
            get
            {
                return TheSelectedConnectionDefinition;
            }
        }

        public void SelectConnectionDefinition(ConnectionDefinition aConnectionDefinition)
        {
            _theSelectedConnectionDefinition = aConnectionDefinition;
            showCatalogsCombo(true);

            if (_theSelectedConnectionDefinition != null)
            {
                if (_theSelectedConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                {
                    _theSystemsCombo.SelectedConnectionDefinition = _theSelectedConnectionDefinition;
                    _theCatalogsComboBox.TheTrafodionSystem = TrafodionSystem.FindTrafodionSystem(_theSelectedConnectionDefinition);

                    try
                    {
                        _theCatalogsComboBox.SelectedTrafodionCatalog = _theCatalogsComboBox.TheTrafodionSystem.FindCatalog(_theSystemsCombo.SelectedCatalogName);
                        _theSchemasComboBox.TheTrafodionCatalog = _theCatalogsComboBox.SelectedTrafodionCatalog;
                    }
                    catch (Exception ex)
                    {
                        if (_theCatalogsComboBox.Items.Count > 0)
                        {
                            _theCatalogsComboBox.SelectedIndex = 0;
                        }
                        else
                        {
                            _theCatalogsComboBox.SelectedIndex = -1;
                        }
                    }

                    if (_theSystemsCombo.SelectedSchemaName != null)
                    {
                        if (!String.IsNullOrEmpty(_theSystemsCombo.SelectedSchemaName))
                        {
                            try
                            {
                                _theSchemasComboBox.SelectedTrafodionSchema = _theCatalogsComboBox.SelectedTrafodionCatalog.FindSchema(_theSystemsCombo.SelectedSchemaName);
                            }
                            catch (Exception)
                            {
                                if (_theSchemasComboBox.Items.Count > 0)
                                {
                                    _theSchemasComboBox.SelectedIndex = 0;
                                }
                                else
                                {
                                    _theSchemasComboBox.SelectedIndex = -1;
                                }
                            }
                        }
                    }

                }
            }
            UpdateControls();
        }

        public void ExpandArea(bool expand)
        {
            _theSplitPanel.Panel2Collapsed = expand;
        }
        private void showCatalogsCombo(bool show)
        {
            _theSelectorPanel.Controls.Remove(_theCatalogsPanel);
            _theSelectorPanel.Controls.Remove(_theSchemasPanel);
            if (show)
            {
                //Set the location of the catalogs panel
                _theCatalogsPanel.Location = new System.Drawing.Point(_theSystemsPanel.Location.X + _theSystemsPanel.Size.Width + 3, 0);
                //Set the location of the schemas panel
                _theSchemasPanel.Location = new System.Drawing.Point(_theCatalogsPanel.Location.X + _theCatalogsPanel.Size.Width + 3, 0);
                //Add the catalog control
                _theSelectorPanel.Controls.Add(_theCatalogsPanel);
            }
            else
            {
                //Set the location of the schemas panel
                _theSchemasPanel.Location = new System.Drawing.Point(_theSystemsPanel.Location.X + _theSystemsPanel.Size.Width + 3, 0);
            }
            //Add the schema control
            _theSelectorPanel.Controls.Add(_theSchemasPanel);
        }
        /// <summary>
        /// Add custom tool strip buttons 
        /// </summary>
        /// 
        ToolStripButton editButton = new ToolStripButton();
        ToolStripButton undoButton = new ToolStripButton();
        ToolStripButton redoButton = new ToolStripButton();
        ToolStripButton saveButton = new ToolStripButton();
        ToolStripButton saveAsButton = new ToolStripButton();
        //Stack<string> undoStack = new Stack<string>();
        //Stack<string> redoStack = new Stack<string>();
        ToolStripButton sqlDesigner = new ToolStripButton();
        protected virtual void AddToolStripButtons()
        {

            runButton = new ToolStripButton();
            runButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            runButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.RunIcon;
            runButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            runButton.Name = "runButton";
            runButton.Size = new System.Drawing.Size(23, 22);
            runButton.Text = "Execute";
            runButton.ToolTipText = "Execute the SQL statement";
            runButton.Click += new EventHandler(runButton_Click);
            this._theWidget.AddToolStripItem(runButton);

            //this._theWidget.AddToolStripItem(new ToolStripSeparator());

            explainButton = new ToolStripButton();
            explainButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            explainButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.SqlPlanIcon;
            explainButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            explainButton.Name = "explainButton";
            explainButton.Size = new System.Drawing.Size(23, 22);
            explainButton.Text = "Explain";
            explainButton.Click += new EventHandler(explainButton_Click);
            this._theWidget.AddToolStripItem(explainButton);

            this._theWidget.AddToolStripItem(new ToolStripSeparator());

            editButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            editButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.EditIcon;
            editButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            editButton.Name = "editButton";
            editButton.Size = new System.Drawing.Size(23, 22);
            editButton.Text = "Edit";
            editButton.ToolTipText = "Edits the report configuration";
            editButton.Click += new EventHandler(editButton_Click);
            this._theWidget.AddToolStripItem(editButton);


            undoButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            undoButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.UndoIcon;
            undoButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            undoButton.Name = "undoButton";
            undoButton.Size = new System.Drawing.Size(23, 22);
            undoButton.Text = "Undo changes to the report sql text";
            undoButton.Click += new EventHandler(undoButton_Click);
            this._theWidget.AddToolStripItem(undoButton);

            redoButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            redoButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.RedoIcon;
            redoButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            redoButton.Name = "redoButton";
            redoButton.Size = new System.Drawing.Size(23, 22);
            redoButton.Text = "Redo changes to the report sql text";
            redoButton.Click += new EventHandler(redoButton_Click);
            this._theWidget.AddToolStripItem(redoButton);

            saveButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            saveButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.SaveIcon;
            saveButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            saveButton.Name = "saveButton";
            saveButton.Size = new System.Drawing.Size(23, 22);
            saveButton.Text = "Saves the report configuration";
            saveButton.Click += new EventHandler(saveButton_Click);
            this._theWidget.AddToolStripItem(saveButton);

            saveAsButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            saveAsButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.SaveAsIcon;
            saveAsButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            saveAsButton.Name = "saveAsButton";
            saveAsButton.Size = new System.Drawing.Size(23, 22);
            saveAsButton.Text = "Saves the report configuration as a new report";
            saveAsButton.Click += new EventHandler(saveAsButton_Click);
            this._theWidget.AddToolStripItem(saveAsButton);

            ToolStripButton cancelButton = new ToolStripButton();
            cancelButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            cancelButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.CloseIcon;
            cancelButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            cancelButton.Name = "cancelButton";
            cancelButton.Size = new System.Drawing.Size(23, 22);
            cancelButton.Text = "Closes the report tab";
            cancelButton.Alignment = ToolStripItemAlignment.Right;
            cancelButton.Click += new EventHandler(cancelButton_Click);
            this._theWidget.AddToolStripItem(cancelButton);

            this._theWidget.AddToolStripItem(new ToolStripSeparator());


            ToolStripButton FullScreenButton = new ToolStripButton();
            FullScreenButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            FullScreenButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.FullScreenIcon;
            FullScreenButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            FullScreenButton.Name = "FullScreenButton";
            FullScreenButton.Size = new System.Drawing.Size(23, 22);
            FullScreenButton.Text = "Expand the tab to occupy the full screen";
            FullScreenButton.Click += new EventHandler(FullScreenButton_Click);
            this._theWidget.AddToolStripItem(FullScreenButton);



            //ToolStripDropDownButton exportButtons = new ToolStripDropDownButton();
            //exportButtons.DisplayStyle = ToolStripItemDisplayStyle.Image;
            //exportButtons.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.Export;
            //exportButtons.DropDownItems.Add(getToolStripMenuItemForExport("Clipboard", 1));
            //exportButtons.DropDownItems.Add(getToolStripMenuItemForExport("Browser" , 2 ));
            //exportButtons.DropDownItems.Add(getToolStripMenuItemForExport("Spreadsheet", 3));
            //exportButtons.DropDownItems.Add(getToolStripMenuItemForExport("File", 4));
            //exportButtons.ImageTransparentColor = System.Drawing.Color.Transparent;
            //exportButtons.Name = "exportButtons";
            //exportButtons.Size = new System.Drawing.Size(23, 22);
            //exportButtons.Text = "Export data to the selected format";
            //this._theWidget.AddToolStripItem(exportButtons);


            sqlDesigner.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            //sqlDesigner.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.SqlIcon;
            //sqlDesigner.ImageTransparentColor = System.Drawing.Color.Transparent;
            sqlDesigner.Name = "sqlDesigner";
            sqlDesigner.Size = new System.Drawing.Size(23, 22);
            sqlDesigner.Text = "SQL Designer";
            sqlDesigner.ToolTipText = "Displays the SQL Designer";
            sqlDesigner.Click += new EventHandler(sqlDesigner_Click);
            this._theWidget.AddToolStripItem(sqlDesigner);

            this._theWidget.AddToolStripItem(new ToolStripSeparator());

            //chartDesigner.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            //chartDesigner.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.DesignerIcon;
            //chartDesigner.ImageTransparentColor = System.Drawing.Color.Transparent;
            //chartDesigner.Name = "chartDesigner";
            //chartDesigner.Size = new System.Drawing.Size(23, 22);
            //chartDesigner.Text = "Chart Designer";
            //chartDesigner.ToolTipText = "Displays the Chart Designer";
            //chartDesigner.Click += new EventHandler(chartDesigner_Click);
            //this._theWidget.AddToolStripItem(chartDesigner);

            //ToolStripButton historyButton = new ToolStripButton();
            //historyButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            //historyButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.HistoryIcon;
            //historyButton.ImageTransparentColor = System.Drawing.Color.Transparent;
            //historyButton.Name = "historyButton";
            //historyButton.Size = new System.Drawing.Size(23, 22);
            //historyButton.Text = "Displays the history of all queries executed";
            //historyButton.Click += new EventHandler(historyButton_Click);
            //this._theWidget.AddToolStripItem(historyButton);
        }

        //redo button event
        void redoButton_Click(object sender, EventArgs e)
        {
            //string sqlText = redoStack.Pop();
            //if (sqlText == _theQueryInputControl.QueryText)
            //{
            //    undoStack.Push(sqlText);
            //    sqlText = redoStack.Pop();
            //    undoStack.Push(sqlText);
            //}
            //else
            //{
            //    undoStack.Push(sqlText);
            //}
            //_theQueryInputControl.QueryText = sqlText;
            _theQueryInputControl.RedoTheQueryText();
            updateUndoRedoButtonStatus();
        }

        //undo button event
        void undoButton_Click(object sender, EventArgs e)
        {
            //string sqlText = undoStack.Pop();
            //if (sqlText == _theQueryInputControl.QueryText)
            //{
            //    redoStack.Push(sqlText);
            //    sqlText = undoStack.Pop();
            //    redoStack.Push(sqlText);
            //}
            //else
            //{
            //    redoStack.Push(sqlText);
            //}
            //_theQueryInputControl.QueryText = sqlText;
            _theQueryInputControl.UndoTheQueryText();
            updateUndoRedoButtonStatus();
        }


        public QueryInputControl getQueryInputControl
        {
            get
            {
                return _theQueryInputControl;
            }
        }

        public bool EditMode
        {
            get
            {
                return editMode;
            }
        }

        public bool RunFlag
        {
            get
            {
                return runFlag;
            }
        }

        void editButton_Click(object sender, EventArgs e)
        {
            editMode = true;
            setButtonStatus(true);
            _theQueryInputControl.ReadOnly = false;
        }

        void setButtonStatus(bool aEnabled)
        {
            editButton.Enabled = !aEnabled;
            saveButton.Enabled = aEnabled;
            saveAsButton.Enabled = aEnabled;
            undoButton.Enabled = aEnabled;
            redoButton.Enabled = aEnabled;
            updateUndoRedoButtonStatus();
            sqlDesigner.Enabled = aEnabled;
        }

        //update redo & undo button status based on the data
        void updateUndoRedoButtonStatus()
        {
            //if (undoStack.Count > 0)
            //{
            //    undoButton.Enabled = true;
            //}
            //else
            //{
            //    undoButton.Enabled = false;
            //}
            //if (redoStack.Count > 0)
            //{
            //    redoButton.Enabled = true;
            //}
            //else
            //{
            //    redoButton.Enabled = false;
            //}

            if (_theQueryInputControl.CanTheQueryTextUndo())
                undoButton.Enabled = true;
            else undoButton.Enabled = false;
            if (_theQueryInputControl.CanTheQueryTextRedo())
                redoButton.Enabled = true;
            else redoButton.Enabled = false;
        }

        protected ToolStripMenuItem getToolStripMenuItemForExport(string text, int type)
        {
            ToolStripMenuItem exportMenu = new ToolStripMenuItem(text);
            exportMenu.Tag = type;
            exportMenu.Name = text;
            exportMenu.Click += new EventHandler(exportMenu_Click);
            return exportMenu;
        }

        void exportMenu_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem exportMenu = sender as ToolStripMenuItem;
            TabularDataDisplayControl dataDisplayControl = this._theWidget.DataDisplayControl as TabularDataDisplayControl;
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

        void exportButtons_Click(object sender, EventArgs e)
        {
            Object btn = sender;
        }

        protected virtual void sqlDesigner_Click(object sender, EventArgs e)
        {
            QueryDesignerDialog dialog = new QueryDesignerDialog(CurrentConnectionDefinition);
            dialog.ShowDialog();
            if (dialog.SelectedOption == DialogResult.OK)
            {
                _theQueryInputControl.QueryText = dialog.QueryText;
            }
            //UniversalWidgetConfig config = WidgetRegistry.GetDefaultDBConfig();
            //config.DataProviderConfig.ConnectionDefinition = CurrentConnectionDefinition;

            ////GenericUniversalWidget widget = new GenericUniversalWidget(config);
            ////widget.DataProvider = new SchemaDataProvider(config.DataProviderConfig);
            ////widget.Dock = DockStyle.Fill;
            ////widget.StartDataProvider();

            //WindowsManager.PutInWindow(new System.Drawing.Size(800, 600), new QueryDesigner(), "Query Designer", CurrentConnectionDefinition);
        }

        public void FullScreenButton_Click(object sender, EventArgs e)
        {
            if (OnExpandRequested != null)
            {
                OnExpandRequested(this);
            }
        }

        //protected virtual void historyButton_Click(object sender, EventArgs e)
        //{
        //    WindowsManager.PutInWindow(new System.Drawing.Size(800, 600), new HistorylogPanel(), "SQL Whiteboard history", _theConfig.DataProviderConfig.ConnectionDefinition);
        //}

        protected virtual void cancelButton_Click(object sender, EventArgs e)
        {
            //stop the timer
            _theWidget.DataProvider.StopTimer();
            //stop fetching of data
            _theWidget.DataProvider.Stop();
            //Cleanup connection
            if (_theWidget.DataProvider is DatabaseDataProvider)
            {
                DatabaseDataProvider dbDataProvider = (DatabaseDataProvider)_theWidget.DataProvider;
                dbDataProvider.TheConnectionProvider.Cleanup();
            }
            //All the delegate for any additional cleanup
            if (_theDoCloseImpl != null)
            {
                _theDoCloseImpl(this);
            }
        }

        protected virtual void saveButton_Click(object sender, EventArgs e)
        {
            saveReport(false, true, true);
            //control report path and browse button status in report input window
            fireUpdateReportTab(null);

            //DatabaseDataProviderConfig dbDataProviderConfig = _theConfig.DataProviderConfig as DatabaseDataProviderConfig;
            //if (dbDataProviderConfig != null)
            //{
            //    dbDataProviderConfig.ConnectionDefinition = _theSystemsCombo.SelectedConnectionDefinition;
            //    dbDataProviderConfig.SQLText = _theQueryInputControl.QueryText;
            //}
            //SaveConfigurationDialog saveDialog = new SaveConfigurationDialog(_theConfig);
            ////string pathtoSave = GetPathToSave(_theConfig);
            ////if (pathtoSave != null)
            ////{
            ////    saveDialog.LibraryPath = pathtoSave;
            ////}
            //saveDialog.SetReportPathStatus(false);
            //saveDialog.ShowDialog();
            //if (saveDialog.SelectedOption == DialogResult.OK)
            //{
            //    try
            //    {
            //        string libraryPath = saveDialog.LibraryPath;
            //        string fileName = _theConfig.ReportFileName; //_theConfig.Name.EndsWith(".widget", StringComparison.InvariantCultureIgnoreCase) ? _theConfig.Name : string.Format("{0}.widget", _theConfig.Name);
            //        WidgetRegistry.GetInstance().SaveWidget(_theConfig, libraryPath, fileName);
            //        //Also update the description in the UI
            //        _theDescriptionDisplayUserControl.SetDescription(_theConfig);
            //    }
            //    catch (Exception ex)
            //    {
            //        MessageBox.Show(Utilities.GetForegroundControl(), "Error saving report : " + ex.Message,
            //            "Save Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            //    }
            //}
        }

        protected void saveReport(bool buttonFlag, bool pathFlag, bool updateFlag)
        {
            _theWidget.DataDisplayControl.PersistConfiguration();

            DatabaseDataProviderConfig dbDataProviderConfig = _theConfig.DataProviderConfig as DatabaseDataProviderConfig;
            if (dbDataProviderConfig != null)
            {
                dbDataProviderConfig.ConnectionDefinition = _theSystemsCombo.SelectedConnectionDefinition;
                dbDataProviderConfig.SQLText = _theQueryInputControl.QueryText;
            }
            SaveConfigurationDialog saveDialog = new SaveConfigurationDialog(_theConfig);
            string pathtoSave = GetPathToSave(_theConfig);
            if (pathtoSave != null)
            {
                saveDialog.LibraryPath = pathtoSave;
            }
            saveDialog.SetReportPathStatus(buttonFlag, pathFlag);
            saveDialog.ShowDialog();
            if (saveDialog.SelectedOption == DialogResult.OK)
            {
                try
                {
                    string libraryPath = saveDialog.LibraryPath;
                    string fileName = Path.GetFileName(_theConfig.ReportFileName); //_theConfig.Name.EndsWith(".widget", StringComparison.InvariantCultureIgnoreCase) ? _theConfig.Name : string.Format("{0}.widget", _theConfig.Name);

                    string fullPath = Path.Combine(libraryPath, Path.GetFileName(fileName));                   
                    //if not updating file, then Save Adhoc report
                    if (!updateFlag && File.Exists(fullPath))
                    {

                        DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), string.Format("File {0} exists. Do you want to overwrite it?", fullPath),
                                     "Overwrite file?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                        if (result != DialogResult.Yes)
                        {
                            return;
                        }   
                    }


                    _theConfig.CmbCatalogName = (_theCatalogsComboBox.SelectedTrafodionCatalog != null) ? _theCatalogsComboBox.SelectedTrafodionCatalog.ExternalName : null;
                    _theConfig.CmbSchemaName = (_theSchemasComboBox.SelectedTrafodionSchema != null) ? _theSchemasComboBox.SelectedTrafodionSchema.ExternalName : null;
                    _theWidget.FillInCurrentChartStatus(_theConfig);
                    WidgetRegistry.GetInstance().SaveWidget(_theConfig, libraryPath, fileName);
                    //Also update the description in the UI
                    _theDescriptionDisplayUserControl.SetDescription(_theConfig);

                    _theQueryInputControl.QueryText = ((DatabaseDataProviderConfig)_theConfig.DataProviderConfig).SQLText;
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Error saving report : " + ex.Message,
                        "Save Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

                if (updateFlag)
                {
                    initReportStatus();
                }
            }
            else
            {
                //If user click Cancel button from dialog, we don't need to retore sql text.

                //if (updateFlag)
                //{
                //    //since the SQL text is already changed, so we need to get original SQL text from local hard disk and restore it.
                //    string fullWidgetName = _theConfig.ReportPath.LastIndexOf("\\").Equals(_theConfig.ReportPath.Length - 1) ? _theConfig.ReportPath + _theConfig.ReportFileName :
                //        _theConfig.ReportPath +"\\"+_theConfig.ReportFileName;
                //    if (fullWidgetName != "\\")
                //    {
                //        UniversalWidgetConfig newWidgetConfig = WidgetRegistry.GetInstance().LoadWidgetFromFileWithoutRegistry(fullWidgetName);
                //        ((DatabaseDataProviderConfig)_theConfig.DataProviderConfig).SQLText = ((DatabaseDataProviderConfig)newWidgetConfig.DataProviderConfig).SQLText;
                //    }
                //}
            }
        }

        protected virtual void saveAsButton_Click(object sender, EventArgs e)
        {
            if (!string.IsNullOrEmpty(_theConfig.ReportPath) && !string.IsNullOrEmpty(_theConfig.ReportFileName))
            {
                string currentFilePath = Path.Combine(_theConfig.ReportPath, _theConfig.ReportFileName);
                UniversalWidgetConfig config = WidgetRegistry.DeepCopy(_theConfig);
                DatabaseDataProviderConfig dbDataProviderConfig = config.DataProviderConfig as DatabaseDataProviderConfig;
                if (dbDataProviderConfig != null)
                {
                    dbDataProviderConfig.ConnectionDefinition = _theSystemsCombo.SelectedConnectionDefinition;
                    dbDataProviderConfig.SQLText = _theQueryInputControl.QueryText;
                }
                config.Name = WidgetRegistry.GetWidgetPath(config.Name) + "Copy_of_" + WidgetRegistry.GetWidgetDisplayName(config.Name);
                config.ReportFileName = WidgetRegistry.GetWidgetDisplayName(config.Name) + ".widget";
                SaveConfigurationDialog saveDialog = new SaveConfigurationDialog(config);
                saveDialog.LibraryPath = "";
                //by default we want to save the new widget in the same location as the old one
                string pathtoSave = GetPathToSave(_theConfig);
                if (pathtoSave != null)
                {
                    saveDialog.LibraryPath = pathtoSave;
                }

                saveDialog.SetReportPathStatus(true, true);
                saveDialog.ShowDialog();
                if (saveDialog.SelectedOption == DialogResult.OK)
                {
                    string libraryPath = saveDialog.LibraryPath;
                    //string fileName = config.Name.EndsWith(".widget", StringComparison.InvariantCultureIgnoreCase) ? config.Name : string.Format("{0}.widget", config.Name);
                    string fileName = saveDialog.TheWidgetPropertyInputControl.ReportFileName;
                    string fullPath = Path.Combine(libraryPath, fileName);
                    bool isOverwriteAnExistingFIle = false;
                    if (File.Exists(fullPath))
                    {

                        DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), string.Format("File {0} exists. Do you want to overwrite it?", fullPath),
                                     "Overwrite file?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                        if (result != DialogResult.Yes)
                        {
                            return;
                        }
                        else
                        {
                            isOverwriteAnExistingFIle = true;
                        }
                    }
                    try
                    {
                        _theWidget.FillInCurrentChartStatus(config);
                        WidgetRegistry.GetInstance().SaveWidget(config, libraryPath, fileName);

                        if (isOverwriteAnExistingFIle)
                        {
                            bool isOverwritingCurrentFile = 0 == string.Compare(fullPath.Trim().ToLower(), currentFilePath.Trim().ToLower());
                            if (isOverwritingCurrentFile)
                            {
                                _theQueryInputControl.QueryText = ((DatabaseDataProviderConfig)config.DataProviderConfig).SQLText;
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "Error saving report : " + ex.Message,
                            "Save As Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }

                    //comment out because after SaveAS the report should retain edit mode.
                    //initReportStatus();

                    if (OnAfterSaveAs != null)
                    {
                        OnAfterSaveAs(this, config);
                    }

                }
            }
        }

        protected virtual void runButton_Click(object sender, EventArgs e)
        {
            executeQuery(false);
        }

        protected virtual void explainButton_Click(object sender, EventArgs e)
        {
            _theWidget.HideChartPanel();
            executeQuery(true);
        }

        private void executeQuery(bool explainMode)
        {
            ConnectionDefinition conDef = _theSystemsCombo.SelectedConnectionDefinition;
            if ((conDef != null) && (conDef.TheState == ConnectionDefinition.State.TestSucceeded))
            {
                if (string.IsNullOrEmpty(_theQueryInputControl.QueryText.Trim()))
                {
                    // This should not have been invoked; return anyway.
                    return;
                }

                DatabaseDataProviderConfig dbDataProviderConfig = _theConfig.DataProviderConfig as DatabaseDataProviderConfig;

                if (dbDataProviderConfig != null)
                {
                    dbDataProviderConfig.ConnectionDefinition = _theSystemsCombo.SelectedConnectionDefinition;

                    //we change all mappings if SQL is changed.
                    if (!_theQueryInputControl.QueryText.Equals(dbDataProviderConfig.SQLText))
                    {
                        dbDataProviderConfig.DefaultVisibleColumnNames = null;
                        dbDataProviderConfig.ColumnSortObjects = null;
                        //dbDataProviderConfig.ColumnMappings = null;
                        _theTabularDataDisplayControl.DataGrid.Clear();
                        _theTabularDataDisplayControl.DataGrid.Cols.Clear();
                        _theTabularDataDisplayControl.DataGrid.CurrentFilter = null;
                    }

                    dbDataProviderConfig.SQLText = _theQueryInputControl.QueryText;
                }
                DatabaseDataProvider databaseDataProvider = Widget.DataProvider as DatabaseDataProvider;
                if (databaseDataProvider != null)
                {
                    databaseDataProvider.ExplainMode = explainMode;
                }

                StartDataProvider();

                if (editMode)
                {
                    ////string sqlQuery = _theQueryInputControl.QueryText;
                    ////if (sqlQuery != null && sqlQuery.Trim() != "")
                    ////{
                    ////    bool flag = undoStack.Count == 0 && redoStack.Count == 0;
                    ////    undoStack.Push(sqlQuery);
                    ////    //for first time run sql, we do not need to update redo and undo status, since there is only one sql query. after user run second time then we need to enable undo button.
                    ////    if (!flag)
                    ////    {
                    ////        updateUndoRedoButtonStatus();
                    ////    }
                    ////}

                    runFlag = true;
                }
            }
            else
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Please select a valid connection from the System list before executing the query.",
                    "Invalid connection for query execution", MessageBoxButtons.OK, MessageBoxIcon.Error);

            }
        }

        //Gets the path where the config should be saved
        private string GetPathToSave(UniversalWidgetConfig config)
        {
            string path = null;
            WidgetRegistryEntry regEntry = WidgetRegistry.GetInstance().GetWidgetRegistryEntry(config.ReportID);
            if (regEntry != null)
            {
                try
                {
                    path = new FileInfo(regEntry.FullFileName).DirectoryName;
                }
                catch (Exception ex)
                {
                    //do nothing
                }
            }
            return path;
        }


        private void _theShowHideQuery_Click(object sender, EventArgs e)
        {
            this._theQuerySplitContainer.Panel1Collapsed = !this._theQuerySplitContainer.Panel1Collapsed;
            setShowHideValues();
        }

        protected virtual void AddMenuItems()
        {
            TabularDataDisplayControl dataDisplayControl = this._theWidget.DataDisplayControl as TabularDataDisplayControl;
            if (dataDisplayControl != null)
            {
                _drillDownMenuItem = new TrafodionIGridToolStripMenuItem();
                _drillDownMenuItem.Text = "Drill Down...";
                _drillDownMenuItem.Enabled = false;
                _drillDownMenuItem.Click += new EventHandler(_drillDownMenuItem_Click);
                dataDisplayControl.AddMenuItem(_drillDownMenuItem);
            }
        }

        void _drillDownMenuItem_Click(object sender, EventArgs e)
        {
            TabularDataDisplayControl dataDisplayControl = this._theWidget.DataDisplayControl as TabularDataDisplayControl;

            if ((_drillDownMenuItem.TrafodionIGridEventObject != null) && (dataDisplayControl != null))
            {
                dataDisplayControl.FireFireDrillDownForRow(_drillDownMenuItem.TrafodionIGridEventObject.Row);
            }
        }



        private void InvokeDataProvider_OnFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(DataProvider_OnFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.MetricMinner,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        void DataProvider_OnFetchingData(object sender, DataProviderEventArgs e)
        {
            //Disable the drill down menu item. This will be enabled once the data arrives
            if (_drillDownMenuItem != null)
            {
                _drillDownMenuItem.Enabled = false;
            }

            //Disable the explain button
            runButton.Enabled = false;
            explainButton.Enabled = false;

            //if it's not explain , just set the display to tabular. that way the UI will be cleaned in case of error
            DatabaseDataProvider dbDataProvider = this._theWidget.DataProvider as DatabaseDataProvider;
            if (dbDataProvider != null)
            {
                if (dbDataProvider.ExplainMode)
                {
                    _theWidget.HideChartPanel();
                }
                else
                {
                    if (_theWidget.DataDisplayControl != _theTabularDataDisplayControl)
                    {
                        _theWidget.DataDisplayControl = _theTabularDataDisplayControl;
                    }
                }
            }
        }

        private void InvokeDataProvider_OnErrorEncountered(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(DataProvider_OnErrorEncountered), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.MetricMinner,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        void DataProvider_OnErrorEncountered(object sender, DataProviderEventArgs e)
        {
            //Enable the explain button
            runButton.Enabled = true;
            explainButton.Enabled = true;

            //set the display to tabular. that way the UI will be cleaned 
            if (_theWidget.DataDisplayControl != _theTabularDataDisplayControl)
            {
                _theWidget.DataDisplayControl = _theTabularDataDisplayControl;
            }

        }

        private void InvokeDataProvider_OnNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(DataProvider_OnNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.MetricMinner,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        void DataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            ReportDefinition reportDef = e.GetEventProperty(DatabaseDataProvider.ReportDefinitionKey) as ReportDefinition;
            List<ReportParameter> reportParams = e.GetEventProperty(DatabaseDataProvider.ReportParametersKey) as List<ReportParameter>;

            //see which display control should be used and appropriately show it
            DatabaseDataProvider dbDataProvider = this._theWidget.DataProvider as DatabaseDataProvider;
            if (dbDataProvider.ExplainMode)
            {
                if (_theWidget.DataDisplayControl != _theQueryPlanDataDisplayControl)
                {
                    _theWidget.DataDisplayControl = _theQueryPlanDataDisplayControl;
                }
                _theWidget.HideChartPanel();
            }
            else
            {
                if (dbDataProvider.ReturnsTabularData)
                {
                    if (_theWidget.DataDisplayControl != _theTabularDataDisplayControl)
                    {
                        _theWidget.DataDisplayControl = _theTabularDataDisplayControl;
                    }
                }
                else
                {
                    if (_theWidget.DataDisplayControl != _theTextBoxDataDisplayControl)
                    {
                        _theWidget.DataDisplayControl = _theTextBoxDataDisplayControl;
                    }
                }
            }

            //Add the parameter tab if needed
            if ((reportParams != null) && (reportParams.Count > 0))
            {
                if (!_theQueryPropertyTabPanel.TabPages.Contains(_theParameterTab))
                {
                    _theQueryPropertyTabPanel.TabPages.Add(_theParameterTab);
                    //Persist the parameters
                    Trafodion.Manager.DatabaseArea.Queries.ReportParameterProcessor.Instance.persistReportParams(reportParams);
                    //Display the parameters
                    _thePrametersDisplayUserControl.ShowParameters(reportParams);
                }
            }

            //Add the status tab if needed
            if (!_theQueryPropertyTabPanel.TabPages.Contains(_theStatusTab) && (reportDef != null))
            {
                _theQueryPropertyTabPanel.TabPages.Insert(0, _theStatusTab);

            }
            //Display the status
            _theStatusDisplayUserControl.ShowStatus(reportDef);

            //bring the status tab to the front
            _theQueryPropertyTabPanel.SelectedTab = _theStatusTab;

            //Enable the drill down menu item
            if (_drillDownMenuItem != null)
            {
                _drillDownMenuItem.Enabled = true;
            }

            if (this._theReportSuccessImpl != null)
            {
                _theReportSuccessImpl(this, this.Parent as TabPage);
            }

            //Enable the explain button
            runButton.Enabled = true;
            explainButton.Enabled = true;

            //write to the historylog
            WriteTolog(reportDef, reportParams);
        }

        private void WriteTolog(ReportDefinition aReportDef, List<ReportParameter> reportParams)
        {
            HistorylogElement historyElement = new HistorylogElement();
            historyElement.ExecutionTime = DateTime.Now;
            historyElement.SqlText = aReportDef.GetProperty(ReportDefinition.ACTUAL_QUERY) as String;
            historyElement.Id = DateTime.Now.Ticks;
            historyElement.TheQueryStatus = HistorylogElement.QueryStatus.Success;

            long execTime = (long)aReportDef.GetProperty(ReportDefinition.LAST_EXECUTION_TIME);
            long fetchTime = (long)aReportDef.GetProperty(ReportDefinition.LAST_FETCH_TIME);
            long totalTime = execTime + fetchTime;
            string rowCount = aReportDef.GetProperty(ReportDefinition.ROWS_AFFECTED).ToString();
            historyElement.ExecutionStats = string.Format("Execution time: {0}, Fetch time: {1}, Total time: {2}, Rows affected/fetched: {3}",
                getFormattedTime(execTime), getFormattedTime(fetchTime), getFormattedTime(totalTime), rowCount);

            ConnectionDefinition connectionDef = _theConfig.DataProviderConfig.ConnectionDefinition;
            historyElement.ConnectionAttributes = string.Format("Host: {0}, Port: {1}, User: {2}, DSN: {3}, Role: {4}, Schema: {5}",
                filterNull(connectionDef.Host),
                filterNull(connectionDef.Port),
                filterNull(connectionDef.UserName),
                filterNull(connectionDef.ConnectedDataSource),
                filterNull(connectionDef.RoleName),
                filterNull(connectionDef.DefaultSchema));

            HistoryLogger.Instance.AddToLog(historyElement);
        }


        private string filterNull(string aString)
        {
            return (aString != null) ? aString : "";
        }
        private string getFormattedTime(long elapsedTicks)
        {
            TimeSpan elapsedSpan = new TimeSpan(elapsedTicks);
            return string.Format("{0:N2} secs", elapsedSpan.TotalSeconds);
        }

        private void setShowHideValues()
        {
            if (_theShowHideQuery.Tag.Equals("show"))
            {
                _theShowHideQuery.Image = global::Trafodion.Manager.Properties.Resources.ShowIcon;
                _theShowHideQuery.Tag = "hide";
                _theShowHideQuery.Text = "Hide Query";
            }
            else
            {
                _theShowHideQuery.Image = global::Trafodion.Manager.Properties.Resources.HideIcon;
                _theShowHideQuery.Tag = "show";
                _theShowHideQuery.Text = "Show Query";
            }
        }

        private void ShowRowDetailsImpl(int row)
        {
            if (!_theQueryPropertyTabPanel.TabPages.Contains(_theRowDetailsTab))
            {
                _theQueryPropertyTabPanel.TabPages.Add(_theRowDetailsTab);
            }
            this._theQueryPropertyTabPanel.SelectedTab = _theRowDetailsTab;
            this._theRowDisplayPanel.ShowRowDetails(((TabularDataDisplayControl)_theWidget.DataDisplayControl).DataGrid, row);

        }

        private void DoPostPopulateImpl()
        {
            _theConfig.LastExecutionTime = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
            //Once the populate has happened save the column layout
            _theWidget.DataDisplayControl.PersistConfiguration();
            //if (_theWidget.DataDisplayControl is TabularDataDisplayControl)
            //{
            //    ((TabularDataDisplayControl)_theWidget.DataDisplayControl).PopulateConfigToPersist();
            //}

            //Populate the description panel to display the description for this configuration
            _theDescriptionDisplayUserControl.SetDescription(_theConfig);
            if (_theWidget.UniversalWidgetConfiguration.SupportCharts)
            {
                _theWidget.ChartControl.PopulateChart(_theWidget.DataProvider.GetDataTable());
            }
        }

        /// <summary>
        /// 
        /// This is where the next widget to display determined. If there is only one associated
        /// widget, then that is selected otherwise the user is prompted with a dialog to select 
        /// the widget to display.
        /// 
        /// This is a helper method.
        /// </summary>
        /// <param name="widgetLinker"></param>
        public virtual void PopulateCalledWidgetImpl(UniversalWidgetConfig callerConfig, WidgetLinkerObject widgetLinker)
        {
            //UniversalWidgetConfig callerConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(widgetLinker.CallingWidget);
            if (callerConfig != null)
            {
                //List<AssociatedWidgetConfig> associatedWidgets = callerConfig.AssociatedWidgets;
                //if ((associatedWidgets != null) && (associatedWidgets.Count > 0))
                //{
                UniversalWidgetConfig config = null;
                WidgetSelectorDialog dialog = new WidgetSelectorDialog(_theConfig, widgetLinker);
                dialog.ShowDialog();
                string selectedWidget = dialog.SelectedWidget;
                if (selectedWidget != null)
                {
                    config = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(selectedWidget);
                    if (config != null)
                    {
                        //widgetLinker.CalledWidget = config.Name;
                        widgetLinker.CalledWidget = config.ReportID;

                        //get the updated parameters from user input
                        List<ReportParameter> updatedParams = dialog.GetUpdatedParameters();

                        //Update the widget linker RowHashtbale with the user provided values
                        //widgetLinker.UpdateFromUserInput(updatedParams);
                        UpdateFromUserInput(widgetLinker, updatedParams);

                        //Add the current catalog and system being used to the widget linker
                        if (widgetLinker.AdditionalParameters == null)
                        {
                            widgetLinker.AdditionalParameters = new System.Collections.Hashtable();
                        }
                        widgetLinker.AdditionalParameters.Add("CONNECTION", this._theSystemsCombo.SelectedConnectionDefinition);
                        widgetLinker.AdditionalParameters.Add("CATALOG", this._theSelectedTrafodionCatalog.ExternalName);
                        widgetLinker.AdditionalParameters.Add("SCHEMA", this._theSelectedTrafodionSchema.ExternalName);

                    }
                }
                else
                {
                    widgetLinker.CalledWidget = null;
                }
                //}
            }
        }

        public void UpdateFromUserInput(WidgetLinkerObject aWidgetLinker, List<ReportParameter> aUserInputs)
        {
            if (aUserInputs != null)
            {
                if ((aWidgetLinker.CallingWidget != null) && (aWidgetLinker.CalledWidget != null))
                {
                    UniversalWidgetConfig callingConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(aWidgetLinker.CallingWidget);
                    AssociatedWidgetConfig assocConfig = callingConfig.GetAssociationByID(aWidgetLinker.CallingWidget, aWidgetLinker.CalledWidget);
                    if ((assocConfig != null) && (assocConfig.ParameterMappings != null) && (assocConfig.ParameterMappings.Count > 0))
                    {
                        foreach (ParameterMapping pm in assocConfig.ParameterMappings)
                        {
                            ReportParameter reportParam = getReportParameterForName(aUserInputs, pm.ParamName);
                            if (reportParam != null)
                            {
                                if (aWidgetLinker.RowHashTable.ContainsKey(pm.ParamName))
                                {
                                    aWidgetLinker.RowHashTable.Remove(pm.ParamName);
                                }
                                aWidgetLinker.RowHashTable.Add(pm.ParamName, reportParam.Value);
                            }
                        }
                    }
                }
            }
        }

        private ReportParameter getReportParameterForName(List<ReportParameter> userInputs, string aParamName)
        {
            foreach (ReportParameter reportParam in userInputs)
            {
                if (reportParam.ParamName.Equals(aParamName))
                {
                    return reportParam;
                }
            }
            return null;
        }
        private void _theSchemasComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            _theSelectedTrafodionSchema = _theSchemasComboBox.SelectedTrafodionSchema;
            if (!_theSchemasComboBox.IsLoading && _theSchemasComboBox.SelectedTrafodionSchema != null)
            {
                _theSystemsCombo.SelectedSchemaName = _theSchemasComboBox.SelectedTrafodionSchema.ExternalName;
            }
            UpdateControls();
            saveCatalogAndSchema();
        }
        private void UpdateControls()
        {

            //_theAddButton.Enabled = CanAddOrUpdate;
            //_theUpdateButton.Enabled = CanAddOrUpdate;

            //_theExecuteButton.Enabled = CanExecuteOrExplain;
            //_theExplainButton.Enabled = CanExecuteOrExplain;

            //if (IsActiveSystemSelected)
            //{
            //    headerPanel.Controls.Clear();
            //    headerPanel.Hide();
            //}
            //else
            //{
            //    TrafodionLabel label = new TrafodionLabel();
            //    label.ForeColor = System.Drawing.Color.Red;
            //    label.Text = Properties.Resources.SelectSystemPrompt;
            //    label.Dock = DockStyle.Fill;
            //    headerPanel.Controls.Add(label);
            //    headerPanel.Show();
            //}

            {
                showCatalogsCombo(true);
            }
        }

        private void _theSystemsCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            //SelectConnectionDefinition(_theSystemsCombo.SelectedConnectionDefinition);
            if (!_theSystemsCombo.IsLoading)
            {
                _theSelectedConnectionDefinition = _theSystemsCombo.SelectedConnectionDefinition;
                if (_theSelectedConnectionDefinition != null)
                {
                    if (_theSelectedConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
                    {
                        ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog(false);
                        theConnectionDefinitionDialog.Edit(_theSelectedConnectionDefinition);
                    }
                    if (_theSelectedConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                    {
                        Cursor.Current = Cursors.WaitCursor;

                        try
                        {
                            CatalogAndSchema CandS = MetricMinerController.Instance.GetCatalogAndSchema(_theSelectedConnectionDefinition.Name);
                            _theCatalogsComboBox.TheTrafodionSystem = TrafodionSystem.FindTrafodionSystem(_theSelectedConnectionDefinition);
                            setCatalog(CandS);
                            setSchema(CandS);
                            //try
                            //{

                            //    _theCatalogsComboBox.SelectedTrafodionCatalog = _theCatalogsComboBox.TheTrafodionSystem.FindCatalog(_theSystemsCombo.SelectedCatalogName);
                            //    _theSchemasComboBox.TheTrafodionCatalog = _theCatalogsComboBox.SelectedTrafodionCatalog;
                            //}
                            //catch (Exception ex)
                            //{
                            //    if (_theCatalogsComboBox.Items.Count > 0)
                            //    {
                            //        _theCatalogsComboBox.SelectedIndex = 0;
                            //    }
                            //    else
                            //    {
                            //        _theCatalogsComboBox.SelectedIndex = -1;
                            //    }
                            //}

                            //if (_theSystemsCombo.SelectedSchemaName != null)
                            //{
                            //    if (!String.IsNullOrEmpty(_theSystemsCombo.SelectedSchemaName))
                            //    {
                            //        try
                            //        {
                            //            _theSchemasComboBox.SelectedTrafodionSchema = _theCatalogsComboBox.SelectedTrafodionCatalog.FindSchema(_theSystemsCombo.SelectedSchemaName);
                            //        }
                            //        catch (Exception ex)
                            //        {
                            //            if (_theSchemasComboBox.Items.Count > 0)
                            //            {
                            //                _theSchemasComboBox.SelectedIndex = 0;
                            //            }
                            //            else
                            //            {
                            //                _theSchemasComboBox.SelectedIndex = -1;
                            //            }
                            //        }
                            //    }
                            //}
                        }
                        catch (Exception e1)
                        {

                        }
                        finally
                        {
                            Cursor.Current = Cursors.Default;
                        }
                        TrafodionContext.Instance.OnConnectionDefinitionSelection(this);
                    }
                    else
                    {
                        _theSystemsCombo.SelectedIndex = -1;
                        _theCatalogsComboBox.TheTrafodionSystem = null;
                        _theSchemasComboBox.TheTrafodionCatalog = null;
                    }
                }
            }

            UpdateControls();
        }

        private void _theCatalogsComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            _theSelectedTrafodionCatalog = _theCatalogsComboBox.SelectedTrafodionCatalog;
            _theSchemasComboBox.TheTrafodionCatalog = _theSelectedTrafodionCatalog;
            if (!_theCatalogsComboBox.IsLoading && _theCatalogsComboBox.SelectedTrafodionCatalog != null)
            {
                _theSystemsCombo.SelectedCatalogName = _theCatalogsComboBox.SelectedTrafodionCatalog.ExternalName;
            }
            UpdateControls();
            saveCatalogAndSchema();
        }

        private void saveCatalogAndSchema()
        {
            string connectionName = null;
            string catalogName = null;
            string schemaName = null;
            if (!_theSystemsCombo.IsLoading)
            {
                _theSelectedConnectionDefinition = _theSystemsCombo.SelectedConnectionDefinition;
                if (_theSelectedConnectionDefinition != null)
                {
                    connectionName = _theSelectedConnectionDefinition.Name;
                }
            }

            if (!_theCatalogsComboBox.IsLoading && _theCatalogsComboBox.SelectedTrafodionCatalog != null)
            {
                catalogName = _theCatalogsComboBox.SelectedTrafodionCatalog.ExternalName;
            }
            if (!_theSchemasComboBox.IsLoading && _theSchemasComboBox.SelectedTrafodionSchema != null)
            {
                schemaName = _theSchemasComboBox.SelectedTrafodionSchema.ExternalName;
            }

            if ((connectionName != null) && (catalogName != null) && (schemaName != null))
            {
                MetricMinerController.Instance.SaveCatalogAndSchema(connectionName, catalogName, schemaName);
            }
        }
        private void setCatalog(CatalogAndSchema CandS)
        {
            try
            {
                if ((CandS != null) && (CandS.CatalogName != null))
                {
                    _theCatalogsComboBox.SelectedTrafodionCatalog = _theCatalogsComboBox.TheTrafodionSystem.FindCatalog(CandS.CatalogName);
                    _theSchemasComboBox.TheTrafodionCatalog = _theCatalogsComboBox.SelectedTrafodionCatalog;
                }
                else
                {
                    _theCatalogsComboBox.SelectedTrafodionCatalog = _theCatalogsComboBox.TheTrafodionSystem.FindCatalog(_theSystemsCombo.SelectedCatalogName);
                    _theSchemasComboBox.TheTrafodionCatalog = _theCatalogsComboBox.SelectedTrafodionCatalog;
                }
            }
            catch (Exception ex)
            {
                if (_theCatalogsComboBox.Items.Count > 0)
                {
                    _theCatalogsComboBox.SelectedIndex = 0;
                }
                else
                {
                    _theCatalogsComboBox.SelectedIndex = -1;
                }
            }
        }
        private void setSchema(CatalogAndSchema CandS)
        {
            try
            {
                if ((CandS != null) && (CandS.SchemaName != null))
                {
                    _theSchemasComboBox.SelectedTrafodionSchema = _theCatalogsComboBox.SelectedTrafodionCatalog.FindSchema(CandS.SchemaName);
                }
                else
                {
                    if (_theSystemsCombo.SelectedSchemaName != null)
                    {
                        if (!String.IsNullOrEmpty(_theSystemsCombo.SelectedSchemaName))
                        {
                            _theSchemasComboBox.SelectedTrafodionSchema = _theCatalogsComboBox.SelectedTrafodionCatalog.FindSchema(_theSystemsCombo.SelectedSchemaName);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                if (_theSchemasComboBox.Items.Count > 0)
                {
                    _theSchemasComboBox.SelectedIndex = 0;
                }
                else
                {
                    _theSchemasComboBox.SelectedIndex = -1;
                }
            }
        }

        private void _theQueryInputControl_OnQueryTextChanged(object aSender, EventArgs e)
        {
            // call update control
            if (_theQueryInputControl.QueryText.Trim().Length == 0)
            {
                runButton.Enabled = false;
                explainButton.Enabled = false;
            }
            else
            {
                runButton.Enabled = true;
                explainButton.Enabled = true;
            }
            updateUndoRedoButtonStatus();
        }
    }

    /// <summary>
    /// Custom data display handler for the Repository Reports widget
    /// </summary>
    public class RepositoryReportsWidgetDataHandler : TabularDataDisplayHandler
    {
        public delegate void DoPostPopulate();
        private DoPostPopulate _theDoPostPopulateImpl;

        public DoPostPopulate DoPostPopulateImpl
        {
            get { return _theDoPostPopulateImpl; }
            set { _theDoPostPopulateImpl = value; }
        }
        /// <summary>
        /// Populate the Repository Reports Grid
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        public override void DoPopulate(UniversalWidgetConfig aConfig,
            System.Data.DataTable aDataTable,
            Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            base.DoPopulate(aConfig, aDataTable, aDataGrid);

            //This is where the description will get updated based on the columns returned 
            //by the query
            if (_theDoPostPopulateImpl != null)
            {
                _theDoPostPopulateImpl();
            }
            //We wanted to add a column to visually indicate that drill down is possible from this row
            //but with the new scheme that drill down is possible from any report, this becomes moot

            //if ((aConfig.AssociatedWidgets != null) && (aConfig.AssociatedWidgets.Count > 0))
            //{
            //    ImageList imgList = new ImageList();
            //    imgList.Images.Add(global::Trafodion.Manager.Properties.Resources.DrillDownIcon);

            //    string key = "drill_down";
            //    iGCellStyle statusStyle = new iGCellStyle();
            //    statusStyle.Flags = TenTec.Windows.iGridLib.iGCellFlags.DisplayImage;
            //    statusStyle.ImageAlign = TenTec.Windows.iGridLib.iGContentAlignment.MiddleCenter;
            //    statusStyle.ImageList = imgList;
            //    statusStyle.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            //    statusStyle.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            //    statusStyle.Selectable = TenTec.Windows.iGridLib.iGBool.False;
            //    statusStyle.ReadOnly = iGBool.True;

            //    iGColPattern colPattern = new iGColPattern();
            //    colPattern.CellStyle = statusStyle;
            //    colPattern.Key = key;
            //    colPattern.SortType = TenTec.Windows.iGridLib.iGSortType.ByImageIndex;
            //    colPattern.Text = key;
            //    colPattern.Width = 60;
            //    colPattern.AllowMoving = false;
            //    colPattern.AllowSizing = false;
            //    colPattern.ImageIndex = 0;
            //    colPattern.DefaultCellImageIndex = 0;

            //    aDataGrid.FrozenArea.ColCount = 1;
            //    aDataGrid.Header.UseXPStyles = true;
            //    aDataGrid.RowMode = true;

            //    aDataGrid.Cols.Insert(0, key, "", 20, colPattern);
            //}
        }
    }
}
