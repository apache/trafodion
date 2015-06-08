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
using System.Linq;
using System.Text;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using System.Data.Odbc;
using Trafodion.Manager.Framework;
using System.IO;
using System.Data;
using Trafodion.Manager.OverviewArea.Models;
using System.ComponentModel;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// dataGridView for OSIM download control
    /// </summary>
    class OSIMDataGridView : TrafodionDataGridView
    {
        private Connection _connection;
        private DataTable _fileList;
        private TarHandler _tarHandler;

        DataGridViewCheckBoxColumn theCheckBoxColumn;
        DataGridViewTextBoxColumn theFileNameColumn;
        DataGridViewTextBoxColumn theFileModifyDateeColumn;
        DataGridViewTextBoxColumn theSizeColumn;
        DataGridViewTextBoxColumn theSizeCopyColumn;

        /// <summary>
        /// The constructor.
        /// </summary>
        public OSIMDataGridView(TarHandler aTarHandler)
        {
            this._tarHandler = aTarHandler;
            // Create the columns
            theCheckBoxColumn = new DataGridViewCheckBoxColumn();
            theFileNameColumn = new DataGridViewTextBoxColumn();
            theFileModifyDateeColumn = new DataGridViewTextBoxColumn();
            theSizeColumn = new DataGridViewTextBoxColumn();
            theSizeCopyColumn = new DataGridViewTextBoxColumn();

            theCheckBoxColumn.HeaderText = "";
            theCheckBoxColumn.Name = "theCheckBox";
            theCheckBoxColumn.FalseValue = 0;
            theCheckBoxColumn.TrueValue = 1;
            theCheckBoxColumn.Resizable = DataGridViewTriState.False;
            theCheckBoxColumn.AutoSizeMode = DataGridViewAutoSizeColumnMode.ColumnHeader;
            theCheckBoxColumn.MinimumWidth = 45;
            theCheckBoxColumn.ReadOnly = false;
            theCheckBoxColumn.DataPropertyName = ColumnStore.Select;

            // 
            // the file name Column
            // 
            theFileNameColumn.HeaderText = "Tar File Name";
            theFileNameColumn.Name = "theFileName";
            theFileNameColumn.DataPropertyName = ColumnStore.FileName;
            // 
            // the File Modify Date Column
            // 
            theFileModifyDateeColumn.HeaderText = "Creation Date";
            theFileModifyDateeColumn.Name = "theFileModifyDate";
            theFileModifyDateeColumn.DataPropertyName = ColumnStore.FileLastModify;
            // 
            // the Size Column
            // 
            theSizeColumn.HeaderText = "Size";
            theSizeColumn.Name = "theSize";
            theSizeColumn.DataPropertyName = ColumnStore.FileSize;
            // 
            // the SizeCopy Column which is unvisible
            // 
            theSizeCopyColumn.HeaderText = "CopySize";
            theSizeCopyColumn.Name = "theCopySize";
            theSizeCopyColumn.DataPropertyName = ColumnStore.FileSizeCopy;
            theSizeCopyColumn.Visible = false;

            this.AutoGenerateColumns = false;

            // Add the columns
            Columns.AddRange(new DataGridViewColumn[] {
                theCheckBoxColumn,
                theFileNameColumn,
                theFileModifyDateeColumn,
                theSizeColumn,
                theSizeCopyColumn
            });

            this.ColumnHeaderMouseClick += new DataGridViewCellMouseEventHandler(OSIMDataGridView_ColumnHeaderMouseClick);
        }

        ~OSIMDataGridView()
        {

        }

        /// <summary>
        ///get file list from server and populate to the grid
        /// </summary>
        public void Populate()
        {
            this._fileList = _tarHandler.getFileInfo();
            if (_fileList == null)
                return;

            _fileList.Columns.Add(ColumnStore.Select, typeof(System.Int32));
            _fileList.Columns.Add(ColumnStore.FileSizeCopy, typeof(System.Int64));
            for (int i = 0; i < _fileList.Rows.Count; i++)
            {
                _fileList.Rows[i][ColumnStore.Select] = 0;
                //change size unit
                string fileSize = _fileList.Rows[i][ColumnStore.FileSize].ToString();
                _fileList.Rows[i][ColumnStore.FileSize] = Utilities.FormatSize(decimal.Parse(fileSize));
                //size copy
                _fileList.Rows[i][ColumnStore.FileSizeCopy] = Int64.Parse(fileSize);
                //formate time
                long ticks = Int64.Parse(_fileList.Rows[i][ColumnStore.FileLastModify].ToString());
                _fileList.Rows[i][ColumnStore.FileLastModify] = Utilities.GetStandardFormattedTimeFromUnixTimestamp(ticks);
            }
            //set order
            _fileList.Columns[ColumnStore.Select].SetOrdinal(0);
            _fileList.Columns[ColumnStore.FileName].SetOrdinal(1);
            _fileList.Columns[ColumnStore.FileLastModify].SetOrdinal(2);
            _fileList.Columns[ColumnStore.FileSize].SetOrdinal(3);
            _fileList.Columns[ColumnStore.FileSizeCopy].SetOrdinal(4);
        }

        public void bindDatasouce()
        {
            this.DataSource = _fileList;
        }

        private void OSIMDataGridView_ColumnHeaderMouseClick(object sender, DataGridViewCellMouseEventArgs e)
        {
            if (SortOrder == SortOrder.None || SortedColumn == null)
            {
                // No column select as the sort column. Do nothing.
                return;
            }

            //if sort column 3, sort column 4 instead
            if (e.ColumnIndex == 3)
            {
                //here this.theSizeCopyColumn.HeaderText is just used to mark SortGlyphDirection in column size 
                if (this.theSizeCopyColumn.HeaderText == "Descending")
                {
                    Sort(this.theSizeCopyColumn, ListSortDirection.Descending);
                    this.theSizeColumn.HeaderCell.SortGlyphDirection = System.Windows.Forms.SortOrder.Descending;
                    this.theSizeCopyColumn.HeaderText = "CopySize";
                }
                else if (SortOrder == SortOrder.Ascending)
                {
                    Sort(this.theSizeCopyColumn, ListSortDirection.Ascending);
                    this.theSizeColumn.HeaderCell.SortGlyphDirection = System.Windows.Forms.SortOrder.Ascending;
                    this.theSizeCopyColumn.HeaderText = "Descending";
                }
            }
            else
            {
                this.theSizeCopyColumn.HeaderText = "CopySize";
            }
        }

        private class ColumnStore
        {
            public const string Select = "Select";
            public const string FileName = "FileName";
            public const string FileSize = "FileSize";
            public const string FileSizeCopy = "FileSizeCopy";
            public const string FileLastModify = "FileLastModify";
        }

    }
}
