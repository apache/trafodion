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
    /// User control to display the summary information for Tables, Indexes, Table Indexes,
    /// MV and MV indexes at a schema level
    /// </summary>
    public partial class DatabaseObjectDetailsControl : UserControl
    {

        private ConnectionDefinition _connectionDefinition = null;
        private SqlMxSchemaObjectDetailsDataGridView _theGrid = null;


        public DatabaseObjectDetailsControl()
        {
            InitializeComponent();

            // Initially non-blank to make them easy to see in the designer so set them empty now
            theTopPanelLowerLabel.Text = "";
            
            this.Update();
        }

        /// <summary>
        /// Displays/hides the progressbar
        /// </summary>
        /// <param name="show"></param>
        public void ShowProgress(bool show)
        {
            if (show)
            {
                _theGrid.SetCountMessage(Properties.Resources.SummaryLoadingMessage);
            }
            //When the progress bar is hidden, it means the processing is complete.
            //We can display the report time then.
            if (!show)
            {
                setReportTime();
                _theGrid.SetCountMessage(String.Format(Properties.Resources.SummaryCountMessage, new Object[] { _theGrid.ParentObjectType, "{0}", _theGrid.ObjectType }));
            }
            WorkingProgressBar.Visible = show ;
            _theProgressStatus.Visible = show;
            theTopPanelLowerLabel.Visible = show;
            refreshButton.Enabled = (!show);
        }

        /// <summary>
        /// Method to load the grid control
        /// </summary>
        /// <param name="aGrid"></param>
        public void populateGrid(SqlMxSchemaObjectDetailsDataGridView aGrid)
        {
            _theGrid = aGrid;
            dataGridviewButtonsPanel.Height = 35;
            Control theButtonControl = _theGrid.GetButtonControl();
            theButtonControl.Dock = DockStyle.Fill;
            dataGridviewButtonsPanel.Controls.Add(theButtonControl);
            gridviewGroupBox.Text = Properties.Resources.SummaryLoadingHeader;
            
            _theGrid.Dock = DockStyle.Fill;
            theTabControlPanel.Controls.Clear();
            theTabControlPanel.Controls.Add(_theGrid);
            //theTopPanelLowerLabel.Text = _theGrid.ObjectInformation;
            theTopPanelLowerLabel.Text = Properties.Resources.AggregateLoadingMessage;

            // Set Tooltips
            //_toolTip.SetToolTip(theTopPanelLowerLabel, theTopPanelLowerLabel.Text);

            _theGrid.AddCountControlToParent(String.Format(Properties.Resources.SummaryCountMessage, new Object[] { _theGrid.ParentObjectType, "{0}", _theGrid.ObjectType}), DockStyle.Top);

        }

        private void setReportTime()
        {
            gridviewGroupBox.Text = String.Format(Properties.Resources.SummaryReportTime, Utilities.CurrentFormattedDateTime);
        }

        public string WindowTitle
        {
            get { return theTopPanelLowerLabel.Text; }
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
            if (_theGrid != null)
            {
                _theGrid.Refresh();
                gridviewGroupBox.Text = Properties.Resources.SummaryLoadingHeader;
            }
        }
    }

}
