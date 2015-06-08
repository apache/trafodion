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
using System.Data.Odbc;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Favorites;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// User control to display the aggregates of Tables, Table Indexes,
    /// MV and MV indexes at a schema level
    /// </summary>
    public partial class DatabaseObjectAggregateControl : UserControl
    {
        DatabaseObjectAggregateDataGridView _theGrid;
        private ConnectionDefinition _connectionDefinition = null;

        /// <summary>
        /// Displays/hides the progressbar
        /// </summary>
        /// <param name="show"></param>
        public void ShowProgress(bool show)
        {
            WorkingProgressBar.Visible = show;
            _theProgressStatus.Visible = show;
            refreshButton.Enabled = (!show);
        }

        public DatabaseObjectAggregateControl()
        {
            InitializeComponent();

            // Initially non-blank to make them easy to see in the designer so set them empty now
            //theTopPanelUpperLabel.Text = "";
            theTopPanelLowerLabel.Text = "";            

        }

        /// <summary>
        /// Calls the background thread to populate the grid with data
        /// </summary>
        /// <param name="aSqlMxSystem"></param>
        /// <param name="aSqlMxCatalog"></param>
        /// <param name="aSqlMxSchema"></param>
        public void Populate(
            SqlMxSystem aSqlMxSystem,
            SqlMxCatalog aSqlMxCatalog,
            SqlMxSchema aSqlMxSchema)
        {
             _theGrid = new DatabaseObjectAggregateDataGridView(
                aSqlMxSystem, 
                aSqlMxCatalog, 
                aSqlMxSchema);
             _theGrid.TheDatabaseObjectAggregateControl = this;
             //theTopPanelUpperLabel.Text =  "Schema : " + aSqlMxSchema.VisibleAnsiName;
             theTopPanelLowerLabel.Text = Properties.Resources.AggregateLoadingMessage;

            // Set Tooltips
             //_toolTip.SetToolTip(theTopPanelUpperLabel, theTopPanelUpperLabel.Text);

             tableGroupBox.Text = Properties.Resources.AggregateLoadingHeader;
             _theGrid.Dock = DockStyle.Fill;
            tableGroupBox.Controls.Clear();
            tableGroupBox.Controls.Add(_theGrid);
            _theGrid.DoLoad();

        }

        /// <summary>
        /// Call back method when the backgroud thread completes
        /// </summary>
        /// <param name="aSize"></param>
        public void OnPopulateComplete(long aSize)
        {
            theTopPanelLowerLabel.Text = String.Format(Properties.Resources.AggregateTotalSize, Utilities.FormatSize(aSize));
            tableGroupBox.Text = String.Format(Properties.Resources.AggregateReportTime, Utilities.CurrentFormattedDateTime);
        }

        /// <summary>
        /// Returns the title of the window being displayed
        /// </summary>
        public string WindowTitle
        {
            get { return "Schema Size"; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get
            {
                if (_connectionDefinition != null)
                {
                    return _connectionDefinition;
                }
                _connectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;
                return _connectionDefinition;
            } 
        }

        private void refreshButton_Click(object sender, EventArgs e)
        {
            _theGrid.DoLoad();
            theTopPanelLowerLabel.Text = Properties.Resources.AggregateLoadingMessage;
            tableGroupBox.Text = Properties.Resources.AggregateLoadingHeader;
        }

        private void closeButtom_Click(object sender, EventArgs e)
        {

        }
    }

}
