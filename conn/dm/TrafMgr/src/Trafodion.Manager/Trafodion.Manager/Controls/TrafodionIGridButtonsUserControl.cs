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
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// This is a panel that contains standard buttons assoicated with a TrafodionIGrid.
    /// </summary>
    public partial class TrafodionIGridButtonsUserControl : UserControl
    {

        private TrafodionIGrid _grid;
        private InvalidateEventHandler _dataGridViewInvalidateEventHandler;

        /// <summary>
        /// Property to get/set associated TrafodionIGrid
        /// </summary>
        private TrafodionIGrid Grid
        {
            get { return _grid; }
            set 
            {
                RemoveEventHandlers();
                _grid = value;
                UpdateControls();
                AddEventHandlers();
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="grid">An assoicated TrafodionIGrid</param>
        public TrafodionIGridButtonsUserControl(TrafodionIGrid grid)
        {
            InitializeComponent();
            Grid = grid;

            _fileButton.Text        = Properties.Resources.DataToFile;
            _clipboardButton.Text   = Properties.Resources.DataToClipboard;
            _explorerButton.Text    = Properties.Resources.DataToBrowser;
            _spreadsheetButton.Text = Properties.Resources.DataToSpreadsheet;

        }

        private void MyDispose(bool disposing)
        {
            RemoveEventHandlers();
            _grid = null;
        }

        private void AddEventHandlers()
        {
            if (Grid != null)
            {
               _dataGridViewInvalidateEventHandler = new InvalidateEventHandler(TheTrafodionDataGridView_Invalidated);
               Grid.Invalidated += _dataGridViewInvalidateEventHandler;
            }
        }

        void TheTrafodionDataGridView_Invalidated(object sender, InvalidateEventArgs e)
        {
            UpdateControls();
        }

        private void RemoveEventHandlers()
        {
            if (Grid != null)
            {
                Grid.Invalidated -= _dataGridViewInvalidateEventHandler;
                _dataGridViewInvalidateEventHandler = null;
            }
        }

        private void UpdateControls()
        {
            // enable buttons only when there is > 0 rows 
            _fileButton.Enabled        = (Grid != null) && (Grid.Rows.Count > 0);
            _clipboardButton.Enabled   = (Grid != null) && (Grid.Rows.Count > 0);
            _explorerButton.Enabled    = (Grid != null) && (Grid.Rows.Count > 0);
            _spreadsheetButton.Enabled = (Grid != null) && (Grid.Rows.Count > 0);

        }

        private void theExportButtonClick(object sender, EventArgs e)
        {
            Grid.ExportToFile();
        }

        private void theSpreadsheetButtonClick(object sender, EventArgs e)
        {
            Grid.ExportToSpreadsheet();
        }

        private void theClipboadButtonClick(object sender, EventArgs e)
        {
            Grid.ExportToClipboard();
        }
 
        /// <summary>
        /// Browser e.g. Internet Explorer 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theBrowserButtonClick(object sender, EventArgs e)
        {
            Grid.ExportToBrowser();
        }

    }
}
