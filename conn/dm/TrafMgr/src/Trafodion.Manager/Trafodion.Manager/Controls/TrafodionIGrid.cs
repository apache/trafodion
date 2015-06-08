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
using System.ComponentModel;
using System.IO;
using System.Data;
using System.Drawing;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework;
using System.Collections;
namespace Trafodion.Manager.Framework.Controls
{
    public class TrafodionIGrid : iGrid
    {
        private const string GRID_CONFIGS_PERSISTENCE_KEY = "GridConfigs";
        private const int ADJUSTED_VISBILE_WIDTH = 80;
        private static readonly object syncRoot = new object();

        List<ToolStripItem> _theMenuItems = new List<ToolStripItem>();
        
        ToolStripMenuItem contextCopy;
        ToolStripMenuItem contextWordWrap;
        ToolStripMenuItem showHideColumnsMenuItem;
        ToolStripMenuItem helpMenuItem;
        TrafodionIGridToolStripMenuItem showRowDetailsMenuItem;
        TrafodionIGridToolStripMenuItem showGridLinesMenuItem;
       
        public delegate void ShowRowDetailsDelegate(int row);
        private ShowRowDetailsDelegate showRowDetails;

        public delegate void DoubleClickDelegate(int row);
        private DoubleClickDelegate doubleClickDelegate;

        public delegate void ShowHideColumnChanged(object sender, EventArgs eArgs);
        public event ShowHideColumnChanged OnShowHideColumnChanged;

        public GridConfig Config
        {
            get;
            private set;
        }

        private bool HasConfig
        {
            get
            {
                return this.Config != null 
                    && this.Config.ID != null 
                    && this.Config.ID.Trim().Length > 0;
            }
        }

        /// <summary>
        /// Flag to avoid re-write config when filling/sorting/clearing data
        /// </summary>
        private bool IsSupressingRecordConfig
        {
            get;
            set;
        }

        private enum ExportType
        {
            Html,
            Text
        }

        [Browsable(false)]
        /// <summary>
        /// Save file dialog used by all instances of TrafodionDataGridView for duration of client session
        /// </summary>
        private SaveFileDialog theSaveFileDialog = null;

        /// <summary>
        /// Deliminated char for CSV file. Can be comma, TAB "\t" or just one-SPACE
        /// </summary>
        private string _deliminatedCharForCSVFile = ",";

        /// <summary>
        /// pattern match for multi-line TAB chars  etc. in a grid-cell
        /// </summary>
        private string _multiLinePatternToMatch = @"\r|\n|\t|\r\n";

        /// <summary>
        /// used for update the counter label
        /// </summary>
        private Control theCountControl;
        private Control theButtonControl;

        private TrafodionIGridFilter _currentFilter = null;

        private bool _theAllowWordWrap = false;
        private bool _theWordWrap = false;

        [NonSerialized]
        List<string> _alwaysHiddenColumnNames = null;

        [NonSerialized]
        private string _theHelpTopic = "";

        [NonSerialized]
        private bool _theAllowColumnFilter = true;
        
        /// <summary>
        /// The help topic associated with the button
        /// </summary>
        public string HelpTopic
        {
            get { return _theHelpTopic; }
            set { _theHelpTopic = value; }
        }

        /// <summary>
        /// By default, always allow columns to be filtered (which is done via the Show/Hight Columns context menu)
        /// However, in some cases, the filtering is not desirable, especially, when grouping is used. 
        /// </summary>
        public bool AllowColumnFilter
        {
            get { return _theAllowColumnFilter; }
            set { _theAllowColumnFilter = value; }
        }

        /// <summary>
        /// Wordwrap is a very expensive UI operation of IGrid if the number of rows are
        /// large (>500). So in UIs where more than 500 rows are displayed this should be
        /// turned off by default
        /// </summary>
        public bool AllowWordWrap
        {
            get { return _theAllowWordWrap; }
            set { _theAllowWordWrap = value; }
        }

