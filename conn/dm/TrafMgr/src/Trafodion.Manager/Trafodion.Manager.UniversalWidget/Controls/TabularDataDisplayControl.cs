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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    public partial class TabularDataDisplayControl : UserControl, IDataDisplayControl
    {
        private DrillDownManager _theDrillDownManager;
        private UniversalWidgetConfig _theConfig;
        private DataTable _theDataTable;
        private TrafodionIGridHyperlinkCellManager _theHyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
        private DataProvider _theDataProvider = null;
        private IDataDisplayHandler _theDataDisplayHandler;
        private List<TrafodionIGridToolStripMenuItem> _theCustomMenuItems = new List<TrafodionIGridToolStripMenuItem>();
        private bool _theShowExportButtons = false;
        private string _theLineCountFormat = null;
        private bool _theKeepDataGridOnError = false;
        public delegate void UpdateStatus(Object obj, EventArgs e);

        public TabularDataDisplayControl()
        {
            InitializeComponent();

            //Create the default data display handler. 
            _theDataDisplayHandler = new TabularDataDisplayHandler();

            _theDataGrid.ColWidthEndChange += new iGColWidthEventHandler(DataGrid_ColumnWidthEndChanged);
            _theDataGrid.AfterContentsSorted += DataGrid_ContentSorted;
            _theDataGrid.OnShowHideColumnChanged += DataGrid_DisplayConfigChanged;
            _theDataGrid.ColHdrEndDrag += DataGrid_ColumnDragged;

            _theDrillDownManager = new DrillDownManager();
            _theDrillDownManager.DataDisplayControl = this;
            //in order to search item forward and backward
            _theDataGrid.SearchAsType.StartFromCurRow = false;
            _theDataGrid.SearchAsType.LoopSearch = true;
        }




        public TabularDataDisplayControl(UniversalWidgetConfig aConfig) : this()
        {
            UniversalWidgetConfiguration = aConfig;
           
        }


        /// <summary>
        /// Get/Set the DrillDownManager associated with this control
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get { return _theDrillDownManager; }
            set { _theDrillDownManager = value; }
        }

        /// <summary>
        /// Display the export buttons at the bottom of the panel
        /// </summary>
        public bool ShowExportButtons
        {
            get { return _theShowExportButtons; }
            set { _theShowExportButtons = value; }
        }

        /// <summary>
        /// Specify the count format for the table
        /// </summary>
        public string LineCountFormat
        {
            get { return _theLineCountFormat; }
            set { _theLineCountFormat = value; }
        }

        /// <summary>
        /// The TrafodionIGrid that diaplays the data
        /// </summary>
        public Trafodion.Manager.Framework.Controls.TrafodionIGrid DataGrid
        {
            get { return _theDataGrid; }
            set 
            {
                if (_theDataGrid != value)
                {
                    if (_theDataGrid != null)
                    {
                        _theDataGrid.OnShowHideColumnChanged -= DataGrid_DisplayConfigChanged;
                        _theDataGrid.ColHdrEndDrag -= DataGrid_ColumnDragged;
                        _theDataGrid.ColWidthEndChange -= DataGrid_ColumnWidthEndChanged;
                        _theDataGrid.AfterContentsSorted -= DataGrid_ContentSorted;
                    }
                    _theDataGrid = value;
                    _theDataGrid.OnShowHideColumnChanged += DataGrid_DisplayConfigChanged;
                    _theDataGrid.ColHdrEndDrag += DataGrid_ColumnDragged;
                    _theDataGrid.ColWidthEndChange += DataGrid_ColumnWidthEndChanged;
                    _theDataGrid.AfterContentsSorted += DataGrid_ContentSorted;
                }
            }
        }

        /// <summary>
        /// The data display handler. It can be used to get the current handler
        /// and set any  custom handler
        /// </summary>
        public  IDataDisplayHandler DataDisplayHandler
        {
            get { return _theDataDisplayHandler; }
            set { _theDataDisplayHandler = value; }
        }

        /// <summary>
        /// Associate the configuration to the chart. 
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _theConfig; }
            set
            {
                _theConfig = value;
                _theDrillDownManager.Config = _theConfig;
                InitializeTableControl();
            }
        }

        public DataProvider DataProvider
        {
            get { return _theDataProvider; }
            set
            {
                if (_theDataProvider != null)
                {
                    RemoveHandlers();
                }
                _theDataProvider = value;
                AddHandlers();
            }
        }

        /// <summary>
        /// To keep the data in the DataGrid on error. In other words, not to clear 
        /// DataGrid when an exception is encountered. 
        /// </summary>
        public bool KeepDataGridOnError
        {
            get { return _theKeepDataGridOnError; }
            set { _theKeepDataGridOnError = value; }
        }

        /// <summary>
        /// Persist the configuration
        /// </summary>
        public void PersistConfiguration()
        {
            PopulateConfigToPersist();
        }

        /// <summary>
        /// The custom menu items that need to be diaplyed for the table
        /// </summary>
        public List<TrafodionIGridToolStripMenuItem> CustomMenuItems
        {
            get { return _theCustomMenuItems; }
            set { _theCustomMenuItems = value; }
        }

        /// <summary>
        /// Helper method to apply the column attributes like width, sort order and visibility
        /// </summary>
        /// <param name="aDataGrid"></param>
        /// <param name="aDataProviderConfig"></param>
        public static void ApplyColumnAttributes(TrafodionIGrid aDataGrid, DataProviderConfig aDataProviderConfig)
        {
            //Newly added columns may not be visible. Make sure they are made visible.
            ShowNewlyAddedColumn(aDataGrid, aDataProviderConfig);

            //If we already have persisted the earlier visible columns, use that.
            if ((aDataProviderConfig.CurrentVisibleColumnNames != null) && (aDataProviderConfig.CurrentVisibleColumnNames.Count > 0))
            {
                //set the current filter for the igrid
                if (aDataGrid.CurrentFilter == null)
                {
                    aDataGrid.CurrentFilter = new TrafodionIGridFilter(aDataGrid, aDataProviderConfig.CurrentVisibleColumnNames, aDataProviderConfig.DefaultVisibleColumnNames);
                }

                //Bring the Filter and the DataGrid in sync in terms of columns added/removed since the last fetch
                if (!aDataGrid.CurrentFilter.Visible)
                {
                    aDataGrid.CurrentFilter.CorrectItemList(aDataGrid);
                }

                //set the column widths
                foreach (string columnName in aDataProviderConfig.CurrentVisibleColumnNames)
                {
                    ColumnMapping cm = aDataProviderConfig.GetColumnMappingForInternalName(columnName);
                    if (cm != null)
                    {
                        if (aDataGrid.Cols.KeyExists(columnName))
                        {
                            aDataGrid.Cols[columnName].Width = cm.ColumnWidth;
                        }
                    }
                }
            }
            else
            {
                aDataGrid.Cols.AutoWidth();
            }


            //apply sort 
            if (aDataProviderConfig.ColumnSortObjects != null)
            {
                foreach (ColumnSortObject cso in aDataProviderConfig.ColumnSortObjects)
                {
                    if (cso.ColIndex >= 0 && cso.ColIndex < aDataGrid.Cols.Count)
                    {
                        aDataGrid.SortObject.Add(cso.ColIndex, (iGSortOrder)cso.SortOrder);
                    }
                }

                if (aDataGrid.SortObject.Count > 0)
                {
                    aDataGrid.Sort();
                }
            }

            //Apply the filter
            aDataGrid.ApplyFilter();
            aDataGrid.SetAutoRowHeight();
        }

        /// <summary>
        /// Compares the list of columns that existed before fetch to the new columns after fetch and 
        /// makes the new columns visible, and also
        /// remove inexistent columns.
        /// </summary>
        /// <param name="aDataGrid"></param>
        /// <param name="aDataProviderConfig"></param>
        private static void ShowNewlyAddedColumn(TrafodionIGrid aDataGrid, DataProviderConfig aDataProviderConfig)
        {
            if (aDataProviderConfig.CurrentVisibleColumnNames == null) return;

            /*
             * Code below is to remove the dirty visible columns which is brough in by code bug.
             * The solution is to remove the columns which belong to always hiddent columns.
            */
            if (aDataGrid.AlwaysHiddenColumnNames != null && aDataGrid.AlwaysHiddenColumnNames.Count > 0)
            {
                for (int i = aDataProviderConfig.CurrentVisibleColumnNames.Count - 1; i >= 0; i--)
                {
                    string visibleColumnName = aDataProviderConfig.CurrentVisibleColumnNames[i];
                    if (aDataGrid.AlwaysHiddenColumnNames.Contains(visibleColumnName))
                    {
                        aDataProviderConfig.CurrentVisibleColumnNames.RemoveAt(i);
                    }
                }
            }


            if (aDataGrid.Cols == null) return;

            /*
             * If it's an inexistent column from the grid columsn, the column should be removed from CurrentVisibleColumnNames
            */
            for (int i = aDataProviderConfig.CurrentVisibleColumnNames.Count - 1; i >= 0; i--)
            {
                string columnName = aDataProviderConfig.CurrentVisibleColumnNames[i];
                bool existing = false;
                foreach (iGCol gridColumn in aDataGrid.Cols)
                {
                    if (0 == string.Compare(gridColumn.Key, columnName, true))
                    {
                        existing = true;
                        break;
                    }
                }
                if (!existing)
                {
                    aDataProviderConfig.CurrentVisibleColumnNames.RemoveAt(i);
                }
            }
            

            /*
             * If it's a new column that did not exist for the last fetch, and did not belong to always hidden columns,              
             * 1. make it visible in the Grid - by setting Visible true
             * 2. make it checked in the filter - by adding it into the Current Visible Column Names collection.
            */
            int cnt = aDataGrid.Cols.Count;
            for (int idx = 0, visibleColumnIndex = 0; idx < cnt; idx++)
            {
                iGCol gridColumn = aDataGrid.Cols.FromOrder(idx);

                string columnName = gridColumn.Key;
                if (!aDataProviderConfig.PrefetchColumnNameList.Contains(columnName))
                {
                    gridColumn.Visible = true;
                    if (!aDataProviderConfig.CurrentVisibleColumnNames.Contains(columnName))
                    {
                        aDataProviderConfig.CurrentVisibleColumnNames.Insert(visibleColumnIndex, columnName);
                    }
                }

                if (aDataProviderConfig.CurrentVisibleColumnNames.Contains(columnName))
                {
                    visibleColumnIndex++;
                }
            }
        }

        /// <summary>
        /// This method populates the config with information that needs to be persisted
        /// </summary>
        private void PopulateConfigToPersist()
        {
            //Get the visible columns
            List<string> visibleColumnNames = new List<string>();
            List<string> prefetchColumnNames = new List<string>();
            if (_theDataGrid.AlwaysHiddenColumnNames != null)
            {
                prefetchColumnNames.AddRange(_theDataGrid.AlwaysHiddenColumnNames);
            }

            if (_theDataGrid.Cols.Count > 0)
            {
                int cnt = _theDataGrid.Cols.Count;
                for (int idx = 0; idx < cnt; idx++)
                {
                    iGCol igc = _theDataGrid.Cols.FromOrder(idx);
                    if (igc.Visible)
                    {
                        visibleColumnNames.Add(igc.Key);
                    }

                    if (!prefetchColumnNames.Contains(igc.Key))
                    {
                        prefetchColumnNames.Add(igc.Key);
                    }
                }

                if (visibleColumnNames.Count > 0)
                {
                    DataProvider.DataProviderConfig.CurrentVisibleColumnNames = visibleColumnNames;
                }

                DataProvider.DataProviderConfig.PrefetchColumnNameList = prefetchColumnNames;

                if (DataProvider.DataProviderConfig.ColumnMappings == null || DataProvider.DataProviderConfig.ColumnMappings.Count == 0)
                {
                    //Create column mappings
                    List<ColumnMapping> colMappings = new List<ColumnMapping>();
                    foreach (string columnName in visibleColumnNames)
                    {
                        iGCol col = _theDataGrid.Cols[columnName];
                        ColumnMapping cm = new ColumnMapping();

                        cm.InternalName = columnName;
                        cm.ExternalName = col.Text as string;
                        cm.ColumnWidth = col.Width;
                        colMappings.Add(cm);
                    }
                    DataProvider.DataProviderConfig.ColumnMappings = colMappings;
                }

                //save the column sort order
                DataProvider.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>();
                iGSortItem sortItem = null;
                for (int i = 0; i < _theDataGrid.SortObject.Count; i++)
                {
                    sortItem = _theDataGrid.SortObject[i];
                    DataProvider.DataProviderConfig.ColumnSortObjects.Add(new ColumnSortObject(sortItem.ColIndex, sortItem.Index, (int)sortItem.SortOrder));
                }
            }
        }


        /// <summary>
        /// Adds a custom menu item that will be displayed when a user right clicks on the table
        /// </summary>
        /// <param name="aTrafodionIGridToolStripMenuItem"></param>
        public void AddMenuItem(TrafodionIGridToolStripMenuItem aTrafodionIGridToolStripMenuItem)
        {
            this._theCustomMenuItems.Add(aTrafodionIGridToolStripMenuItem);
            this._theDataGrid.AddContextMenu(aTrafodionIGridToolStripMenuItem);
        }

        /// <summary>
        /// Adds a tool strip separator that will be displayed when a user right clicks on the table
        /// </summary>
        /// <param name="aTrafodionIGridToolStripMenuItem"></param>
        public void AddToolStripSeparator(ToolStripSeparator aToolStripSeparator)
        {
            this._theDataGrid.AddToolStripSeparator(aToolStripSeparator);
        }

        private void InitializeTableControl()
        {
            _theDataGrid.RowMode = false;
            _theDataGrid.RowSelectionInCellMode = iGRowSelectionInCellModeTypes.None;
            _theDataGrid.SelectionMode = iGSelectionMode.MultiExtended;

            //Add button control if needed
            if (ShowExportButtons)
            {
                _theDataGrid.AddButtonControlToParent(DockStyle.Bottom);
            }

            //Show row counters if needed
            if ((LineCountFormat != null) && (LineCountFormat.Trim().Length > 0))
            {
                _theDataGrid.AddCountControlToParent(LineCountFormat.Trim(), DockStyle.Top);
            }
        }

        /// <summary>
        /// Cleanup
        /// </summary>
        /// <param name="disposing"></param>
        void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_theHyperLinkCellManager != null)
                {
                    _theHyperLinkCellManager.Detach();
                }
                //Stop the data provider
                if (_theDataProvider != null)
                {
                    _theDataProvider.Stop();
                }

                //Remove the event handlers
                this.RemoveHandlers();

                // remove the event handler for grid
                if (_theDataGrid != null)
                {
                    _theDataGrid.OnShowHideColumnChanged -= DataGrid_DisplayConfigChanged;
                    _theDataGrid.ColHdrEndDrag -= DataGrid_ColumnDragged;
                    _theDataGrid.ColWidthEndChange -= DataGrid_ColumnWidthEndChanged;
                    _theDataGrid.AfterContentsSorted -= DataGrid_ContentSorted;
                }
            }
        }

        private void RecordColumnToGridConfig()
        {
            DataProviderConfig config = this._theConfig.DataProviderConfig;

            if (config.CurrentVisibleColumnNames == null)
            {
                config.CurrentVisibleColumnNames = new List<string>();
            }
            else
            {
                config.CurrentVisibleColumnNames.Clear();
            }

            for (int i = 0; i < this._theDataGrid.Cols.Count; i++)
            {
                iGCol igc = this._theDataGrid.Cols.FromOrder(i);
                if (igc.Visible)
                {
                    config.CurrentVisibleColumnNames.Add(igc.Key);
                }
            }
        }

        void DataGrid_DisplayConfigChanged(object sender, EventArgs eArgs)
        {
            RecordColumnToGridConfig();
            this._theConfig.Persist();
        }

        /// <summary>
        /// Have to manually calculate the column order, due to the strange logic of event Grid.ColHdrEndDrag
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DataGrid_ColumnDragged(object sender, iGColHdrEndDragEventArgs e)
        {
            RecordColumnToGridConfig();

            DataProviderConfig config = this._theConfig.DataProviderConfig;
            string draggedColumn = this._theDataGrid.Cols[e.ColIndex].Key;
            TrafodionIGridUtils.MoveConfigColumnOrder(this._theDataGrid.Cols, config.CurrentVisibleColumnNames, draggedColumn, e.NewOrder);

            if (this._theDataGrid.CurrentFilter != null && !this._theDataGrid.CurrentFilter.Visible)
            {
                this._theDataGrid.CurrentFilter.CurrentVisibleColumns = config.CurrentVisibleColumnNames;
            }
            
            this._theConfig.Persist();
        }

        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleError), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UW,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        protected virtual void HandleError(Object obj, EventArgs e)
        {
            //Update status bar with exception
            if (!KeepDataGridOnError)
            {
            this._theDataGrid.Clear();
            _theDataTable = null;
            //Detach any existing managers
            _theHyperLinkCellManager.Detach();
        }
        }


        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
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
            Populate();

            //Update the status bar with chart status
        }

        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
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
            if(!string.IsNullOrEmpty(LineCountFormat))
            {
                _theDataGrid.UpdateCountControlText(LineCountFormat);
            }
        }

        /// <summary>
        /// Re-draws the graph in the UI
        /// </summary>
        protected virtual void Populate()
        {
            //DataProvider dataProvider = UniversalWidgetConfiguration.DataProvider;
            _theDataTable = _theDataProvider.GetDataTable();
            try
            {
                //_theDataGrid.BeginUpdate();
                DataDisplayHandler.DoPopulate(UniversalWidgetConfiguration, _theDataTable, _theDataGrid);
                //ApplyColumnAttributes();
                //_theDataGrid.EndUpdate();

            }
            catch (Exception ex)
            {
                MessageBox.Show("Error encountered while populating table - " + ex.Message, "Error in table population", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Trafodion.Manager.Framework.Logger.OutputErrorLog(ex.StackTrace);
            }
        }

        private void DataGrid_ColumnWidthEndChanged(object sender, iGColWidthEventArgs e)
        {
            // Record ColumnMappingInfos
            DataProviderConfig config = this._theConfig.DataProviderConfig;

            if (config.ColumnMappings == null)
            {
                config.ColumnMappings = new List<ColumnMapping>();
            }
            else
            {
                config.ColumnMappings.Clear();
            }

            foreach (iGCol gridColumn in this._theDataGrid.Cols)
            {
                if (gridColumn.Visible)
                {
                    ColumnMapping cm = new ColumnMapping();
                    cm.InternalName = gridColumn.Key;
                    cm.ExternalName = gridColumn.Text as string;
                    cm.ColumnWidth = gridColumn.Width;

                    config.ColumnMappings.Add(cm);
                }
            }

            this._theConfig.Persist();
        }

        void DataGrid_ContentSorted(object sender, EventArgs args)
        {
            DataProviderConfig config = this._theConfig.DataProviderConfig;

            // Record ColumnSorts
            if (config.ColumnSortObjects == null)
            {
                config.ColumnSortObjects = new List<ColumnSortObject>();
            }
            else
            {
                config.ColumnSortObjects.Clear();
            }

            for (int i = 0; i < this._theDataGrid.SortObject.Count; i++)
            {
                iGSortItem sort = this._theDataGrid.SortObject[i];
                config.ColumnSortObjects.Add(new ColumnSortObject(sort.ColIndex, sort.Index, (int)sort.SortOrder));
            }

            this._theConfig.Persist();
        }

        public void FireDrillDownRequested(DrillDownEventArgs e)
        {
            if (_theDrillDownManager != null) 
            {
                _theDrillDownManager.FireDrillDownRequested(this, e);
            }
        }
        /// <summary>
        /// The cell click will be used as a drill down event for the table
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void _theDataGrid_CellDoubleClick(object sender, iGCellDoubleClickEventArgs e)//(object sender, iGCellClickEventArgs e)
        {
            int row = e.RowIndex;
            //Clicked on the heading, hence nothing to do

            FireFireDrillDownForRow(row);
        }

        /// <summary>
        /// Given the selected row, fires a drill down event for that row
        /// </summary>
        /// <param name="row"></param>
        public void FireFireDrillDownForRow(int row)
        {
            //If the report id is null, there are no linked reports to drill down.
            if (row < 0 || string.IsNullOrEmpty(UniversalWidgetConfiguration.ReportID))
            {
                return;
            }

            //Instantiate the object and pass the parameters that will be needed to display the 
            //next widget
            WidgetLinkerObject widgetLinker = new WidgetLinkerObject();
            widgetLinker.CallingWidget = UniversalWidgetConfiguration.ReportID;

            //Get the data of the entire row that the user clicked on
            Hashtable rowHashTable = new Hashtable();
            int columnCount = _theDataGrid.Rows[row].Cells.Count;

            //Load the hashtable with data corresponding to the row
            for (int i = 0; i < columnCount; i++)
            {
                rowHashTable.Add(_theDataGrid.Cols[i].Key, _theDataGrid.Rows[row].Cells[i].Value);
                //rowHashTable.Add(dataTable.Columns[i].Caption, _theDataGrid.Rows[row].Cells[i].Value);
            }
            widgetLinker.RowHashTable = rowHashTable;

            //Set the entire data table
            widgetLinker.CallingWidgetDataTable = _theDataTable;

            DrillDownEventArgs eventArgs = new DrillDownEventArgs();
            eventArgs.WidgetLinkerObject = widgetLinker;

            FireDrillDownRequested(eventArgs);
        }

        private void AddHandlers()
        {
            if (_theDataProvider != null)
            {
                //Associate the event handlers
                _theDataProvider.OnErrorEncountered += InvokeHandleError;
                _theDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _theDataProvider.OnFetchingData += InvokeHandleFetchingData;

               // _theDataGrid.ColHdrMouseDown += _theDataGrid_ColHdrMouseDown;
            }
        }

        void _theDataGrid_ColHdrMouseDown(object sender, iGColHdrMouseDownEventArgs e)
        {
            iGColHdr header = _theDataGrid.Header.Cells[e.RowIndex, e.ColIndex];
            _theDataGrid.DoDragDrop(header.ColKey, DragDropEffects.Link);
        }

        private void RemoveHandlers()
        {
            if (_theDataProvider != null)
            {
                //Remove the event handlers
                _theDataProvider.OnErrorEncountered -= InvokeHandleError;
                _theDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theDataProvider.OnFetchingData -= InvokeHandleFetchingData;
                //_theDataGrid.ColHdrMouseDown -= _theDataGrid_ColHdrMouseDown;
            }
        }
    }
}
