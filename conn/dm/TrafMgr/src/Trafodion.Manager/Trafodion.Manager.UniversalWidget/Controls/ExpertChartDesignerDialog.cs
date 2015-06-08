// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    public partial class ExpertChartDesignerDialog : Form
    {
        private GenericUniversalWidget _theWidget = null;
        private ExpertChartDesignerUserControl _theUserControl = null;
        private DataTable _theDataTable = null;

        public DataTable DataTable
        {
            get { return _theDataTable; }
            set 
            { 
                _theDataTable = value;
                if (value != null)
                    _theUserControl.DataSource = value;
            }
        }



        public ExpertChartDesignerDialog(GenericUniversalWidget aWidget)
        {
            // defensive code to catch programming error here. 
            if (!aWidget.UniversalWidgetConfiguration.SupportCharts)
            {
                throw new Exception("Error: widget does not support charts, invoking Chart Designer is not valid.");
            }

            _theWidget = aWidget;
            InitializeComponent();
            ShowWidget();
        }


        private void ShowWidget()
        {
            _theUserControl = new ExpertChartDesignerUserControl(_theWidget.ChartControl.TheChart);
            _theUserControl.Dock = DockStyle.Fill;
            _upperPanel.Controls.Add(_theUserControl);
        }

        private void _theOkButton_Click(object sender, EventArgs e)
        {
            _theUserControl.Apply();
            if (_theDataTable != null)
            {
                _theWidget.ChartControl.PopulateChart(_theDataTable);
            }
            this.Hide();
        }

        private void _theApplyButton_Click(object sender, EventArgs e)
        {
            _theUserControl.Apply();
            if (_theDataTable != null)
            {
                _theWidget.ChartControl.PopulateChart(_theDataTable);
            }
        }

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            this.Hide();
        }
    }
}
