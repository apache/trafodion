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
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.MetricMiner;
namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class HistorylogPanel : UserControl
    {
        GenericUniversalWidget _theWidget;
        UniversalWidgetConfig _theConfig;
        public delegate void OnRowDoubleClicked(HistorylogElement hle);
        public event OnRowDoubleClicked OnRowDoubleClickedEvent;
        public HistorylogPanel()
        {
            InitializeComponent();
            addLogWidget();
        }


        private void addLogWidget()
        {
            _theConfig = WidgetRegistry.GetDefaultDBConfig();
            _theConfig.Name = "Metric Miner History";

            _theWidget = new GenericUniversalWidget();
            _theWidget.DataProvider = new HistoryLogDataProvider(_theConfig.DataProviderConfig);
            _theWidget.UniversalWidgetConfiguration = _theConfig;
            _theWidget.Dock = DockStyle.Fill;

            TabularDataDisplayControl displayControl = _theWidget.DataDisplayControl as TabularDataDisplayControl;
            displayControl.DataGrid.DoubleClickHandler =  this.DataGrid_CellDoubleClick;
            _theLogPanel.Controls.Add(_theWidget);
            _theWidget.DataProvider.Start();
        }

        void DataGrid_CellDoubleClick(int row)
        {
            if (row < 0)
            {
                return;
            }
            if (OnRowDoubleClickedEvent != null)
            {
                TabularDataDisplayControl displayControl = _theWidget.DataDisplayControl as TabularDataDisplayControl;
                OnRowDoubleClickedEvent((HistorylogElement)displayControl.DataGrid.Rows[row].Cells[2].Value);
            }
        }

        //void DataGrid_CellDoubleClick(object sender, TenTec.Windows.iGridLib.iGCellDoubleClickEventArgs e)
        //{
        //    int row = e.RowIndex;
        //    if (row < 0)
        //    {
        //        return;
        //    }
        //    if (OnRowDoubleClickedEvent != null)
        //    {
        //        OnRowDoubleClickedEvent((HistorylogElement)((TrafodionIGrid)sender).Rows[row].Cells[2].Value);
        //    }

        //}
        //

    }

}
