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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;
using TenTec.Windows.iGridLib;
using System.Collections;

namespace Trafodion.Manager.SecurityArea.Controls
{
    /// <summary>
    /// Directory parameters user control
    /// </summary>
    public partial class DirectoryServerParametersUserControl : UserControl
    {
        #region Fields

        private const string GRID_COLUMN_NAME = "Name";
        private const string GRID_COLUMN_VALUE = "Value";

        private TrafodionIGrid _parametersGrid;
        private DirectoryServer.CONFIG_PARAMETERS_TYPE _paramType = DirectoryServer.CONFIG_PARAMETERS_TYPE.LDAP_CONFIG;
        private DataTable _dataTable = null;
        private iGColPattern _parameterColPattern = null;

        /// <summary>
        /// Delegate for required fields changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        public delegate void RequiredFieldsChanged(object sender, EventArgs args);

        /// <summary>
        /// Event for required fields changed.
        /// </summary>
        public event RequiredFieldsChanged OnRequiredFieldsChanged;
        #endregion 

        #region Properties

        /// <summary>
        /// ParameterType: Is this for common or reqular parameters?
        /// </summary>
        public DirectoryServer.CONFIG_PARAMETERS_TYPE ParameterType
        {
            get { return _paramType; }
            set { _paramType = value; }
        }

        /// <summary>
        /// Parameters: the user entered parameters
        /// </summary>
        public ArrayList Parameters
        {
            get 
            {
                // Concatenate all rows to form a single string.
                ArrayList _newArrayList = new ArrayList();

                foreach (iGRow row in _parametersGrid.Rows)
                {
                    if (!String.IsNullOrEmpty(row.Cells[GRID_COLUMN_NAME].Value as string) &&
                        !String.IsNullOrEmpty(row.Cells[GRID_COLUMN_VALUE].Value as string))
                    {
                        _newArrayList.Add(new string[] { row.Cells[GRID_COLUMN_NAME].Value as string, row.Cells[GRID_COLUMN_VALUE].Value as string });
                    }
                }
                return _newArrayList; 
            }
            set 
            {
                ResetGrid(value);
            }
        }

        /// <summary>
        /// IsEmpty: return true if there is no parameter entered
        /// </summary>
        public bool IsEmpty
        {
            get
            {
                foreach (iGRow row in _parametersGrid.Rows)
                {
                    if (!String.IsNullOrEmpty(row.Cells[GRID_COLUMN_NAME].Value as string) &&
                        !String.IsNullOrEmpty(row.Cells[GRID_COLUMN_VALUE].Value as string))
                    {
                        return false;
                    }
                }
                return true; 
            }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The default constructor
        /// </summary>
        //public DirectoryServerParametersUserControl()
        //    : this(DirectoryServer.CONFIG_PARAMETERS_TYPE.LDAP_CONFIG)
        //{

        //}

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aServerType"></param>
        public DirectoryServerParametersUserControl(DirectoryServer.CONFIG_PARAMETERS_TYPE aParamType)
        {
            _paramType = aParamType;
            InitializeComponent();
            _parametersGrid = new TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0ATQB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAIABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            _parametersGrid.ReadOnly = false;
            _parametersGrid.Padding = new Padding(3);

            iGDropDownList parameterList = new iGDropDownList();

            switch (ParameterType)
            {
                case DirectoryServer.CONFIG_PARAMETERS_TYPE.AD_COMMON_CONFIG:
                    foreach (string name in Enum.GetNames(typeof(DirectoryServer.AD_COMMON_CONFIG_PARAMETERS)))
                    {
                        parameterList.Items.Add(name);
                    }
                    break;

                case DirectoryServer.CONFIG_PARAMETERS_TYPE.LDAP_COMMON_CONFIG:
                    foreach (string name in Enum.GetNames(typeof(DirectoryServer.LDAP_COMMON_CONFIG_PARAMETERS)))
                    {
                        parameterList.Items.Add(name);
                    }
                    break;

                case DirectoryServer.CONFIG_PARAMETERS_TYPE.AD_CONFIG:
                    foreach (string name in Enum.GetNames(typeof(DirectoryServer.AD_CONFIG_PARAMETERS)))
                    {
                        parameterList.Items.Add(name);
                    }
                    break;

                case DirectoryServer.CONFIG_PARAMETERS_TYPE.LDAP_CONFIG:
                    foreach (string name in Enum.GetNames(typeof(DirectoryServer.LDAP_CONFIG_PARAMETERS)))
                    {
                        parameterList.Items.Add(name);
                    }
                    break;

                default:
                    break;
            }

            iGCellStyle cellStyle = new iGCellStyle();
            cellStyle.DropDownControl = parameterList;
            cellStyle.Flags = iGCellFlags.DisplayText;
            cellStyle.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            cellStyle.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            cellStyle.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            cellStyle.Type = TenTec.Windows.iGridLib.iGCellType.Text;
            cellStyle.TypeFlags = TenTec.Windows.iGridLib.iGCellTypeFlags.NoTextEdit;

            _parameterColPattern = new iGColPattern();
            _parameterColPattern.CellStyle = cellStyle;
            _parameterColPattern.Text = GRID_COLUMN_NAME;
            _parameterColPattern.Key = GRID_COLUMN_NAME;
            //_parameterColPattern.MinWidth = 150;
            _parameterColPattern.Width = 150;
            _parameterColPattern.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);

            _dataTable = new DataTable();
            _dataTable.Columns.Add(GRID_COLUMN_NAME);
            _dataTable.Columns.Add(GRID_COLUMN_VALUE);
            _dataTable.Rows.Add(new Object[] { DirectoryServer.LDAP_COMMON_CONFIG_PARAMETERS.DirectoryBase.ToString(), "" });
            _parametersGrid.AutoResizeCols = true;
            _parametersGrid.AutoWidthColMode = iGAutoWidthColMode.HeaderAndCells;
            _parametersGrid.FillWithData(_dataTable);

            _parametersGrid.Cols[GRID_COLUMN_NAME].Pattern = _parameterColPattern;
            _parametersGrid.Dock = DockStyle.Fill;
            _parametersGrid.RowSelectionInCellMode = iGRowSelectionInCellModeTypes.None;
            _parametersGrid.SelectionMode = iGSelectionMode.One;

            MenuItem addMenu = new MenuItem("Add Row");
            addMenu.Click += new EventHandler(addMenu_Click);
            MenuItem deleteMenu = new MenuItem("Delete Row");
            deleteMenu.Click += new EventHandler(deleteMenu_Click);
            _parametersGrid.ContextMenu = new ContextMenu();
            _parametersGrid.ContextMenu.MenuItems.Add(addMenu);
            _parametersGrid.ContextMenu.MenuItems.Add(deleteMenu);
            _parametersGrid.DoAutoResizeCols();
            _parametersGrid.DoubleClickHandler = ParameterDoubleClick_Handler;

            Controls.Add(_parametersGrid);

            _parametersGrid.KeyDown += new KeyEventHandler(_parametersGrid_KeyDown);
        }

