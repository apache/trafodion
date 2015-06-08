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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Datagridview to display the histogram statistics for a sql table
    /// </summary>
    public class TableHistogramStatsDataGridView : TrafodionDataGridView
    {
        #region Fields

        private TrafodionTable _sqlMxTable;

        #endregion Fields

        /// <summary>
        /// Default constructor for this control.
        /// </summary>
        public TableHistogramStatsDataGridView()
        {
            InitializeComponent();

            //Add a listener for double click to pop up the second screen
            this.DoubleClick += new EventHandler(TableStatsDataGridView_DoubleClick);
        }

        /// <summary>
        /// Load the histogram statistics from the database and display in the datagrid
        /// </summary>
        private void Load()
        {
            Rows.Clear();
            Columns.Clear();

            //Add the columns
            Columns.Add(new ColumnNameLinkColumn("ColumnName", Properties.Resources.ColumnName));
            Columns.Add("Datatype", Properties.Resources.DataType);
            Columns.Add("NumberOfNulls", Properties.Resources.NumberOfNulls);
            Columns.Add("MinValue", Properties.Resources.MinValue);
            Columns.Add("MaxValue", Properties.Resources.MaxValue);
            Columns.Add("Skew", Properties.Resources.Skew);
            Columns.Add("UEC", Properties.Resources.UEC);
            Columns.Add("Cardinality", Properties.Resources.Cardinality);
            Columns.Add("LastStatsTimestamp", Properties.Resources.LastStatsTimestamp);
            //Set the display format for the cells
            Columns[2].DefaultCellStyle.Format = "N0"; //#nulls
            Columns[5].DefaultCellStyle.Format = "N2"; //skew
            Columns[6].DefaultCellStyle.Format = "N0"; //uec
            Columns[7].DefaultCellStyle.Format = "N0"; //cardinality
            
            //Load the histogram stats for all columns in table
            _sqlMxTable.LoadTableStatistics();

            //For each column in the table, fill the histogram stats into the datagrid
            foreach (TrafodionTableColumn sqlMxTableColumn in TrafodionTable.Columns)
            {
                TableColumnHistogramStats columnStatistics = sqlMxTableColumn.HistogramStatistics;
                if (columnStatistics != null)
                {
                    //Column name is a hyper link to launch the sampled statistics screen
                    ColumnNameLink columnLink = new ColumnNameLink(columnStatistics.ColumnName);
                    columnLink.TrafodionTableColumn = sqlMxTableColumn;

                    Rows.Add(new Object[]{
                        columnLink, columnStatistics.DataType,columnStatistics.NumberOfNulls, 
                        columnStatistics.MinValue, columnStatistics.MaxValue, columnStatistics.Skew,
                        columnStatistics.TotalUEC, columnStatistics.RowCount,
                        new Trafodion.Manager.Framework.JulianTimestamp(columnStatistics.StatsTime)
                    });
                }
            }
        }

        /// <summary>
        /// Handle double clicks on the datagrid. If a row is selected, show the details screen for the selected row
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TableStatsDataGridView_DoubleClick(object sender, EventArgs e)
        {
            if (CurrentRow != null)
            {
                ColumnNameLink.ShowDetails(CurrentRow);
            }
        }

        /// <summary>
        /// Designer generated code
        /// </summary>
        private void InitializeComponent()
        {
            this.ReadOnly = true;
            ((System.ComponentModel.ISupportInitialize)(this)).BeginInit();
            this.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this)).EndInit();
            this.ResumeLayout(false);

        }

        #region Properties

        /// <summary>
        /// The SQL table whose histogram statistics are being displayed
        /// </summary>
        public TrafodionTable TrafodionTable
        {
            get { return _sqlMxTable; }
            set
            {
                _sqlMxTable = value;
                if (_sqlMxTable != null)
                {
                    //If valid table reference, load the histogram stats from database
                    Load();
                }
                else
                {
                    Rows.Clear();
                }
            }
        }

        #endregion Properties
    }

    /// <summary>
    /// A DataGridTextBoxColumn that displays a HyperLink for a column name
    /// </summary>
    public class ColumnNameLinkColumn : DataGridViewTextBoxColumn
    {
        /// <summary>
        /// Constructs the column and sets it template to point to the hyperlink column
        /// </summary>
        /// <param name="aName"></param>
        public ColumnNameLinkColumn(string aName)
        {
            CellTemplate = new ColumnNameLink(aName);
        }

        /// <summary>
        /// Constructs the column using the name and header text
        /// </summary>
        /// <param name="aName"></param>
        /// <param name="aHeaderText"></param>
        public ColumnNameLinkColumn(string aName, string aHeaderText)
            : this(aName)
        {
            Name = aName;
            HeaderText = aHeaderText;
        }
    }
    
    /// <summary>
    /// A DataGridViewTextBoxCell that displays a hyperlink for the column name
    /// </summary>
    public class ColumnNameLink : DataGridViewTextBoxCell
    {
        #region Fields
        
        private string _name = null;
        private TrafodionTableColumn _sqlMxTableColumn = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// The SQL Table Column model associated with this link
        /// </summary>
        public TrafodionTableColumn TrafodionTableColumn
        {
            get { return _sqlMxTableColumn; }
            set { _sqlMxTableColumn = value; }
        }

        #endregion Properties

        /// <summary>
        /// Default constructor
        /// </summary>
        public ColumnNameLink()
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="name"></param>
        public ColumnNameLink(string name)
            :base()
        {
            _name = name;
        }

        /// <summary>
        /// Displays the column name
        /// </summary>
        /// <returns></returns>
        override public string ToString()
        {
            return Name;
        }

        /// <summary>
        /// Name of the sql table column
        /// </summary>
        public string Name
        {
            get { return _name; }
        }

        /// <summary>
        /// Launches the sampled statistics screen to display statistics by sampling the table content
        /// </summary>
        /// <param name="dataGridViewRow">The datagridview that contains the column hyperlinks</param>
        public static void ShowDetails(DataGridViewRow dataGridViewRow)
        {
            //Find the first column name link and launch the TableSampledStatsControl for it 
            //in a managed window
            foreach (DataGridViewCell dataGridViewCell in dataGridViewRow.Cells)
            {
                if (dataGridViewCell is ColumnNameLink)
                {
                    ColumnNameLink columnLink = dataGridViewCell.Value as ColumnNameLink;
                    try
                    {

                        //TableSampledStatsControl detailStats = new TableSampledStatsControl(columnLink.TrafodionTableColumn);
                        FloatingTableSampledStatsControl detailStats = new FloatingTableSampledStatsControl(columnLink.TrafodionTableColumn);
                        detailStats.Dock = DockStyle.Fill;

                        string title = String.Format(Properties.Resources.TableSampledStatisticsTitle, columnLink.Name, columnLink.TrafodionTableColumn.TrafodionTable.VisibleAnsiName);
                        WindowsManager.PutInWindow(new Size(800, 700), detailStats, title, true, columnLink.TrafodionTableColumn.TrafodionTable.ConnectionDefinition);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error,
                              MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    return;
                }
            }
        }

        /// <summary>
        /// Override the default Paint method for this control to display the hyperlinl
        /// </summary>
        /// <param name="aGraphics"></param>
        /// <param name="aClipBounds"></param>
        /// <param name="aCellBounds"></param>
        /// <param name="aRowIndex"></param>
        /// <param name="aCellState"></param>
        /// <param name="aValue"></param>
        /// <param name="aFormattedValue"></param>
        /// <param name="anErrorText"></param>
        /// <param name="aCellStyle"></param>
        /// <param name="anAdvancedBorderStyle"></param>
        /// <param name="aPaintParts"></param>
        protected override void Paint(
            Graphics aGraphics, Rectangle aClipBounds,
            Rectangle aCellBounds, int aRowIndex,
            DataGridViewElementStates aCellState,
            object aValue, object aFormattedValue,
            string anErrorText, DataGridViewCellStyle aCellStyle,
            DataGridViewAdvancedBorderStyle anAdvancedBorderStyle,
            DataGridViewPaintParts aPaintParts)
        {
            // Check to see if this is a type we know how to deal with
            if (!(aValue is ColumnNameLink))
            {
                // It's not ... just do standard paint as defined by base class
                base.Paint(aGraphics, aClipBounds, aCellBounds, aRowIndex, aCellState,
                    aValue, aFormattedValue, anErrorText, aCellStyle,
                    anAdvancedBorderStyle, aPaintParts);

                // And exit
                return;

            }

            DataGridViewCellStyle theCellStyle = new DataGridViewCellStyle(aCellStyle);
            theCellStyle.Font = new Font(aCellStyle.Font, aCellStyle.Font.Style | FontStyle.Underline);

            // Retrieve the client location of the mouse pointer.
            Point theCursorPosition = DataGridView.PointToClient(Cursor.Position);
            if (aCellBounds.Contains(theCursorPosition))
            {
                theCellStyle.ForeColor = Color.Blue;

                base.Paint(aGraphics, aClipBounds, aCellBounds, aRowIndex, aCellState,
                    aValue, aFormattedValue, anErrorText, theCellStyle,
                    anAdvancedBorderStyle, aPaintParts);
            }
            else
            {

                base.Paint(aGraphics, aClipBounds, aCellBounds, aRowIndex, aCellState,
                    aValue, aFormattedValue, anErrorText, theCellStyle,
                    anAdvancedBorderStyle, aPaintParts);
            }
        }

        /// <summary>
        /// Force the cell to repaint itself when the mouse pointer enters it.
        /// </summary>
        /// <param name="rowIndex"></param>
 
        protected override void OnMouseEnter(int rowIndex)
        {
            DataGridView.InvalidateCell(this);
        }

        /// <summary>
        /// Force the cell to repaint itself when the mouse pointer leaves it.
        /// </summary>
        /// <param name="rowIndex"></param>
 
        protected override void OnMouseLeave(int rowIndex)
        {
            DataGridView.InvalidateCell(this);
        }

        /// <summary>
        /// On double click, launch the sampled statistics screen, if any row is selected 
        /// in the datagridview
        /// </summary>
        /// <param name="e"></param>
        protected override void OnClick(DataGridViewCellEventArgs e)
        {
            // Can get here with no rows selected if the click is unselecting a row
            if (DataGridView.SelectedRows.Count == 0)
            {
                return;
            }
            ShowDetails(DataGridView.SelectedRows[0]);
        }
    }
}
