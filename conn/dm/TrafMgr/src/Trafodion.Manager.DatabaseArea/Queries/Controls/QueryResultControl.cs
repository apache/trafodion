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
using System.Data;
using System.Data.Odbc;
using System.Windows.Forms;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /**************************************************************************
     * Given a DataSet, displays the results in a DataGridView 
     **************************************************************************/
    public partial class QueryResultControl : UserControl, IQueryResultControl
    {
        private String theSqlStatement;
        private bool isPageMode = false;
        private int pageNumber;
        private long lastRowNumber = 0;

        /// <summary>
        /// Indicates if the result control is in a pagination mode.
        /// </summary>
        public bool IsPageMode
        {
            get { return isPageMode; }
            set { isPageMode = value; }
        }

        /// <summary>
        /// The current page number that is being displayed.
        /// </summary>
        public int PageNumber
        {
            get { return pageNumber; }
            set { pageNumber = value; }
        }

        /// <summary>
        /// The last row number that has been displayed
        /// </summary>
        public long LastRowNumber
        {
            get { return lastRowNumber; }
            set { lastRowNumber = value; }
        }

        public Trafodion.Manager.Framework.Controls.TrafodionIGrid TheGrid
        {
            get { return _theTrafodionIGrid;}
            set { _theTrafodionIGrid = value; }
        }

        public QueryResultControl()
        {
            InitializeComponent();
            InitializeIGrid();
        }

        void InitializeIGrid()
        {
            _theTrafodionIGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid();
            Controls.Clear();
            Controls.Add(_theTrafodionIGrid);
            _theTrafodionIGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            _theTrafodionIGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            _theTrafodionIGrid.ReadOnly = true;
            _theTrafodionIGrid.TreeCol = null;
            _theTrafodionIGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            _theTrafodionIGrid.AddCountControlToParent("", DockStyle.Top);
            _theTrafodionIGrid.AddButtonControlToParent(DockStyle.Bottom);
            _theTrafodionIGrid.AutoResizeCols = false;
            _theTrafodionIGrid.RowMode = false;
            _theTrafodionIGrid.SelectionMode = iGSelectionMode.MultiExtended;
        }

        void DisposeIGrid()
        {
            if (_theTrafodionIGrid != null)
            {
                _theTrafodionIGrid.Rows.Clear();
                _theTrafodionIGrid.Dispose();
            }
        }

        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                DisposeIGrid();
            }
        }

        /// <summary>
        /// Formats the date, timestamp and time columns for display
        /// </summary>
        public void SetDateFormat(DataTable aDataTable)
        {
            foreach (iGCol col in _theTrafodionIGrid.Cols)
            {
                if (col.CellStyle != null)
                {
                    if (col.CellStyle.ValueType == typeof(System.DateTime))
                    {
                        col.CellStyle.FormatString = "{0}";
                        int scale = -1;
                        string sqlType = "";

                        if (aDataTable != null && aDataTable.Columns != null && aDataTable.Columns.Count == _theTrafodionIGrid.Cols.Count)
                        {
                            if (aDataTable.Columns[col.Index].ExtendedProperties.ContainsKey("SCALE"))
                            {
                                scale = Int32.Parse(aDataTable.Columns[col.Index].ExtendedProperties["SCALE"].ToString());
                            }
                            if(aDataTable.Columns[col.Index].ExtendedProperties.ContainsKey("SQL_TYPE"))
                            {
                                sqlType = aDataTable.Columns[col.Index].ExtendedProperties["SQL_TYPE"].ToString();
                            }
                        }
                        col.CellStyle.FormatProvider = new TrafodionDateTimeFormatProvider(scale, sqlType);
                    }
                    else
                        if (col.CellStyle != null && col.CellStyle.ValueType == typeof(System.TimeSpan))
                        {
                            col.CellStyle.FormatString = "{0}";
                            col.CellStyle.FormatProvider = new TrafodionTimeFormatProvider(-1);
                        }
                }
            }

        }

        public void LoadTable(OdbcCommand anOdbcCommand, string aSqlStatement, int aMaxRows)
        {
            theSqlStatement = aSqlStatement;
            _theTrafodionIGrid.BeginUpdate();
            _theTrafodionIGrid.FillWithData(anOdbcCommand);
            _theTrafodionIGrid.UpdateCountControlText(Properties.Resources.QueryReturnedRows);
            SetDateFormat(new DataTable());
            _theTrafodionIGrid.EndUpdate();
        }

        public void LoadTable(DataTable aTable)
        {
            DisposeIGrid();
            InitializeIGrid();
            _theTrafodionIGrid.BeginUpdate();
            _theTrafodionIGrid.FillWithData(aTable);
            _theTrafodionIGrid.UpdateCountControlText(Properties.Resources.QueryReturnedRows);
            SetDateFormat(aTable);
            _theTrafodionIGrid.ResizeGridColumns(aTable);
            _theTrafodionIGrid.EndUpdate();
            lastRowNumber += _theTrafodionIGrid.Rows.Count;
        }

        public void Select(OdbcCommand anOdbcCommand, string aSqlStatement, int aMaxRows)
        {
            LoadTable(anOdbcCommand, aSqlStatement, aMaxRows);
        }

        public void Explain(OdbcCommand anOdbcCommand, string aSqlStatement, int aMaxRows)
        {
            LoadTable(anOdbcCommand, aSqlStatement, aMaxRows);
        }

        #region IQueryResultControl Members

        public string getSqlStatement()
        {
            return theSqlStatement;
        }

        #endregion
    }

    /// <summary>
    /// Custom formatter for date and timestamp columns
    /// </summary>
    public class TrafodionDateTimeFormatProvider : IFormatProvider, ICustomFormatter
    {
        static public string DateTimeFormatString = "yyyy-MM-dd HH':'mm':'ss";
        static public string DateFormatString = "yyyy-MM-dd";
        int _scale = -1;
        string _sqlType = "";

        public TrafodionDateTimeFormatProvider(int scale, string sqlType)
        {
            _scale = scale;
            _sqlType = sqlType;
        }

        public object GetFormat(Type formatType)
        {
            if (formatType == typeof(ICustomFormatter))
                return this;
            else
                return null;
        }

        public string Format(string fmt, object arg, IFormatProvider formatProvider)
        {
            // Convert argument to a string.
            if (arg is DateTime)
            {
                DateTime dateTime = (DateTime)arg;
                if (!string.IsNullOrEmpty(_sqlType) && _sqlType.Equals("DATE"))
                {
                    return dateTime.ToString(DateFormatString);
                }
                string fractionalString = "";
                if (_scale < 0)
                {
                    string millisecondsString = ""; 
                    string timeSpanString = dateTime.TimeOfDay.ToString();
 
                    if (string.IsNullOrEmpty(timeSpanString))
                    {
                        timeSpanString = "";
                    }
                    else
                    {
                        if (timeSpanString.LastIndexOf('.') >= 0)
                        {
                            millisecondsString = timeSpanString.Substring(timeSpanString.LastIndexOf('.') + 1);
                        }
                        else
                        {
                            millisecondsString = "";
                        }
                    }
                    fractionalString = GetFractionalString(millisecondsString);
                }
                else
                {
                    fractionalString = GetFractionalString(_scale);
                }
                string dateTimeString = dateTime.ToString(DateTimeFormatString + fractionalString);
                return dateTimeString;
            }
            else
            {
                return arg.ToString();
            }
        }

        public static string GetFractionalString(string value)
        {
            string fractionString = "";
            bool nonZeroFound = false;
            for (int i = value.Length - 1; i >= 0; i--)
            {
                short x = 0;
                if (char.IsDigit(value[i]))
                {
                    if (nonZeroFound)
                    {
                        fractionString += "f";
                    }
                    else
                    {
                        if (value[i] != '0')
                        {
                            nonZeroFound = true;
                            fractionString = ".f";
                        }
                    }
                }
                else
                {
                    return "";
                }
            }
            return fractionString;
        }

        public static string GetFractionalString(int length)
        {
            string fractionString = "";
            if (length > 0)
            {
                fractionString = ".";
                for (int i = 0; i < length; i++)
                {
                    fractionString += "f";
                }
            }
            return fractionString;
        }
    }

    /// <summary>
    /// Custom format provider for time columns
    /// </summary>
    public class TrafodionTimeFormatProvider : IFormatProvider, ICustomFormatter
    {
        static public string TimeFormatString = "HH':'mm':'ss";
        int _scale = -1;

        public TrafodionTimeFormatProvider(int scale)
        {
            _scale = scale;
        }

        public object GetFormat(Type formatType)
        {
            if (formatType == typeof(ICustomFormatter))
                return this;
            else
                return null;
        }

        public string Format(string fmt, object arg, IFormatProvider formatProvider)
        {
            // Convert argument to a string.
            if (arg is TimeSpan)
            {
                TimeSpan timeSpan = (TimeSpan)arg;
                string timeSpanString = timeSpan.ToString(); 
                DateTime dateTime = new DateTime(timeSpan.Ticks);

                string fractionalFormatString = "";
                string millisecondsString = "";

                if (string.IsNullOrEmpty(timeSpanString))
                {
                    timeSpanString = "";
                }
                else
                {
                    if (timeSpanString.LastIndexOf('.') >= 0)
                    {
                        millisecondsString = timeSpanString.Substring(timeSpanString.LastIndexOf('.') + 1);
                    }
                    else
                    {
                        millisecondsString = "";
                    }
                }
                if (_scale < 0)
                {
                    if (timeSpan.TotalSeconds <=0 )
                    {
                        return dateTime.ToString(TimeFormatString);
                    }
                    fractionalFormatString = TrafodionDateTimeFormatProvider.GetFractionalString(millisecondsString);
                }
                else
                {
                    fractionalFormatString = TrafodionDateTimeFormatProvider.GetFractionalString(_scale);
                }
                return dateTime.ToString(TimeFormatString + fractionalFormatString);
            }
            else
            {
                return arg.ToString();
            }
        }
    }
}
