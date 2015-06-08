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
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// User Control displays the active systems and some of their properties in a datagrid
    /// </summary>
    public partial class ConnectivityActiveSystemsUserControl : UserControl
    {
        /// <summary>
        /// Constructor for this control
        /// </summary>
        /// <param name="connectivityAreaSystemsUserControl"></param>
        public ConnectivityActiveSystemsUserControl(ConnectivityAreaSystemsUserControl connectivityAreaSystemsUserControl)
        {
            InitializeComponent();

            ConnectivityObjectsIGrid<NDCSSystem> ConnectivityActiveSystemsDataGrid = new ConnectivityObjectsIGrid<NDCSSystem>();

            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(Properties.Resources.SystemName, typeof(Trafodion.Manager.ConnectivityArea.Model.NDCSObject));
            dataTable.Columns.Add(Properties.Resources.MyActiveSystemState, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MyActiveSystemOdbcServerVersion, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MyActiveSystemServerDataSource, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MyActiveSystemUserName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MyActiveSystemHost, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MyActiveSystemPortNumber, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MyActiveSystemDefaultSchema, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MyActiveSystemDriverString, typeof(System.String));

            foreach (ConnectionDefinition theConnectionDefinition in ConnectionDefinition.ActiveConnectionDefinitions)
            {
                NDCSSystem theNDCSSystem = NDCSSystem.FindNDCSSystem(theConnectionDefinition);
                dataTable.Rows.Add(new object[] {
                    theNDCSSystem,
                    theNDCSSystem.ConnectionDefinition.StateString, 
                    theNDCSSystem.ConnectionDefinition.OdbcServerVersion,
                    theNDCSSystem.ConnectionDefinition.ClientDataSource,
                    theNDCSSystem.ConnectionDefinition.UserName,
                    theNDCSSystem.ConnectionDefinition.Host,
                    theNDCSSystem.ConnectionDefinition.Port,
                    theNDCSSystem.ConnectionDefinition.FullyQualifiedDefaultSchema,
                    theNDCSSystem.ConnectionDefinition.DriverString
                });
            }

            ConnectivityActiveSystemsDataGrid.FillWithData(dataTable);
            ConnectivityActiveSystemsDataGrid.ObjectLinkColumnNumber = 0;
            ConnectivityActiveSystemsDataGrid.Dock = DockStyle.Fill;
            ConnectivityActiveSystemsDataGrid.ResizeGridColumns(dataTable, 7, 30);
            ConnectivityActiveSystemsDataGrid.TreeView = connectivityAreaSystemsUserControl.ConnectivityTreeView;

            TrafodionPanel1.Controls.Add(ConnectivityActiveSystemsDataGrid);

            string _timestamp = Utilities.CurrentFormattedDateTime;
            
            string _strformat = string.Format(Properties.Resources.RefreshTimestampCaption, _timestamp)
                + Properties.Resources.ActiveSystemsCount;
            ConnectivityActiveSystemsDataGrid.AddCountControlToParent(_strformat, DockStyle.Top);
            ConnectivityActiveSystemsDataGrid.AddButtonControlToParent(DockStyle.Bottom);
        }

    }
}

