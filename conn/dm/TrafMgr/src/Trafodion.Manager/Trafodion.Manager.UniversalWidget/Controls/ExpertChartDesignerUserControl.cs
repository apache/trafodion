//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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

using System.Data;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    /// <summary>
    /// User control for export chart designer
    /// </summary>
    public partial class ExpertChartDesignerUserControl : UserControl
    {
        #region Fields

        public const string CHART_ICON = "CHART_ICON";
        private Chart _theChart = null;
        private DataTable _dataTable = null;
        private string[] cols = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Chart - the chart control itself
        /// </summary>
        public Chart Chart
        {
            get { return _theChart; }
            set
            {
                _theChart = value;
                ShowTree();
            }
        }

        /// <summary>
        /// Property: DataSource - the data to be populated
        /// </summary>
        public DataTable DataSource
        {
            get { return _dataTable; }
            set 
            { 
                _dataTable = value;
                if (value != null)
                {
                    cols = new string[_dataTable.Columns.Count];
                    for (int i = 0; i < _dataTable.Columns.Count; i++)
                    {
                        cols[i] = _dataTable.Columns[i].ColumnName;
                    }
                    _theChart.DataSource = value;
                }
            }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// The default Constructor
        /// </summary>
        private ExpertChartDesignerUserControl()
        {
            InitializeComponent();
            this._theChartsTreeView.ImageList.Images.Add(CHART_ICON, Properties.Resources.chart_icon);
        }

        /// <summary>
        /// The constructor
        /// </summary>
        /// <param name="aChart"></param>
        public ExpertChartDesignerUserControl(Chart aChart)
            : this()
        {
            _theChart = aChart;
            ShowTree();
        }

        #endregion Constructor 

        #region Public methods

        /// <summary>
        /// Apply the changes to the chart
        /// </summary>
        public void Apply()
        {
            // there is not much to do here since any change to the chart configuration is reflected right away on 
            // the chart.  This is a place holder in case any additional things need to be done.
            MessageBox.Show(_theChart.Serializer.SerializableContent, "Chart Content", MessageBoxButtons.OK);
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To show the tree on the left hand side
        /// </summary>
        private void ShowTree()
        {
            if (_theChart == null)
                return;

            this.PopulateTree();
            this._theChartsTreeView.ExpandAll();
        }

        /// <summary>
        /// To populate the tree using the chart
        /// </summary>
        private void PopulateTree()
        {
            _theChartsTreeView.Nodes.Clear();

            TreeNode node = new TreeNode(Chart.Name);
            node.ImageKey = CHART_ICON;
            node.SelectedImageKey = CHART_ICON;
            this._theChartsTreeView.Nodes.Add(node);
            node.Tag = Chart;
        }

        /// <summary>
        /// Display the chart property using property grid
        /// </summary>
        /// <param name="node"></param>
        private void ShowRigthPanel(TreeNode node)
        {
            if (node.Tag != null)
            {
                PropertyGrid property = new PropertyGrid();
                property.SelectedObject = node.Tag;
                property.Dock = DockStyle.Fill;

                TrafodionGroupBox box = new TrafodionGroupBox();
                box.Text = node.Name;
                box.Controls.Add(property);
                box.Dock = DockStyle.Fill;

                _theAttributePanel.Controls.Clear();
                _theAttributePanel.Controls.Add(box);
            }
            else
            {
                _theAttributePanel.Controls.Clear();
            }
        }

        /// <summary>
        /// Event handler after the node selected event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theChartsTreeView_AfterSelect(object sender, TreeViewEventArgs e)
        {
            ShowRigthPanel(e.Node);
        }

        #endregion Private methods
    }
}