        #endregion Constructor

        #region Public methods

        #endregion Public methods

        #region Private methods

        private void ParameterDoubleClick_Handler(int rowIndex)
        {
            // do nothing here.
        }

        private void ResetGrid(ArrayList list)
        {
            _parametersGrid.BeginUpdate();
            _parametersGrid.Rows.Clear();

            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(GRID_COLUMN_NAME);
            dataTable.Columns.Add(GRID_COLUMN_VALUE);

            if (null != list && list.Count > 0)
            {
                foreach (string[] pair in list.ToArray())
                {
                    // The order is assumed to be name and followed by value
                    dataTable.Rows.Add(new Object[] { pair[0], pair[1] });
                }
            }

            dataTable.Rows.Add(new Object[] { DirectoryServer.LDAP_COMMON_CONFIG_PARAMETERS.DirectoryBase.ToString(), "" });
            //_parametersGrid.AutoResizeCols = true;
            _parametersGrid.FillWithData(dataTable);
            _parametersGrid.Cols[GRID_COLUMN_NAME].Pattern = _parameterColPattern;
            _parametersGrid.DoAutoResizeCols();
            _parametersGrid.EndUpdate();
            FireRequiredFieldsChanged(new EventArgs());
        }

        /// <summary>
        /// Event handler for add context menu
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void addMenu_Click(object sender, EventArgs e)
        {
            _parametersGrid.Rows.Add();
            FireRequiredFieldsChanged(new EventArgs());
        }

        /// <summary>
        /// Event handler for delete context menu
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void deleteMenu_Click(object sender, EventArgs e)
        {
            foreach (iGRow row in _parametersGrid.SelectedRows)
            {
                _parametersGrid.Rows.RemoveAt(row.Index);
            }

            if (_parametersGrid.Rows.Count == 0)
            {
                _parametersGrid.Rows.Add();
                FireRequiredFieldsChanged(new EventArgs());
            }
        }

        /// <summary>
        /// Event handler for key pressed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _parametersGrid_KeyDown(object sender, KeyEventArgs e)
        {
            if (null != _parametersGrid.CurCell)
            {
                if ((_parametersGrid.CurCell.ColIndex == 1) && (e.KeyCode == Keys.Tab))
                {
                    _parametersGrid.Rows.Add();
                    FireRequiredFieldsChanged(new EventArgs());
                }
                else if (e.KeyCode == Keys.Delete)
                {
                    if (_parametersGrid.Rows.Count == 1)
                    {
                        _parametersGrid.Rows.RemoveAt(_parametersGrid.CurCell.RowIndex);
                        _parametersGrid.Rows.Add();
                        FireRequiredFieldsChanged(new EventArgs());
                    }
                    else
                    {
                        _parametersGrid.Rows.RemoveAt(_parametersGrid.CurCell.RowIndex);
                        FireRequiredFieldsChanged(new EventArgs());
                    }
                }
            }
        }

        /// <summary>
        /// Fire events if required field changed.
        /// </summary>
        /// <param name="e"></param>
        private void FireRequiredFieldsChanged(EventArgs e)
        {
            if (OnRequiredFieldsChanged != null)
            {
                OnRequiredFieldsChanged(this, e);
            }
        }

        #endregion Private methods

    }
}
