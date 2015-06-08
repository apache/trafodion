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

using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class DatabaseMonitoringControl : UserControl
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public DatabaseMonitoringControl()
        {
            InitializeComponent();

            TrafodionDataGridView.StandardInit(dgvDBMonTableFull);
            TrafodionDataGridView.StandardInit(dgvDBMonObjectMaintain);
            TrafodionDataGridView.StandardInit(dgvDBMonDiskFull);

            string[] row60 = { "Gold", "Schema1.Table1", "92.37%", "7.63%", "Oct 13 2007, 4:55 PM MST", "ROLE.DBA" };
            string[] row61 = { "Silver", "Schema3.Table4", "87.58%", "12.42%", "Oct 13 2007, 4:55 PM MST", "ROLE.DBA" };
            string[] row62 = { "Silver", "Schema4.Table1", "86.67%", "13.33%", "Oct 13 2007, 4:55 PM MST", "ROLE.DBA" };
            string[] row63 = { "Lead", "Schema1.Table3", "86.37%", "13.63%", "Oct 13 2007, 4:55 PM MST", "ROLE.DBA" };
            string[] row64 = { "Lead", "Schema5.Table5", "85.43%", "14.57%", "Oct 13 2007, 4:55 PM MST", "ROLE.DBA" };

            dgvDBMonTableFull.Rows.Clear();
            dgvDBMonTableFull.Rows.Add(row60);
            dgvDBMonTableFull.Rows.Add(row61);
            dgvDBMonTableFull.Rows.Add(row62);
            dgvDBMonTableFull.Rows.Add(row63);
            dgvDBMonTableFull.Rows.Add(row64);

            string[] row70 = { "Silver", "Schema7.Table7", "Table", "Update Stats" };
            string[] row71 = { "Silver", "Schema9.Table5.Index1", "Index", "Some Action" };
            string[] row72 = { "Lead", "Schema7.Table7", "Table", "Reorganize" };

            dgvDBMonObjectMaintain.Rows.Clear();
            dgvDBMonObjectMaintain.Rows.Add(row70);
            dgvDBMonObjectMaintain.Rows.Add(row71);
            dgvDBMonObjectMaintain.Rows.Add(row72);

            string[] row80 = { "Gold", "GLD01001/DB12", "87.72%", "12.28%", "354" };
            string[] row81 = { "Silver", "SLV01007/DB07", "85.72%", "14.28%", "134" };

            dgvDBMonDiskFull.Rows.Clear();
            dgvDBMonDiskFull.Rows.Add(row80);
            dgvDBMonDiskFull.Rows.Add(row81);
        }
    }
}
