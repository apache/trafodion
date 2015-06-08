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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// This is a panel that contains standard buttons assoicated with a TrafodionDataGridView.
    /// </summary>
    public partial class DataGridViewButtonsUserControl : UserControl
    {

        private TrafodionDataGridView _TrafodionDataGridView;
        private DataGridViewRowsAddedEventHandler _dataGridViewRowsAddedEventHandler;
        private DataGridViewRowsRemovedEventHandler _dataGridViewRowsRemovedEventHandler;

        /// <summary>
        /// Property to get/set associated TrafodionDataGridView
        /// </summary>
        public TrafodionDataGridView TheTrafodionDataGridView
        {
            get { return _TrafodionDataGridView; }
            set 
            {
                RemoveEventHandlers();
                _TrafodionDataGridView = value;
                UpdateControls();
                AddEventHandlers();
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aTrafodionDataGridView">An assoicated TrafodionDataGridView</param>
        public DataGridViewButtonsUserControl(TrafodionDataGridView aTrafodionDataGridView)
        {
            InitializeComponent();
            TheTrafodionDataGridView = aTrafodionDataGridView;

            // Get the Button-name from the Resource
            //theExportButton.Text = Properties.Resources.ExportToFile;

            _fileButton.Text        = Properties.Resources.DataToFile;
            _clipboardButton.Text   = Properties.Resources.DataToClipboard;
            _explorerButton.Text    = Properties.Resources.DataToBrowser;
            _spreadsheetButton.Text = Properties.Resources.DataToSpreadsheet;

        }
        


        private void theTrafodionDataGridViewRowsAdded(object sender, DataGridViewRowsAddedEventArgs e)
        {
            UpdateControls();
        }

        private void theTrafodionDataGridViewRowsRemoved(object sender, DataGridViewRowsRemovedEventArgs e)
        {
            UpdateControls();
        }

        private void AddEventHandlers()
        {
            if (TheTrafodionDataGridView != null)
            {
                _dataGridViewRowsAddedEventHandler = new DataGridViewRowsAddedEventHandler(theTrafodionDataGridViewRowsAdded);
                TheTrafodionDataGridView.RowsAdded += _dataGridViewRowsAddedEventHandler;
                _dataGridViewRowsRemovedEventHandler = new DataGridViewRowsRemovedEventHandler(theTrafodionDataGridViewRowsRemoved);
                TheTrafodionDataGridView.RowsRemoved += _dataGridViewRowsRemovedEventHandler;
            }
        }

        private void RemoveEventHandlers()
        {
            if (TheTrafodionDataGridView != null)
            {
                TheTrafodionDataGridView.RowsAdded -= _dataGridViewRowsAddedEventHandler;
                _dataGridViewRowsAddedEventHandler = null;
                TheTrafodionDataGridView.RowsRemoved -= _dataGridViewRowsRemovedEventHandler;
                _dataGridViewRowsRemovedEventHandler = null;
            }
        }

        private void UpdateControls()
        {
            // enable buttons only when there is > 0 rows 
            _fileButton.Enabled        = (TheTrafodionDataGridView != null) && (TheTrafodionDataGridView.Rows.Count > 0);
            _clipboardButton.Enabled   = (TheTrafodionDataGridView != null) && (TheTrafodionDataGridView.Rows.Count > 0);
            _explorerButton.Enabled    = (TheTrafodionDataGridView != null) && (TheTrafodionDataGridView.Rows.Count > 0);
            _spreadsheetButton.Enabled = (TheTrafodionDataGridView != null) && (TheTrafodionDataGridView.Rows.Count > 0);

        }

        private void theExportButtonClick(object sender, EventArgs e)
        {
            TheTrafodionDataGridView.ExportToFile();
        }

        private void theSpreadsheetButtonClick(object sender, EventArgs e)
        {
            TheTrafodionDataGridView.ExportToSpreadsheet();
        }

        private void theClipboadButtonClick(object sender, EventArgs e)
        {
            TheTrafodionDataGridView.ExportToClipboard();
        }

 
        /// <summary>
        /// Browser e.g. Internet Explorer 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theBrowserButtonClick(object sender, EventArgs e)
        {
            TheTrafodionDataGridView.ExportToBrowser();
        }

    }
}
