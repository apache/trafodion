//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionPropertyGridUserControl : UserControl
    {
        #region Fields

        private DataTable _theDataTable = null;
        private Color _theCellOrigColor;
        private string _theTitle = null;
        private List<string> _theAvailPropertyList = new List<string>();
        private List<string> _theSelectedPropertyList = new List<string>();
        private string _theAvailListTitle = Trafodion.Manager.Properties.Resources.AvailableValues;
        private string _theSelectedListTitle = Trafodion.Manager.Properties.Resources.CurrentlyUsedValues;
        private bool _theCurrentFilterState = false;

        /// <summary>
        /// The datatable schema
        /// </summary>
        public const string DATATABLE_COL_NAME = "Name";
        public const string DATATABLE_COL_VALUE = "Value";
        public const string DATATABLE_COL_FORMAT = "Format";

        public delegate void PropertyFilterChanged(object sender, PropertyFilterChangedEventArgs e);
        public event PropertyFilterChanged OnFilterChanged;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Title of the property grid
        /// </summary>
        public string Title
        {
            get { return _theTitle; }
            set { _theTitle = value; }
        }

        /// <summary>
        /// Property: the available property list 
        /// </summary>
        public List<string> AvailPropertyList
        {
            get { return _theAvailPropertyList; }
            set { _theAvailPropertyList = value; }
        }

        /// <summary>
        /// Property: the currently selected property list
        /// </summary>
        public  List<string> SelectedPropertyList
        {
            get { return _theSelectedPropertyList; }
            set { _theSelectedPropertyList = value; }
        }

        /// <summary>
        /// Property: the available list title for the filter dialog
        /// </summary>
        public string AvailListTitle
        {
            get { return _theAvailListTitle; }
            set { _theAvailListTitle = value; }
        }

        /// <summary>
        /// Property: the selected list title for the filter dialog
        /// </summary>
        public string SelectedListTitle
        {
            get { return _theSelectedListTitle; }
            set { _theSelectedListTitle = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        public TrafodionPropertyGridUserControl(string aTitle, List<string> anAvailPropertyList, List<string> aSelectedPropertyList)
        {
            InitializeComponent();
            _theGrid.SelectionMode = iGSelectionMode.MultiExtended;
            _theGrid.Cols[0].CellStyle.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            _theCellOrigColor = this._theGridCol1CellStyle.ForeColor;
            _theTitle = aTitle;
            AvailPropertyList = anAvailPropertyList;
            SelectedPropertyList = aSelectedPropertyList;
            SetFilterAppliedLabelState(AvailPropertyList, SelectedPropertyList);
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// To load initial data to the metric
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <param name="aForeColor"></param>
        public void LoadInitialData(DataTable aDataTable, Color aForeColor)
        {
            _theDataTable = aDataTable;
            _theGrid.BeginUpdate();
            _theGrid.FillWithData(aDataTable, true);
            _theGrid.ResizeGridColumns(aDataTable, 7, 12);

            foreach (iGRow row in _theGrid.Rows)
            {
                row.Cells[DATATABLE_COL_VALUE].ForeColor = aForeColor;
                if (!SelectedPropertyList.Contains(row.Cells[DATATABLE_COL_NAME].Value as string))
                {
                    row.Visible = false;
                }

                // Now, format it.
                string format = aDataTable.Rows[row.Index][DATATABLE_COL_FORMAT] as string;
                if (!string.IsNullOrEmpty(format))
                {
                    row.Cells[DATATABLE_COL_VALUE].Style.FormatString = format;
                }
            }
            _theGrid.EndUpdate();
        }

        /// <summary>
        /// To load new data to the metric
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <param name="aUnChangedColor"></param>
        /// <param name="aChangedColor"></param>
        public void LoadNewData(DataTable aDataTable, Color aUnChangedColor, Color aChangedColor)
        {
            if (_theGrid.Rows.Count > 0)
            {
                // To see if the values have being changed.
                Hashtable ht = new Hashtable();

                if (aUnChangedColor != aChangedColor)
                {
                    // Do it only if the user wants to show change highlight 
                    for (int i = 0; i < _theDataTable.Rows.Count; i++)
                    {
                        Type type = _theDataTable.Rows[i][DATATABLE_COL_VALUE].GetType();
                        string value1, value2;
                        if (type != typeof(string))
                        {
                            value1 = _theDataTable.Rows[i][DATATABLE_COL_VALUE].ToString();
                            value2 = aDataTable.Rows[i][DATATABLE_COL_VALUE].ToString();
                        }
                        else
                        {
                            value1 = _theDataTable.Rows[i][DATATABLE_COL_VALUE] as string;
                            value2 = aDataTable.Rows[i][DATATABLE_COL_VALUE] as string;
                        }

                        if (!value1.Equals(value2))
                        {
                            ht.Add(_theDataTable.Rows[i][DATATABLE_COL_NAME] as string, value2);
                        }
                    }
                }

                // Start updating the metric
                _theGrid.BeginUpdate();
                _theGrid.StaySorted = true;
                _theGrid.FillWithData(aDataTable, true);

                foreach (iGRow row in _theGrid.Rows)
                {
                    string name = row.Cells[DATATABLE_COL_NAME].Value as string;
                    if (SelectedPropertyList.Contains(name))
                    {
                        if (ht.Contains(name))
                        {
                            row.Cells[DATATABLE_COL_VALUE].ForeColor = aChangedColor;
                        }
                        else
                        {
                            row.Cells[DATATABLE_COL_VALUE].ForeColor = aUnChangedColor;
                        }
                    }
                    else
                    {
                        row.Visible = false;
                    }

                    // Now, format it.
                    string format = aDataTable.Rows[row.Index][DATATABLE_COL_FORMAT] as string;
                    if (!string.IsNullOrEmpty(format))
                    {
                        if (row.Cells[DATATABLE_COL_VALUE].Style != null)
                        {
                            row.Cells[DATATABLE_COL_VALUE].Style.FormatString = format;
                        }
                        else
                        {
                            iGCellStyle style = new iGCellStyle();
                            style.FormatString = format;
                            row.Cells[DATATABLE_COL_VALUE].Style = style;
                        }
                    }
                }

                _theGrid.EndUpdate();
                _theGrid.ResizeGridColumns(aDataTable, 7, 12);
            }
        }

        /// <summary>
        /// Reset the metric 
        /// </summary>
        public void ResetData()
        {
            _theGrid.BeginUpdate();
            _theGrid.Rows.Clear();
            _theGrid.EndUpdate();
        }
        /// <summary>
        ///  Set "Filter Applied Label Visible or Invisible"
        /// </summary>
        /// <param name="argAvailPropertyList"></param>
        /// <param name="argSelectedPropertyList"></param>
        public void SetFilterAppliedLabelState(List<string> argAvailPropertyList, List<string> argSelectedPropertyList) 
        {
            _theCurrentFilterState = (AvailPropertyList.Count != SelectedPropertyList.Count);
            _theFilterAppliedLabel.Visible = _theCurrentFilterState;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Fire the filter change event
        /// </summary>
        /// <param name="e"></param>
        private void FireOnFilterChanged(PropertyFilterChangedEventArgs e)
        {
            if (OnFilterChanged != null)
            {
                OnFilterChanged(this, e);
            }
        }

        private void _theFilterButton_Click(object sender, EventArgs e)
        {
            TrafodionFilterSelection fs = new TrafodionFilterSelection(_theTitle, AvailPropertyList.ToArray(), SelectedPropertyList.ToArray());
            fs.AvailableListTitle = AvailListTitle;
            fs.SelectedListTitle = SelectedListTitle;
            DialogResult result = fs.ShowDialog();
            if (result == DialogResult.OK)
            {
                string[] selected = fs.SelectedList;
                SelectedPropertyList = (selected == null) ? new List<string>() : new List<string>(selected);
            }

            _theGrid.BeginUpdate();

            foreach (iGRow row in _theGrid.Rows)
            {
                string name = row.Cells[DATATABLE_COL_NAME].Value as string;
                if (SelectedPropertyList.Contains(name))
                {
                    row.Visible = true;
                }
                else
                {
                    row.Visible = false;
                }
            }

            _theGrid.EndUpdate();

            // Fire up events if the filtering state changed.
            if (_theCurrentFilterState)
            {
                // Filter changed from ON to OFF
                if (SelectedPropertyList.Count == AvailPropertyList.Count)
                {
                    _theCurrentFilterState = false;
                    FireOnFilterChanged(new PropertyFilterChangedEventArgs(Title, _theCurrentFilterState, true));
                }
            }
            else
            {
                // Filter changed from OFF to ON
                if (SelectedPropertyList.Count != AvailPropertyList.Count)
                {
                    _theCurrentFilterState = true;
                    FireOnFilterChanged(new PropertyFilterChangedEventArgs(Title, _theCurrentFilterState, false));
                }
            }

            // Turn on the applied label only if the filtering is on. 
            _theFilterAppliedLabel.Visible = _theCurrentFilterState;
        }

        #endregion Private methods
    }

    #region Metrics Filter Changed Event

    /// <summary>
    /// Class for metrics filter change event
    /// </summary>
    public class PropertyFilterChangedEventArgs : EventArgs
    {
        private string _title;
        private bool _currentFilterState;
        private bool _previousFilterState;

        /// <summary>
        /// Property: Metrics
        /// </summary>
        public string Metrics
        {
            get { return _title; }
            set { _title = value; }
        }

        /// <summary>
        /// Property: Current Filter State
        /// </summary>
        public bool CurrentFilterState
        {
            get { return _currentFilterState; }
            set { _currentFilterState = value; }
        }

        /// <summary>
        /// Property: Previous Filter State
        /// </summary>
        public bool PreviousFilterState
        {
            get { return _previousFilterState; }
            set { _previousFilterState = value; }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="originalState"></param>
        /// <param name="currentState"></param>
        public PropertyFilterChangedEventArgs(string title, bool currentFilterState, bool previousFilterState)
        {
            _title = title;
            _currentFilterState = currentFilterState;
            _previousFilterState = previousFilterState;
        }

    #endregion Metrics Filter Changed Event
    }
}