        public List<string> AlwaysHiddenColumnNames
        {
            get 
            { 
                return _alwaysHiddenColumnNames != null? _alwaysHiddenColumnNames : new List<string>(); 
            }
            set 
            { 
                _alwaysHiddenColumnNames = value;
                if (_alwaysHiddenColumnNames != null && Cols.Count > 0)
                {
                    foreach (string colName in _alwaysHiddenColumnNames)
                    {
                        if (this.Cols[colName] != null)
                        {
                            this.Cols[colName].Visible = false;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// The delegate to display the row details. The default behavior is to display the 
        /// row details in a popup dialog
        /// </summary>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public ShowRowDetailsDelegate ShowRowDetails
        {
            get { return showRowDetails; }
            set { showRowDetails = value; }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public DoubleClickDelegate DoubleClickHandler
        {
            get { return doubleClickDelegate; }
            set { doubleClickDelegate = value; }
        }
        
        /// <summary>
        /// Sets the wordwrap property for the TrafodionIGrid
        /// </summary>
        public bool WordWrap
        {
            get { return _theWordWrap; }
            set 
            {
                if (_theWordWrap != value)
                {
                    _theWordWrap = value;
                    if (_theAllowWordWrap)
                    {
                        if (_theWordWrap)
                        {
                            DefaultCol.CellStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
                            DefaultCol.ColHdrStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
                        }
                        else
                        {
                            DefaultCol.CellStyle.TextFormatFlags = iGStringFormatFlags.None;
                            DefaultCol.ColHdrStyle.TextFormatFlags = iGStringFormatFlags.None;
                        }

                        if (Rows != null)
                        {
                            for (int i = 0; i < Cols.Count; i++)
                            {
                                iGCol col = Cols[i];
                                if (_theWordWrap)
                                {
                                    col.CellStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
                                    col.ColHdrStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
                                }
                                else
                                {
                                    col.CellStyle.TextFormatFlags = iGStringFormatFlags.None;
                                    col.ColHdrStyle.TextFormatFlags = iGStringFormatFlags.None;
                                }
                            }
                        }
                        SetAutoRowHeight();
                    }
                }                
            }
        }

        public List<int> SelectedRowIndexes
        {
            get
            {
                List<int> selectedRowIndexes = new List<int>();
                if (RowMode)
                {
                    foreach (iGRow row in SelectedRows)
                    {
                        selectedRowIndexes.Add(row.Index);
                    }
                }
                else
                {
                    foreach (iGCell cell in SelectedCells)
                    {
                        if (selectedRowIndexes.Contains(cell.RowIndex))
                            continue;

                        selectedRowIndexes.Add(cell.RowIndex);
                    }
                }
                return selectedRowIndexes;
            }
        }

        public TrafodionIGridFilter CurrentFilter
        {
            get { return _currentFilter; }
            set { _currentFilter = value; }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public TrafodionIGrid()
            : base()
        {
            Initialize();
            showRowDetails = new ShowRowDetailsDelegate(this.ShowRowDetailsImpl);
            doubleClickDelegate = new DoubleClickDelegate(this.DoubleClickImpl);
            _alwaysHiddenColumnNames = new List<string>();
        }

        public TrafodionIGrid(string licenseString)
            : base()
        {
            Initialize();
            showRowDetails = new ShowRowDetailsDelegate(this.ShowRowDetailsImpl);
            doubleClickDelegate = new DoubleClickDelegate(this.DoubleClickImpl);
            _alwaysHiddenColumnNames = new List<string>();
        }

        public GridConfig CreateGridConfig(string configId)
        {
            if (configId == null || configId.Trim().Length == 0)
            {
                throw new ArgumentNullException("configId", "'configId' cannot be null or empty!");
            }

            if (this.Config != null)
            {
                throw new Exception(string.Format("CreateGridConfig() can only be called once!"));
            }

            CreateConfig(configId);
            AttachOrDetachConfigEvent(true);
            return this.Config;
        }

        private void CreateConfig(string configId)
        {
            List<GridConfig> gridConfigs = Persistence.Get(GRID_CONFIGS_PERSISTENCE_KEY) as List<GridConfig>;

            if (gridConfigs == null)
            {
                gridConfigs = new List<GridConfig>();
                Persistence.Put(GRID_CONFIGS_PERSISTENCE_KEY, gridConfigs);

                this.Config = new GridConfig(configId);
                gridConfigs.Add(this.Config);
            }
            else
            {
                foreach (GridConfig config in gridConfigs)
                {
                    if (0 == string.Compare(config.ID, configId, true))
                    {
                        this.Config = config;
                        break;
                    }
                }

                if (!this.HasConfig)
                {
                    this.Config = new GridConfig(configId);
                    gridConfigs.Add(this.Config);
                }
            }
        }

        private void RecordToGridConfig()
        {
            if (!this.HasConfig) return;

            RecordColumnToGridConfig();
            RecordColumnMappingToGridConfig();
            RecordSortToGridConfig();
        }

        private void RecordColumnToGridConfig()
        {
            if (!this.HasConfig) return;

            // Record CurrentVisibleColumns
            this.Config.CurrentVisibleColumns.Clear();
            for (int i = 0; i < this.Cols.Count; i++)
            {
                iGCol igc = this.Cols.FromOrder(i);
                if (igc.Visible)
                {
                    this.Config.CurrentVisibleColumns.Add(igc.Key);
                }
            }
        }
        
        /// <summary>
        /// It indicates first-time use, if DefaultColumnToGridConfig is not set yet.
        /// So fill it with all the grid columns
        /// </summary>
        private void RecordInitialColumnToGridConfig()
        {
            if (!this.HasConfig) return;

            bool isVisibleColumnsNotSet = this.Config.CurrentVisibleColumns.Count == 0;
            if (this.Config.DefaultVisibleColumns.Count == 0 && this.Cols.Count > 0)
            {
                foreach (iGCol gridColumn in this.Cols)
                {
                    // Have all columns as visible columns only when developers haven't set current visible columns
                    if ( isVisibleColumnsNotSet ) 
                    {
                        this.Config.CurrentVisibleColumns.Add(gridColumn.Key);
                    }

                    this.Config.DefaultVisibleColumns.Add(gridColumn.Key);
                }
            }
        }
        
        private void RecordSortToGridConfig()
        {
            if (!this.HasConfig) return;

            // Record ColumnSorts
            this.Config.ColumnSorts.Clear();
            for (int i = 0; i < this.SortObject.Count; i++)
            {
                iGSortItem sort = this.SortObject[i];
                this.Config.ColumnSorts.Add(new ColumnSort(sort.ColIndex, sort.Index, (int)sort.SortOrder));
            }
        }

        private void RecordColumnMappingToGridConfig()
        {
            if (!this.HasConfig) return;

            // Record ColumnMappingInfos
            this.Config.ColumnMappingInfos.Clear();
            foreach (iGCol gridColumn in this.Cols)
            {
                if (gridColumn.Visible)
                {
                    ColumnMappingInfo cm = new ColumnMappingInfo();
                    cm.InternalName = gridColumn.Key;
                    cm.ExternalName = gridColumn.Text as string;
                    cm.ColumnWidth = gridColumn.Width;

                    this.Config.ColumnMappingInfos.Add(cm);
                }
            }
        }

        public void FillWithDataConfig(DataTable dataTable)
        {
            FillWithDataConfig(null, dataTable);
        }

        public void FillWithDataConfig(MethodInvoker methodFillData)
        {
            FillWithDataConfig(methodFillData, null);
        }
        
        private void FillWithDataConfig(MethodInvoker methodFillData, DataTable dataTable)
        {
            if (!this.HasConfig)
            {
                throw new InvalidOperationException("Cannot call FillWithDataConfig(), because Conig is not set yet!");
            }

            try
            {
                this.IsSupressingRecordConfig = true;
                this.BeginUpdate();

                bool isFirstTimeConfig = this.Config.DefaultVisibleColumns.Count == 0;

                if (methodFillData != null)
                {
                    TrafodionIGridUtils.PopulateGrid(this.Config, methodFillData, this);
                }
                else
                {
                    TrafodionIGridUtils.PopulateGrid(this.Config, dataTable, this);
                }

                if (isFirstTimeConfig)
                {
                    /* 
                     * Initialize default visibile columns if it's first-time use.
                     * This step should done: after data is filled into grid, before applying grid column setting
                     */
                    RecordInitialColumnToGridConfig();
                    this.AutoResizeCols = true; // Make it fill the grid for first-use
                    this.Cols.AutoWidth();
                    this.AutoResizeCols = false; // Make the last cols resizable and also avoid overwriting the persisted columns width
                }

                TrafodionIGridUtils.ApplyGridSetting(this);

                if (isFirstTimeConfig)
                {
                    RecordColumnToGridConfig();
                    RecordColumnMappingToGridConfig();
                }
            }
            finally
            {
                this.EndUpdate();
                this.IsSupressingRecordConfig = false;
            }

            CorrectColumnWidth();
        }

        /// <summary>
        /// Avoid grid's inproper column width issue: 
        /// You can see the columns checked in the filter, 
        /// but they cannot be seen in the grid, 
        /// because their widths are set to 0 by grid's internal logic
        /// </summary>
        private void CorrectColumnWidth()
        {
            foreach (iGCol gridColumn in this.Cols)
            {
                if (gridColumn.Visible && gridColumn.Width == 0)
                {
                    gridColumn.Width = ADJUSTED_VISBILE_WIDTH;
                }
            }
        }


        void PersistenceCofigSaving(Dictionary<string, object> dictionary, Persistence.PersistenceOperation persistenceOperation)
        {
            if (persistenceOperation == Persistence.PersistenceOperation.Save)
            {
                RecordToGridConfig();
            }
        }

        void GridColumnChanged(object sender, EventArgs args)
        {
            if (this.IsSupressingRecordConfig) return;

            RecordColumnToGridConfig();
        }

        private void GridContentSorted(object sender, EventArgs args)
        {
            if (this.IsSupressingRecordConfig) return;

            RecordSortToGridConfig();
        }

        private void GridColumnDragged(object sender, iGColHdrEndDragEventArgs e)
        {
            if (this.IsSupressingRecordConfig) return;

            if (!this.HasConfig) return;

            RecordColumnToGridConfig();

            // Have to manually calculate the column order, due to the strange logic of event Grid.ColHdrEndDrag
            string draggedColumn = this.Cols[e.ColIndex].Key;
            TrafodionIGridUtils.MoveConfigColumnOrder(this.Cols, this.Config.CurrentVisibleColumns, draggedColumn, e.NewOrder);

            if (this.CurrentFilter != null && !this.CurrentFilter.Visible)
            {
                this.CurrentFilter.CurrentVisibleColumns = this.Config.CurrentVisibleColumns;
            }
        }

        private void GridColomnWidthChanged(object sender, iGColWidthEventArgs e)
        {
            if (this.IsSupressingRecordConfig) return;

            // Change one column's width my impact others'. So record all columns' width info again.
            RecordColumnMappingToGridConfig();
        }

        private void AttachOrDetachConfigEvent(bool isAttach)
        {
            if (!this.HasConfig) return;

            if (isAttach)
            {
                Persistence.PersistenceHandlers += PersistenceCofigSaving;
                this.OnShowHideColumnChanged += GridColumnChanged;
                this.ColHdrEndDrag += GridColumnDragged;
                this.ColWidthEndChange += GridColomnWidthChanged;
                this.AfterContentsSorted += GridContentSorted;
            }
            else
            {
                Persistence.PersistenceHandlers -= PersistenceCofigSaving;
                this.OnShowHideColumnChanged -= GridColumnChanged;
                this.ColHdrEndDrag -= GridColumnDragged;
                this.ColWidthEndChange -= GridColomnWidthChanged;
                this.AfterContentsSorted -= GridContentSorted;
            }
        }

        protected override void Dispose(bool disposing)
        {
            AttachOrDetachConfigEvent(false);

            RecordToGridConfig();

            if (theCountControl != null)
            {
                theCountControl.Dispose();
            }
            if (theButtonControl != null)
            {
                theButtonControl.Dispose();
            }
            if (_currentFilter != null)
            {
                _currentFilter.Dispose();
            }
            base.Dispose(disposing);
        }

        void Initialize()
        {
            ReadOnly = true;
            BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this.RowMode = true;
            this.ColHdrEndDrag += new iGColHdrEndDragEventHandler(TrafodionIGrid_ColHdrEndDrag);
            this.CellMouseUp += new TenTec.Windows.iGridLib.iGCellMouseUpEventHandler(this._theDataGrid_CellMouseUp);
            this.SearchAsType.Mode = iGSearchAsTypeMode.Seek;
            this.SearchAsType.MatchRule = iGMatchRule.Contains;
            this.CellDoubleClick += new iGCellDoubleClickEventHandler(TrafodionIGrid_CellDoubleClick);
            this.ColHdrMouseUp += new iGColHdrMouseUpEventHandler(TrafodionIGrid_ColHdrMouseUp);
            Font = new Font("Tahoma", 8.25F, FontStyle.Regular);

            //By deafault word wrap is off
            DefaultCol.CellStyle.TextFormatFlags = iGStringFormatFlags.None;
            DefaultCol.ColHdrStyle.TextFormatFlags = iGStringFormatFlags.None;
            DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);

            contextCopy = new ToolStripMenuItem();
            contextCopy.Text = Properties.Resources.ToolstripMenuCopy;
            contextCopy.Click += new EventHandler(contextCopy_click);

            showRowDetailsMenuItem = new TrafodionIGridToolStripMenuItem();
            showRowDetailsMenuItem.Text = "Row Details...";
            showRowDetailsMenuItem.Click += new EventHandler(showRowDetailsMenuItem_click);

            showGridLinesMenuItem = new TrafodionIGridToolStripMenuItem();
            showGridLinesMenuItem.Text = "Show Grid Lines";
            showGridLinesMenuItem.Click += new EventHandler(showGridLinesMenuItem_click);

            showHideColumnsMenuItem = new ToolStripMenuItem();
            showHideColumnsMenuItem.Text = Properties.Resources.ShowHideGridColumns;
            showHideColumnsMenuItem.Click += new EventHandler(showHideColumnsMenuItem_click);

            helpMenuItem = new ToolStripMenuItem();
            helpMenuItem.Text = Properties.Resources.Help;
            helpMenuItem.Click += new EventHandler(helpMenuItem_Click);

            contextWordWrap = new System.Windows.Forms.ToolStripMenuItem();
            contextWordWrap.Name = "contextWordWrap";
            contextWordWrap.Text = global::Trafodion.Manager.Properties.Resources.MenuWordWrap;
            contextWordWrap.CheckOnClick = true;
            contextWordWrap.Checked = WordWrap;
            contextWordWrap.Visible = true;
            contextWordWrap.Click += new System.EventHandler(this.wrapOnOff_Click);

            this.Resize += new EventHandler(_theDataGrid_Resize);
            this.ColWidthChanging += new iGColWidthEventHandler(_theDataGrid_ColWidthChanging);

            SetInitialRowHeight();
        }

        void TrafodionIGrid_ColHdrEndDrag(object sender, iGColHdrEndDragEventArgs e)
        {
            if (_currentFilter != null)
            {
                _currentFilter.MoveVisibleColumn(this.Cols[e.ColIndex].Key, e.NewOrder);
            }
        }

        void TrafodionIGrid_CellDoubleClick(object sender, iGCellDoubleClickEventArgs e)
        {
            if (e.RowIndex < 0)
            {
                return; //Header clicked.
            }

            if (CurRow != null)
            {
                if (doubleClickDelegate != null)
                {
                    doubleClickDelegate(CurRow.Index);
                }
            }
            else
            {
                if (CurCell != null)
                {
                    if (doubleClickDelegate != null)
                    {
                        doubleClickDelegate(CurCell.RowIndex);
                    }
                }
            }
        }

        /// <summary>
        /// The rowIndex where the rows will be inserted 
        /// </summary>
        /// <param name="rowIndex"></param>
        /// <param name="datatable"></param>
        public void InsertRows(int rowBefore, DataTable datatable)
        {
            if ((datatable != null) && (datatable.Rows.Count > 0))
            {
                int igridSize = this.Rows.Count;
                this.Rows.InsertRange(rowBefore,datatable.Rows.Count );
                for (int i = 0; i < datatable.Rows.Count; i++)
                {
                    iGRow row = this.Rows[rowBefore + i];
                    object[] values = datatable.Rows[i].ItemArray;

                    for (int j = 0; j < values.Length; j++)
                    {
                        row.Cells[j].Value = values[j];
                        this.CommitEditCurCell();
                    }                        
                }
            }
        }

        public void InsertRows(int index, DataTable datatable, int startRow, int endRow)
        {
            lock (this)
            {
                try
                {
                    BeginUpdate();
                    if ((datatable != null) && (datatable.Rows.Count > 0) && startRow >= 0 && endRow > startRow)
                    {
                        int igridSize = this.Rows.Count;
                        int copyLength = endRow - startRow;
                        this.Rows.InsertRange(index, copyLength);
                        for (int i = 0; i < copyLength; i++)
                        {
                            iGRow row = this.Rows[index + i];
                            object[] values = datatable.Rows[i + startRow].ItemArray;

                            for (int j = 0; j < values.Length; j++)
                            {
                                try
                                {
                                    row.Cells[j].Value = values[j];
                                }
                                catch (Exception ex)
                                {
                                    Console.WriteLine(ex.Message);
                                }
                                this.CommitEditCurCell();
                            }
                        }
                    }
                }
                finally
                {
                    EndUpdate();
                }
            }
        }
        private void SetInitialRowHeight()
        {
            String testColName = "_123_AutoSizeTestcol_XYZ_";
            int rowHeight = DefaultRow.Height;

            try
            {
                iGCol col = Cols.Add(testColName, testColName);
                iGRow row = Rows.Add();

                row.Cells[testColName].Value = "Auto Resizing Grid ... all lowercase ... ALL UPPERCASE ... ";
                row.AutoHeight();
                rowHeight = row.Height;
            }
            catch (Exception e)
            {
            }
            finally
            {
                try
                {
                    Cols.RemoveAt(testColName);
                }
                catch (Exception e)
                {
                }
                Rows.Clear();
            }

            DefaultRow.Height = (int)(1.15 * rowHeight);
            DefaultRow.NormalCellHeight = (int)(1.15 * rowHeight);
        }

        /// <summary>
        /// Enables or disables the display of ContextMenu on this tree view
        /// </summary>
        virtual public bool AllowContextMenu
        {
            get { return true; }
        }

        /// <summary>
        /// This method allows the users to add custom context menus to the TrafodionIgrid control
        /// </summary>
        /// <param name="aMenuItem"></param>
        public void AddContextMenu(TrafodionIGridToolStripMenuItem aMenuItem)
        {
            this._theMenuItems.Add(aMenuItem);
        }
        /// <summary>
        /// This method allows the users to add separators to the context menus of the TrafodionIgrid control
        /// </summary>
        /// <param name="aMenuItem"></param>
        public void AddToolStripSeparator(ToolStripSeparator aSeparator)
        {
            this._theMenuItems.Add(aSeparator);
        }

        /// <summary>
        /// Sets the autoheight for rows and columns. this is needed to resize the height and column 
        /// if the word wrap is set to true
        /// </summary>
        public void SetAutoRowHeight()
        {
            if (_theAllowWordWrap)
            {
                if (Rows != null)
                {
                    Rows.AutoHeight();
                }

                if (Header != null)
                {
                    Header.AutoHeight();
                }
            }
        }

        public void AutoRowHeight()
        {
            BeginUpdate();
            foreach (iGRow row in Rows)
            {
                row.AutoHeight();
            }
            EndUpdate();
        }

        void TrafodionIGrid_ColHdrMouseUp(object sender, iGColHdrMouseUpEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                if (Cols.Count > 0)
                {
                    TrafodionContextMenuStrip contextMenuStrip = new TrafodionContextMenuStrip();
                    if (AllowColumnFilter)
                    {
                        contextMenuStrip.Items.Add(showHideColumnsMenuItem);
                    }
                    if (!String.IsNullOrEmpty(HelpTopic))
                    {
                        contextMenuStrip.Items.Add(helpMenuItem);
                    }
                    if (contextMenuStrip.Items.Count > 0)
                    {
                        contextMenuStrip.Show(this, e.MousePos);
                    }
                }
            }
        }

       private void _theDataGrid_CellMouseUp(object sender, iGCellMouseUpEventArgs e)
       {
           if (e.Button == MouseButtons.Right)
           {              
               if (this.RowMode)
               {
                   this.Rows[e.RowIndex].Selected = true;
               }
               else
               {
                   this.Cells[e.RowIndex, e.ColIndex].Selected = true;
               }
               ShowContextMenu(e);
           }
       }
  
       private void ShowContextMenu(iGCellMouseUpEventArgs e)
        {
           TrafodionContextMenuStrip contextMenuStrip = new TrafodionContextMenuStrip();

           TrafodionIGridEventObject eventObject = new TrafodionIGridEventObject();
           eventObject.TheGrid = this;
           eventObject.Row = e.RowIndex;
           eventObject.Col = e.ColIndex;
           eventObject.PointOfClick = e.MousePos;

            if ((_theMenuItems != null) && (_theMenuItems.Count > 0))
            {
                foreach (ToolStripItem menuitem in this._theMenuItems)
                {
                    if (menuitem is TrafodionIGridToolStripMenuItem)
                    {
                        ((TrafodionIGridToolStripMenuItem)menuitem).TrafodionIGridEventObject = eventObject;
                    }
                    contextMenuStrip.Items.Add(menuitem);
                }
                contextMenuStrip.Items.Add(new ToolStripSeparator());
            }

           //Add the copy menu item
            contextCopy.Enabled = (this.SelectedRowIndexes.Count > 0);
            contextMenuStrip.Items.Add(contextCopy);
            contextMenuStrip.Items.Add(new ToolStripSeparator());
            showGridLinesMenuItem.Text = (this.GridLines.Mode == iGGridLinesMode.None) ? "Show Grid Lines" : "Hide Grid Lines";

            showRowDetailsMenuItem.TrafodionIGridEventObject = eventObject;
            contextMenuStrip.Items.Add(showRowDetailsMenuItem);
            contextMenuStrip.Items.Add(showGridLinesMenuItem);
            if (_theAllowWordWrap)
            {
                contextMenuStrip.Items.Add(contextWordWrap);
            }

            contextMenuStrip.Show(this, e.MousePos);
        }

       public void ApplyFilter(TrafodionIGridFilter aFilter)
       {
           TrafodionIGridUtils.applyGridFilter(this, aFilter);
       }

       public void ApplyFilter()
       {
           if (CurrentFilter != null)
           {
               ApplyFilter(CurrentFilter);
           }
           HideColumnsMarkedToBeHidden();
       }

       public void HideColumnsMarkedToBeHidden()
       {
           if (_alwaysHiddenColumnNames != null)
           {
               foreach (string colName in _alwaysHiddenColumnNames)
               {
                   Cols[colName].Visible = false;
               }
           }
       }

       /// <summary>
       /// The help menu item event handler.
       /// </summary>
       /// <param name="sender"></param>
       /// <param name="e"></param>
       private void helpMenuItem_Click(object sender, EventArgs e)
       {
           if (!String.IsNullOrEmpty(this.HelpTopic))
           {
               TrafodionHelpProvider.Instance.ShowHelpTopic(this.HelpTopic);
           }
       }

       private void showHideColumnsMenuItem_click(object sender, EventArgs e)
        {
            if (Cols.Count > 0)
            {
                if (_currentFilter == null)
                {
                    _currentFilter = TrafodionIGridUtils.createAGridFilter(this, Properties.Resources.ShowHideGridColumns);
                }

                _currentFilter.Grid = this;
                TrafodionIGridFilter clonedFilter = _currentFilter.Clone(); //The clone method needs to be implemented..
                if (clonedFilter.ShowDialog() == DialogResult.OK)
                {
                    _currentFilter = clonedFilter;
                    ApplyFilter();
                    FireOnShowHideColumnChanged(new EventArgs());
                }

            }
        }

        private void ShowRowDetailsImpl(int row)
        {
            TrafodionIGridRowDisplay rowDisplay = new TrafodionIGridRowDisplay();
            rowDisplay.ShowRowDetails(this, row);
            WindowsManager.PutInWindow(new Size(800, 600), rowDisplay, "Row Details", TrafodionContext.Instance.CurrentConnectionDefinition);
        }

        private void DoubleClickImpl(int row)
        {
            ShowRowDetailsImpl(row);
        }

        private void wrapOnOff_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem contextWordWrap = sender as ToolStripMenuItem;
            if (contextWordWrap != null)
            {
                WordWrap = contextWordWrap.Checked;
            }
        }

        private void _theDataGrid_ColWidthChanging(object sender, EventArgs e)
        {
            SetAutoRowHeight();
        }

        private void _theDataGrid_Resize(object sender, EventArgs e)
        {
            SetAutoRowHeight();
        }

        private void showRowDetailsMenuItem_click(object sender, EventArgs e)
        {
            TrafodionIGridToolStripMenuItem menuItem = sender as TrafodionIGridToolStripMenuItem;
            showRowDetails(menuItem.TrafodionIGridEventObject.Row);
        }

        private void showGridLinesMenuItem_click(object sender, EventArgs e)
        {
            if (this.GridLines.Mode == iGGridLinesMode.None)
            {
                this.GridLines.Mode = iGGridLinesMode.Both;
            }
            else
            {
                this.GridLines.Mode = iGGridLinesMode.None;
            }
        }

        private void contextCopy_click(object sender, EventArgs e)
        {
            copyToClipboard();
        }

        /// <summary>
        /// Fire the state change event
        /// </summary>
        /// <param name="e"></param>
        private void FireOnShowHideColumnChanged(EventArgs e)
        {
            if (OnShowHideColumnChanged != null)
            {
                OnShowHideColumnChanged(this, e);
            }
        }

          /// <summary>
        /// Initialize the File-dialog with appropriate dir  and file type
        /// </summary>
        private void FileDialogInit()
        {

            // Create the save file as dialog
            theSaveFileDialog = new SaveFileDialog();

            // start it  off with the same directory as it ended with the last time in the same session or previous.
            theSaveFileDialog.InitialDirectory = Utilities.FileDialogLocation();

            // We default to CSV Excel-appropriate format
            theSaveFileDialog.DefaultExt = "CSV";

            // Set the file dialog filter to the kinds of files we support
            theSaveFileDialog.Filter =
                  "Comma Separated Values Format file (*.CSV)|*.CSV"
                + "|HyperText Markup Language file (*.HTML)|*.HTML";

            // Future: 
            //  + "|Excel Spreadsheet file (*.XLS)|*.XLS";
            //+ "|Portable Document Format file (*.PDF)|*.PDF"

        }


        /// <summary>
        /// Export the grid to a file
        /// </summary>
        public void ExportToFile()
        {
            // create a Save-File-Dialog and position in appropriate dir
            FileDialogInit();

            // Present the dilaog to the user
            if (theSaveFileDialog.ShowDialog() == DialogResult.OK)
            {

                // Make comparisons easier
                string theUpperFileName = theSaveFileDialog.FileName.ToUpper();

                // Persist the last file-location in this or across user sessions
                Utilities.FileDialogLocation(theUpperFileName);

                 // Check to see what extension the filename ends with and pass in the chosen filename
                if (theUpperFileName.EndsWith(".CSV"))
                {

                    // CSV
                    ExportToCSVFile(theSaveFileDialog.FileName);

                }
                else if (theUpperFileName.EndsWith(".HTML"))
                {

                    // HTML
                    ExportToHtmlFile(theSaveFileDialog.FileName);
 
                }
                else
                {
                    // Other: File type must be CSV or HTML
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.AllowedExportFileTypes, Properties.Resources.Error, MessageBoxButtons.OK); 
                }
            }
        }

        /// <summary>
        /// Export the data grid to an Excel application's spreadsheet
        /// Saves data in html format and then invokes Excel.
        /// </summary>
        public void ExportToSpreadsheet()
        {
            // Use system's feature to create/manage a unique temp file name
            string tempFileName = System.IO.Path.GetTempFileName();

            // add extension .xls to it for system to start the default spreadsheet-EXCEL app
            tempFileName += ".xls";

            // 1. Save the data to an HTML file -suitable for Excel import
            ExportToHtmlFile(tempFileName);

            // 2. Launch Excel application on the above html file
            StartExcelSpreadsheetForHTMLFile(tempFileName);

            // 3. Leave the Deletion of this file to the user.

        }

        /// <summary>
        /// Export the full data from the grid to the system's Clipboard to enable paste
        /// e.g. to a word document.
        /// </summary>
        public void ExportToClipboard()
        {
            // call the internal helper method
            CopyDataToClipBoard();
        }


        /// <summary>
        /// Export the data grid to the browser- Internet explorer
        /// Saves data in html format and then invokes I.E.
        /// </summary>
        public void ExportToBrowser()
        {
            // Use system's feature to create/manage a unique temp file name
            string tempFileName = System.IO.Path.GetTempFileName();

            // add extension .html ot it for system to start the default browser.
            tempFileName += ".html";

            // 1. Save the data to a HTML file - suitable for IE import
            ExportToHtmlFile(tempFileName);

            // 2. Launch Internet Explorer Browser on the above html file
            StartBrowserForHTMLFile(tempFileName);

        }

        /// <summary>
        /// To export the Grid as a DataTable. 
        /// </summary>
        /// <param name="table"></param>
        /// <returns></returns>
        public DataTable ExportToDataTable(DataTable table)
        {
            if (table.Columns.Count == 0 && Cols.Count > 0)
            {
                foreach (iGCol col in Cols)
                {
                    DataColumn dc = new DataColumn(col.Text as string, typeof(object));
                    table.Columns.Add(dc);
                }
            }

            foreach (iGRow row in Rows)
            {
                DataRow dr = table.NewRow();
                for (int i = 0; i < Cols.Count; i++)
                {
                    dr[i] = row.Cells[i].Value;
                }
                table.Rows.Add(dr);
            }

            return table;
        }

        /// <summary>
        /// Save this data grid to a Comma Separated Values (.CSV) file for Excel import
        /// </summary>
        /// <param name="fileName">The filename from the save file dialog</param>
        public void ExportToCSVFile(string fileName)
        {
            // Open a file to write data from the grid
            StreamWriter theCSVStreamWriter = null;
            try
            {
                theCSVStreamWriter = File.CreateText(fileName);

                // Write column-headers
                WriteColumnHeaderToCSVFile(theCSVStreamWriter);


                // Write ROWS: all  data from the rows
                WriteDataRowsToCSVFile(theCSVStreamWriter);
            }
            catch (Exception e)
            {
                //  "Error in  opening or writing to file: " + fileName + "  Exception: " + e.Message);
                string message = string.Format(Properties.Resources.ErrorOpeningWritingToFile, fileName, e.Message);
                MessageBox.Show(Utilities.GetForegroundControl(), message, Properties.Resources.Error, MessageBoxButtons.OK); 
            }
            finally
            {
                if (theCSVStreamWriter != null)
                {
                    try
                    {
                        theCSVStreamWriter.Close();
                    }
                    finally
                    {
                    }
                }
            } // end-try block
        }


        /// <summary>
        /// Write the Grid's column headers to the CSV file
        /// </summary>
        /// <param name="csvStreamWriter"></param>
        private void WriteColumnHeaderToCSVFile(StreamWriter csvStreamWriter)
        {
            int    columnIndex = 0;
            string columnText  = "";
            string rowText     = "";

            //  navigate thru column-headers, re-format the cells and write to the file
            foreach (TenTec.Windows.iGridLib.iGCol column in this.Cols)
            {
                columnText = "\"" + column.Text + "\"";
                rowText += columnText;
                // Don't need separator/deliminator for last cell
                if (++columnIndex < this.Cols.Count)
                {
                    rowText += _deliminatedCharForCSVFile; // deliminator
                }
            }
            //Console.WriteLine(rowText);
            csvStreamWriter.WriteLine(rowText);
        }




        /// <summary>
        /// helper method to Write each row in CSV format to the file
        /// </summary>
        /// <param name="csvStreamWriter"></param>
        private void WriteDataRowsToCSVFile(StreamWriter csvStreamWriter)
        {
            string rowText    = "";
            string columnText = "";

            // navigate thru row-by-row, re-format the cells and write to the file
            foreach (TenTec.Windows.iGridLib.iGRow row in this.Rows)
            {
                rowText = "";

                // navigate to each columns and make one-line per row to write to file
                for (int columnIndex = 0; columnIndex < this.Cols.Count; columnIndex++)
                {
                    TenTec.Windows.iGridLib.iGCell cell = row.Cells[columnIndex];
                    string cellValue = "";
                    if (cell.Value != null)
                    {
                        //For DateTime use the long format so we preserve the microseconds
                        if (cell.Value is DateTime)
                            cellValue = Utilities.GetTrafodionSQLLongDateTime((DateTime)cell.Value, false);
                        else
                            cellValue = cell.Value.ToString();
                    }
                    if (cellValue.StartsWith("\0"))
                        cellValue = "";

                    // Comments/Strings may span multiple lines
                    cellValue = ReplaceMultilineChars(cellValue);

                    // if cell has double-quotes as part of name e.g. "<FooBar>" is the name
                    // Put quotes around the value
                    columnText = "\"" + cellValue.Replace("\"", "\"\"")  + "\"";
                    rowText += columnText;
                    // Don't need separator/deliminator for last cell
                    if (columnIndex < row.Cells.Count-1 )
                    {
                        rowText += _deliminatedCharForCSVFile; // deliminator
                    }
                }
                csvStreamWriter.WriteLine(rowText);
            } // end-foreach on rows

        }

        /// <summary>
        /// Replace all newline, TAB chars from the input string with a space
        /// </summary>
        /// <param name="aFromString"></param>
        /// <returns></returns>
        private string ReplaceMultilineChars(string aFromString)
        {
            // Using  regular expressions 
            // this could be an in-line statement for efficiency. 
            // A separate method for readability and keep it modular to add new scenarios/chars
            return Regex.Replace(aFromString, _multiLinePatternToMatch, " ");
        }

        /// <summary>
        /// Save the this data grid to an HTML file
        /// </summary>
        /// <param name="fileName">The filename from the save file dialog</param>
        public void ExportToHtmlFile(string fileName)
        {
            // Open a file to write data from the grid
            StreamWriter theHTMLStreamWriter = null;
            try
            {
                theHTMLStreamWriter = File.CreateText(fileName);

                // 1: html -BEGIN  table tag
                string rowText = "<table border=\"1\">";
                theHTMLStreamWriter.WriteLine(rowText);

                // 2: Write Grid-Header
                WriteColumnHeader(theHTMLStreamWriter, ExportType.Html);
                
                // 3: Write column-data for each  Row 
                WriteDataRows(theHTMLStreamWriter, ExportType.Html);

                // 4: html -END   table tag
                rowText = "</table>";
                theHTMLStreamWriter.WriteLine(rowText);
            }
            catch (Exception e)
            {
                //  "Error in  opening or writing to file: " + fileName + "  Exception: " + e.Message);
                string message = string.Format(Properties.Resources.ErrorOpeningWritingToFile, fileName, e.Message);
                MessageBox.Show(Utilities.GetForegroundControl(), message, Properties.Resources.Error, MessageBoxButtons.OK);
            }
            finally
            {
                if (theHTMLStreamWriter != null)
                {
                    try
                    {
                        theHTMLStreamWriter.Close();
                    }
                    finally
                    {
                    }
                }
            } // end-try block
        } // ExportToHtmlFile

        /// <summary>
        /// internal  helper method to - Write Grid-Header to HTML file
        /// </summary>
        /// <param name="textWriter"></param>
        /// <param name="exportType"></param>
        private void WriteColumnHeader(TextWriter textWriter, ExportType exportType)
        {

            if (exportType == ExportType.Html)
            {
                textWriter.WriteLine("<tr>");
            }

            // 2: Write column-headers
            // navigate thru column-headers, re-format the cells and write to the file
            foreach (TenTec.Windows.iGridLib.iGCol column in this.Cols)
            {
                string headerText = TrafodionIGridUtils.ConvertBreakToBlank(column.Text.ToString());
                if (exportType == ExportType.Html)
                {
                    textWriter.WriteLine("<th>" + headerText + "</th>");
                }
                else
                {
                    textWriter.Write("\"" + headerText + "\" ");
                }
            }

            // 3: end of - Header row
            if (exportType == ExportType.Html)
            {
                textWriter.WriteLine("</tr>");
            }
            else
            {
                textWriter.WriteLine();
            }
        }


        /// <summary>
        /// internal helper method to Write each cell for each  Row 
        /// </summary>
        /// <param name="textWriter"></param>
        /// <param name="exportType"></param>
        private void WriteDataRows(TextWriter textWriter, ExportType exportType)
        {
            // 1: Write column-data for each  Row 
            // <td valign=\"top\">  column_value </td>
            // navigate thru row-by-row, the cells and write to the file
            foreach (TenTec.Windows.iGridLib.iGRow row in this.Rows)
            {
                string rowText = "";

                // 1 : begin of - DATA Row 
                if (exportType == ExportType.Html)
                {
                    textWriter.WriteLine("<tr>");
                }

                // navigate to each columns and make one-line per row to write to file
                for (int columnIndex = 0; columnIndex < this.Cols.Count; columnIndex++)
                {
                    // 2 : begin of - DATA Row 
                    TenTec.Windows.iGridLib.iGCell cell = row.Cells[columnIndex];

                    string valueString = "";
                    if (cell.Value != null)
                    {
                        //For DateTime use the long format so we preserve the microseconds
                        if (cell.Value is DateTime)
                            valueString = Utilities.GetTrafodionSQLLongDateTime((DateTime)cell.Value, false);
                        else
                            valueString = cell.Value.ToString();
                    }

                    if (valueString.StartsWith("\0"))
                        valueString = "";
                    if (exportType == ExportType.Html)
                    {
                        if (cell.Value is long)
                        {
                            textWriter.WriteLine("<td valign=\"top\" style='mso-number-format:\"@\";'>" + FormatToHTMLString(valueString) + "</td>");
                            /*
                              mso-number-format:"0"       No Decimals 
                              mso-number-format:"0\.00"     2 Decimals 
                              mso-number-format:"mm\/dd\/yy"    Date format
                              mso-number-format:"m\/d\/yy\ h\:mm\ AM\/PM" D -T AMPM 
                              mso-number-format:"Short Date"    05/06/-2008  
                              mso-number-format:"Medium Date"   05-jan-2008 
                              mso-number-format:"Short Time"    8:67
                              mso-number-format:"Medium Time"   8:67 am 
                              mso-number-format:"Long Time"     8:67:25:00 
                              mso-number-format:"Percent"     Percent - two decimals
                              mso-number-format:"0\.E+00"     Scientific Notation 
                              mso-number-format:"\@"      Text 
                              mso-number-format:"\#\ ???\/???"    Fractions - up to 3 digits (312/943) 
                              mso-number-format:"\0022$\0022\#\,\#\#0\.00"                  Currency $12.76 
                              mso-number-format:"\#\,\#\#0\.00_ \;\[Red\]\-\#\,\#\#0\.00\ "             2 decimals, negative numbers in red and signed (1.86  -1.66)
                              mso-number-format:¡±\\#\\,\\#\\#0\\.00_\\)\\;\\[Black\\]\\\\(\\#\\,\\#\\#0\\.00\\\\)¡±    Accounting Format ¨C5,(5)
                            */
                        }
                        else
                        {
                            textWriter.WriteLine("<td valign=\"top\">" + FormatToHTMLString(valueString) + "</td>");
                        }
                    }
                    else
                    {
                        textWriter.Write("\"" + FormatToHTMLString(valueString) + "\" ");
                    }
                }

                // 3: end of -  DATA row
                if (exportType == ExportType.Html)
                {
                    textWriter.WriteLine("</tr>");
                }
                else
                {
                    textWriter.WriteLine();
                }
            } // end-foreach on rows

        }

        /// <summary>
        /// Covert the given value from the grid-cell to HTML appropriate value
        /// </summary>
        /// <param name="aCellValue"></param>
        /// <returns>HTML string for the given string</returns>
        // e.g. <lame> tp &lt;lame&gt;  to make it HTML readable
        private string FormatToHTMLString(string aCellValue)
        {
            string htmlString = "";
            for (int index = 0; index < aCellValue.Length; index++)
            {
                char theChar = aCellValue[index];

                switch (theChar)
                {
                    case '<':
                    {
                        htmlString += "&lt;";
                        break;
                    }
                    case '>':
                    {
                        htmlString += "&gt;";
                        break;
                    }
                    default:
                    {
                        htmlString += theChar;
                        break;
                    }
                }
            } // for

            //Console.WriteLine("htmlString: "  + htmlString);
            return htmlString;
        }


        /// <summary>
        /// Invoke Browser e.g. default Internet Explorer with the HTML file 
        /// </summary>
        /// <param name="aHTMLFileName"></param>
        private void StartBrowserForHTMLFile(string aHTMLFileName)
        {
            // this method will be invoked from a button e.g. "Data to Browser..."  e.g. 
            // Invoke Internet Explorer by default
            // If in future: Someone may want other browser, we can query system default browser from registry.
            //
            System.Diagnostics.Process ieProcess = new System.Diagnostics.Process();
            ieProcess.EnableRaisingEvents = false;
            ieProcess.StartInfo.FileName = aHTMLFileName; 
            ieProcess.Start();

        }

        /// <summary>
        /// Start Excel-Spreadsheet displaying the data from the grid that was saved in .html format
        /// </summary>
        /// <param name="aHTMLFileName"></param>
        private void StartExcelSpreadsheetForHTMLFile(string aHTMLFileName)
        {
            // this method will be invoked from a button e.g. "Data to Spreadsheet..."

            System.Diagnostics.Process excelProcess = new System.Diagnostics.Process();
            excelProcess.EnableRaisingEvents = false;
            excelProcess.StartInfo.FileName = aHTMLFileName;

            excelProcess.Start();

        }


        /// <summary>
        /// Copy the data from the datagridvoew to the system's  Clipboard.
        /// </summary>
        /// Future: This method may be invoked  via standard CTRL+C, if DGV handles the keyboard
        private void CopyDataToClipBoard()
        {
            try
            {
                StringWriter theStringWriter = new StringWriter();
                try
                {
                    WriteColumnHeader(theStringWriter, ExportType.Text);
                    WriteDataRows(theStringWriter, ExportType.Text);
                }
                catch (Exception e)
                {
                    //  "Error in  opening or writing to file: " + fileName + "  Exception: " + e.Message);
                    string message = string.Format(Properties.Resources.ErrorOpeningWritingToFile, "", e.Message);
                    MessageBox.Show(Utilities.GetForegroundControl(), message, Properties.Resources.Error, MessageBoxButtons.OK);
                }
                finally
                {
                    if (theStringWriter != null)
                    {
                        try
                        {
                            theStringWriter.Close();
                        }
                        finally
                        {
                        }
                    }
                } // end-try block

                Clipboard.SetDataObject(theStringWriter.ToString());
            }
            catch (System.Runtime.InteropServices.ExternalException)
            {
                // Exception: The Clipboard could not be accessed. 
                MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ClipboardExceptionMessage, Properties.Resources.Error, MessageBoxButtons.OK); 
            }
        }

        /// <summary>
        /// Processes the key commands associated with copy and select
        /// </summary>
        /// <param name="msg"></param>
        /// <param name="keydata"></param>
        /// <returns></returns>
        protected override bool ProcessCmdKey(ref Message msg, Keys keydata)
        {
            switch (msg.Msg)
            {
                case 0x100:
                case 0x104:
                    switch (keydata)
                    {
                        case Keys.Control | Keys.C:
                        case Keys.Control | Keys.Shift | Keys.C:
                            copyToClipboard();
                            break;

                        case Keys.Control | Keys.A:
                        case Keys.Control | Keys.Shift | Keys.A:
                            selectAllForClipboard();
                            break;

                        default:
                            break;
                    }
                    break;
            }

            return base.ProcessCmdKey(ref msg, keydata);
        }



        private void selectAllForClipboard()
        {
            if (RowMode)
            {
                PerformAction(iGActions.SelectAllRows);
            }
            else
            {
                PerformAction(iGActions.SelectAllCells);
            }
        }



        private void copyToClipboard()
        {
            TrafodionIGridUtils.copyIGridContentsToClipboard(this, true);
        }

        /// <summary>
        /// Clear all rows and columns from the grid
        /// </summary>
        public void Clear()
        {
            Cols.Clear();
            Rows.Clear();
        }

        /// <summary>
        /// Call to get a control panel for the grid.  The control
        /// is wired to the grid and requires nothing from the caller other than a place to "mount" it.
        /// </summary>
        /// <returns>The control</returns>
        public Control GetButtonControl()
        {
            return new TrafodionIGridButtonsUserControl(this);
        }

        /// <summary>
        /// Creates a panel with a count of the number of entries in the grid and docks it in the parent.
        /// </summary>
        /// <param name="aFormat">A text format string</param>
        /// <param name="aDockStyle">Where to dock the panel</param>
        /// <returns>True if the panel was created and docked</returns>
        public bool AddCountControlToParent(string aFormat, DockStyle aDockStyle)
        {
            try
            {
                TrafodionPanel thePanel = new TrafodionPanel();

                thePanel.Height = 20;

                theCountControl = GetCountControl(aFormat);
                theCountControl.Dock = DockStyle.Fill;
                thePanel.Controls.Add(theCountControl);

                thePanel.Dock = aDockStyle;
                Parent.Controls.Add(thePanel);

                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// Update the existing CountControl Panel with a new format string.
        /// </summary>
        /// <param name="aFormat">A text format string</param>
        /// <returns>True if the text updated</returns>
        public bool UpdateCountControlText(string aFormat)
        {
            try
            {
                ((TrafodionIGridCountUserControl)theCountControl).TheFormat = aFormat;
                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        
        /// <summary>
        /// Creates a control panel for the grid and docks it in the parent.
        /// </summary>
        /// <param name="aDockStyle">Where to dock the panel</param>
        /// <returns>True if the panel was created and docked</returns>
        public bool AddButtonControlToParent(DockStyle aDockStyle)
        {
            try
            {
                TrafodionPanel thePanel = new TrafodionPanel();

                thePanel.Height = 40;

                theButtonControl = GetButtonControl();
                theButtonControl.Dock = aDockStyle;
                thePanel.Controls.Add(theButtonControl);

                thePanel.Dock = aDockStyle;
                Parent.Controls.Add(thePanel);

                return true;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// Called when the "Export" button on the control panel is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void theExportToFileButton_Click(object sender, EventArgs e)
        {
            ExportToFile();
        }

        /// <summary>
        /// Creates a panel with a count of the number of entries in the grid.
        /// </summary>
        /// <param name="aFormat">A text format string</param>
        /// <returns>The control</returns>
        public Control GetCountControl(string aFormat)
        {
            return new TrafodionIGridCountUserControl(this, aFormat);
        }



        public void ResizeGridColumns(System.Data.DataTable dt, int minWidth, int maxWidth)
        {
            int maxDefaultAutoSizeColumnWidth = maxWidth;
            int minDefaultAutoSizeColumnWidth = minWidth;

            int nRows = dt.Rows.Count;
            int nCols = dt.Columns.Count;

            int[] widths = new int[dt.Columns.Count];
            int idx;

            try
            {
                for (idx = 0; idx < nCols; idx++)
                {
                    widths[idx] = dt.Columns[idx].ColumnName.Length;
                    if (maxDefaultAutoSizeColumnWidth < widths[idx])
                        widths[idx] = maxDefaultAutoSizeColumnWidth;
                }

                for (int rowIndex = 0; rowIndex < nRows; rowIndex++)
                {
                    for (idx = 0; idx < dt.Columns.Count; idx++)
                    {
                        int vLen = dt.Rows[rowIndex][idx].ToString().Trim().Length;
                        widths[idx] = Math.Max(widths[idx], vLen);
                    }
                }

                int pixelsPerChar = Math.Max(9, (int)Math.Ceiling(this.Font.SizeInPoints));

                for (idx = 0; idx < nCols; idx++)
                {
                    int width = Math.Min(maxDefaultAutoSizeColumnWidth, widths[idx]);
                    width = Math.Max(minDefaultAutoSizeColumnWidth, width);
                    this.Cols[idx].Width = (int)(width * pixelsPerChar);
                }
            }
            catch (Exception)
            {
            }
        }

        public void ResizeGridColumns(System.Data.DataTable dt)
        {
            ResizeGridColumns(dt, 7, 45);
        }

        public iGRow AddRow(object[] rowValues)
        {
            iGRow row = Rows.Add();
            for (int i = 0; i < rowValues.Length; i++)
            {
                //row.Cells[i].Value = rowValues[i].ToString();
                row.Cells[i].Value = rowValues[i];
            }
            return row;
        }
    }
}
