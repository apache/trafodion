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

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public partial class QueryParametersDisplayUserControl : UserControl
    {
        List<ReportParameter> _theParameters = null;

        public QueryParametersDisplayUserControl()
        {
            InitializeComponent();
        }

        public List<ReportParameter> TheParameters
        {
            get { return _theParameters; }
            set
            { 
                _theParameters = value;

                Controls.Clear();

                if (_theParameters != null)
                {
                    TrafodionDataGridView theDataGridView = new TrafodionDataGridView();

                    DataGridViewTextBoxColumn theNameColumn = new DataGridViewTextBoxColumn();
                    DataGridViewTextBoxColumn theValueColumn = new DataGridViewTextBoxColumn();

                    // 
                    // theNameColumn
                    // 
                    theNameColumn.HeaderText = "Name";
                    theNameColumn.Name = "theNameColumn";

                    // 
                    // theValueColumn
                    // 
                    theValueColumn.HeaderText = "Value";
                    theValueColumn.Name = "theValueColumn";

                    // Add the columns
                    theDataGridView.Columns.AddRange(new DataGridViewColumn[] {
                        theNameColumn,
                        theValueColumn
                    });

                    foreach (ReportParameter theReportParameter in _theParameters)
                    {
                        theDataGridView.Rows.Add(new object[]
                        {
                            theReportParameter.ParamName,
                            theReportParameter.Value
                        });
                    }

                    theDataGridView.AutoResizeColumns(DataGridViewAutoSizeColumnsMode.AllCells);
                    theDataGridView.Dock = DockStyle.Fill;
                    Controls.Add(theDataGridView);

                    theDataGridView.AddCountControlToParent("This statement had {0} parameters", DockStyle.Top);
                    theDataGridView.AddButtonControlToParent(DockStyle.Bottom);

                }

            }
        }

    }
}
