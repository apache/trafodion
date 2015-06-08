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

using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// Display user control for query control settings
    /// </summary>
    public partial class QueryControlStatementsDisplayUserControl : UserControl
    {
        #region Fields

        private List<ReportControlStatement> _theControlStatements = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: TheControlStatements - list of control statements for the session's control settings
        /// </summary>
        public List<ReportControlStatement> TheControlStatements
        {
            get { return _theControlStatements; }
            set
            {
                _theControlStatements = value;

                Controls.Clear();

                if (_theControlStatements != null)
                {
                    TrafodionDataGridView theDataGridView = new TrafodionDataGridView();

                    DataGridViewTextBoxColumn theCQTypeColumn = new DataGridViewTextBoxColumn();
                    DataGridViewTextBoxColumn theAttributeColumn = new DataGridViewTextBoxColumn();
                    DataGridViewTextBoxColumn theValueColumn = new DataGridViewTextBoxColumn();

                    // 
                    // theCQTypeColumn
                    // 
                    theCQTypeColumn.HeaderText = "Control Statement";
                    theCQTypeColumn.Name = "theCQTypeColumn";

                    // 
                    // theAttributeColumn
                    // 
                    theAttributeColumn.HeaderText = "Attribute";
                    theAttributeColumn.Name = "theAttributeColumn";

                    // 
                    // theValueColumn
                    // 
                    theValueColumn.HeaderText = "Value";
                    theValueColumn.Name = "theValueColumn";

                    // Add the columns
                    theDataGridView.Columns.AddRange(new DataGridViewColumn[] {
                        theCQTypeColumn,
                        theAttributeColumn,
                        theValueColumn
                    });

                    foreach (ReportControlStatement theReportControlStatement in _theControlStatements)
                    {
                        if (!theReportControlStatement.Disable)
                        {
                            theDataGridView.Rows.Add(new object[]
                            {
                                theReportControlStatement.CQType,
                                theReportControlStatement.Attribute,
                                theReportControlStatement.Value
                            });
                        }
                    }

                    theDataGridView.AutoResizeColumns(DataGridViewAutoSizeColumnsMode.AllCells);
                    theDataGridView.Dock = DockStyle.Fill;
                    Controls.Add(theDataGridView);

                    if (theDataGridView.Rows.Count > 1)
                    {
                        theDataGridView.AddCountControlToParent("{0} control statements evaluated", DockStyle.Top);
                    }
                    else
                    {
                        theDataGridView.AddCountControlToParent("{0} control statement evaluated", DockStyle.Top);
                    }

                    theDataGridView.AddButtonControlToParent(DockStyle.Bottom);
                }
            }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// Default constructor
        /// </summary>
        public QueryControlStatementsDisplayUserControl()
        {
            InitializeComponent();
        }

        #endregion Constructor

    }
}
