//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.DatabaseArea.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Model;
using System.Collections.Generic;
using TenTec.Windows.iGridLib;
using System.Drawing;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class ParentChildQueriesUserControl : UserControl
    {
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        private MonitorWorkloadCanvas _parentChildQueriesWidget; 

        public ParentChildQueriesUserControl()
        {
            InitializeComponent();
        }

        public ParentChildQueriesUserControl(ConnectionDefinition connectionDefinition, string queryID, Size preferredSize)
            :this()
        {
            InitializeParentChildQueriesWidget(connectionDefinition, queryID, preferredSize);
            PopulateQueryInfo(queryID);
        }

        private void PopulateQueryInfo(string queryID)
        {
            _queryIDTextBox.Text = queryID;
        }

        private void InitializeParentChildQueriesWidget(ConnectionDefinition connectionDefinition, string queryID, Size preferredSize)
        {
            if (queryID == null) queryID = string.Empty;
            _parentChildQueriesWidget = new MonitorWorkloadCanvas(connectionDefinition, queryID, preferredSize);
            _parentChildQueriesWidget.Dock = DockStyle.Fill;
            _parentChildQueriesPanel.Controls.Add(_parentChildQueriesWidget);
            _parentChildQueriesWidget.TheMonitorWorkloadWidget.StartDataProvider();
        }

        private bool IsParentQueryRow(iGRow gridRow)
        {
            bool isParentQueryRow = false;

            string parentQueryId = (string)gridRow.Cells["PARENT_QUERY_ID"].Value;
            if (parentQueryId != null)
            {
                if (parentQueryId.Trim().Length == 0 
                    || string.Compare(parentQueryId, "NONE", true) == 0)
                {
                    isParentQueryRow = true;
                }
            }

            return isParentQueryRow;
        }
    }
}
