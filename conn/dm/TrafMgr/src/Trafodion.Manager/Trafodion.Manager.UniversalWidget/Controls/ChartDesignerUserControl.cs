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

using System;
using System.Data;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using System.Reflection;
using System.Threading;
using System.Drawing;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    /// <summary>
    /// Control for chart desinger
    /// </summary>
    public partial class ChartDesignerUserControl : UserControl
    {
        #region Fields

        /// <summary>
        /// Keys for the image icon
        /// </summary>
        public const string CHART_ROOT_ICON = "CHART_ROOT_ICON";
        public const string CHART_AREA_ICON = "CHART_AREA_ICON";
        public const string CHART_SERIES_ICON = "CHART_SERIES_ICON";

        /// <summary>
        /// Define a status change event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public delegate void StatusChanged(object sender, EventArgs e);
        public event StatusChanged OnStatusChanged;

        private ChartConfig _theChartConfig = null;
        private bool isChartValid = false;
        private string chartError = string.Empty;
        private ChartConfig testChartConfig = null;
        private DataTable testDataTable = null;

        // This can be used when user wants to reload to original
        private ChartConfig _theBackupChartConfig = null;

        // Datatable to populate the chart
        private DataTable _dataTable = null;
        private string[] cols = null;

        //public const string CHART_ROOT_PREFIX = Properties.Resources.ChartRootPrefix; //"Chart Root";
        //public const string CHART_AREA_PREFIX = Properties.Resources.ChartAreaPrefix; //"Chart Area: ";
        //public const string CHART_SERIES_PREFIX = Properties.Resources.ChartSeriesPrefix; //"Chart Series: ";
        //private const string CHART_AREA_NODE_NAME_TEMPLATE = CHART_AREA_PREFIX + "{0}";
        //private const string CHART_SERIES_NODE_NAME_TEMPLATE = CHART_SERIES_PREFIX + "{0}";

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: ChartConfig - the configuration for this chart
        /// </summary>
        public ChartConfig ChartConfig
        {
            get { return _theChartConfig; }
            set
            {
                _theChartConfig = value;
                ShowTree();
            }
        }

        /// <summary>
        /// Property: DataSource - the data source for the chart
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
                }
            }
        }


        /// <summary>
        /// StatusMessage: the error message to be displayed.
        /// </summary>
        public string StatusMessage
        {
            get { return _theStatusTextBox.Text; }
            set
            {
                if (!_theStatusTextBox.Text.Equals(value))
                {
                    _theStatusTextBox.Text = value;
                    this.FireOnStatusChanged(new EventArgs());
                }

                if (String.IsNullOrEmpty(_theStatusTextBox.Text))
                {
                    _theStatusTextBox.Visible = false;
                }
                else
                {
                    _theStatusTextBox.Visible = true;
                }
            }
        }

        #endregion Properties

        #region Constructor
        /// <summary>
        /// Default Constructor 
        /// </summary>
        private ChartDesignerUserControl()
        {
            InitializeComponent();
            this._theChartsTreeView.ImageList.Images.Add(CHART_ROOT_ICON, Properties.Resources.chart_root_icon);
            this._theChartsTreeView.ImageList.Images.Add(CHART_AREA_ICON, Properties.Resources.chart_icon);
            this._theChartsTreeView.ImageList.Images.Add(CHART_SERIES_ICON, Properties.Resources.base_charts_icon);
            StatusMessage = "";
            _theAddToolStripButton.Enabled = false;
            _theDeleteToolStripButton.Enabled = false;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aChartConfig"></param>
        public ChartDesignerUserControl(ChartConfig aChartConfig)
            : this()
        {
            _theChartConfig = aChartConfig;
            _theBackupChartConfig = aChartConfig.DeepCopy();
            ShowTree();
            _theChartsTreeView.SelectedNode = _theChartsTreeView.Nodes[0]; // select the first node 
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Apply any of the chart configuration made so far to the chart.
        /// </summary>
        public bool Apply(DataTable dataTable)
        {
            bool isValid = true;
            bool currentNodeNameChanged = false;
            TreeNode currentlySelectedNode = _theChartsTreeView.SelectedNode;
            ChartConfig newChartConfig;

            if (_theChartsTreeView.Nodes.Count > 0)
            {
                TreeNode root = _theChartsTreeView.Nodes[0];
                ChartConfig chartConfig = root.Tag as ChartConfig;
                if (chartConfig == null)
                {
                    //[TBD] something wrong here.
                    newChartConfig = new ChartConfig();
                }
                else
                {
                    newChartConfig = chartConfig.ShallowCopy();
                }

                // Now, go on to the children
                foreach (TreeNode areaNode in root.Nodes)
                {
                    ChartAreaConfig areaConfig = areaNode.Tag as ChartAreaConfig;
                    if (areaConfig != null)
                    {
                        // Check if the area name has been changed.
                        //if (!areaNode.Text.Equals(areaConfig.Name))
                        if (IsNodeNameChanged(areaNode, areaConfig.Name))
                        {
                            areaNode.Name = areaNode.Text = string.Format(Properties.Resources.ChartAreaPrefix + "{0}", areaConfig.Name);
                            if (areaNode == currentlySelectedNode)
                            {
                                currentNodeNameChanged = true;
                            }
                        }

                        newChartConfig.ChartAreaConfigs.Add(areaConfig);

                        // Fix up the Legend if it is necessary
                        if (areaConfig.EnableLegend)
                        {
                            LegendConfig legend = GetLegendConfigForChartArea(newChartConfig, areaConfig.Name);
                            if (legend == null)
                            {
                                legend = new LegendConfig(string.Format("legend{0}_{1}", newChartConfig.LegendConfigs.Count, areaConfig.Name), areaConfig.LegendTitle);
                                legend.DockedChartAreaName = areaConfig.Name;
                                areaConfig.LegendName = legend.Name;
                                newChartConfig.LegendConfigs.Add(legend);
                            }
                            else
                            {
                                // Only changeable attribute
                                legend.Text = areaConfig.LegendTitle;
                            }
                        }
                        else
                        {
                            // Remove the legend since it is not enabled.
                            LegendConfig legend = GetLegendConfigForChartArea(newChartConfig, areaConfig.Name);
                            if (legend != null)
                            {
                                newChartConfig.LegendConfigs.Remove(legend);
                            }
                            areaConfig.LegendName = "";
                            areaConfig.LegendTitle = "";
                            areaConfig.LegendDocking = Docking.Right;
                            areaConfig.IsLegendInsideChart = false;
                        }

                        // Fix up the Title if it is necessary
                        if (areaConfig.EnableTitle)
                        {
                            TitleConfig title = GetTitleConfigForChartArea(newChartConfig, areaConfig.Name);
                            if (title == null)
                            {
                                title = new TitleConfig(string.Format("title{0}_{1}", newChartConfig.TitleConfigs.Count, areaConfig.Name), areaConfig.Title);
                                title.DockedChartAreaName = areaConfig.Name;
                                areaConfig.TitleName = title.Name;
                                newChartConfig.TitleConfigs.Add(title);
                            }
                            else
                            {
                                // Only changable attribute
                                title.Text = areaConfig.Title;
                            }
                        }
                        else
                        {
                            // Remove the title since there is not enabled.
                            TitleConfig title = GetTitleConfigForChartArea(newChartConfig, areaConfig.Name);
                            if (title != null)
                            {
                                newChartConfig.TitleConfigs.Remove(title);
                            }
                            areaConfig.TitleName = "";
                            areaConfig.TitleDocking = Docking.Top;
                            areaConfig.Title = "";
                            areaConfig.IsTitleInsideChart = false;
                        }

                        areaConfig.ChartSeriesConfigs.Clear();

                        foreach (TreeNode seriesNode in areaNode.Nodes)
                        {
                            ChartSeriesConfig seriesConfig = seriesNode.Tag as ChartSeriesConfig;
                            if (seriesConfig != null)
                            {
                                //if (!seriesNode.Text.Equals(seriesConfig.Name))
                                if (IsNodeNameChanged(seriesNode, seriesConfig.Name))
                                {
                                    seriesNode.Name = seriesNode.Text = string.Format(Properties.Resources.ChartSeriesPrefix + "{0}", seriesConfig.Name);
                                    if (seriesNode == currentlySelectedNode)
                                    {
                                        currentNodeNameChanged = true;
                                    }
                                }

                                seriesConfig.LegendName = areaConfig.LegendName;
                                seriesConfig.ChartAreaName = areaConfig.Name;
                                areaConfig.ChartSeriesConfigs.Add(seriesConfig);
                            }
                        }
                    }
                }

                isValid = ValidateChart(newChartConfig, dataTable);

                if (isValid)
                {
                    _theChartConfig = newChartConfig;
                    if (currentNodeNameChanged)
                    {
                        ShowRightPanel(_theChartsTreeView.SelectedNode);
                    }
                }
            }

            return isValid;
        }

        /// <summary>
        /// Reset to the original chart configuration when the chart designer is first started.
        /// Note: this does not reload from the serialization.
        /// </summary>
        public void Reset()
        {
            _theChartConfig = _theBackupChartConfig.DeepCopy();
            ShowTree();
            this._theChartsTreeView.SelectedNode = _theChartsTreeView.Nodes[0]; // select the 1st node
        }

        #endregion Public methods

        #region Private methods

        #region Vlidate Chart

        /// <summary>
        /// Validate if chart has valid settings. Calling this method before applying chart settings could avoid chart crashing.
        /// Especially, it prevents from below crashing error:
        /// [
        ///          Chart Area Axes - The chart area contains incompatible chart types. 
        ///          For example, bar charts and column charts cannot exist in the same chart area.
        /// ]
        /// </summary>
        /// <param name="chartConfig"></param>
        /// <param name="dataTable"></param>
        /// <returns></returns>
        private bool ValidateChart(ChartConfig chartConfig, DataTable dataTable)
        {
            this.isChartValid = true;
            this.chartError = string.Empty;

            if (_theChartsTreeView.Nodes.Count > 0)
            {
                try
                {
                    this.testChartConfig = chartConfig;
                    this.testDataTable = dataTable;
                    Thread chartTesting = new Thread(this.TestChart);
                    chartTesting.Start();
                    chartTesting.Join();
                }
                catch (Exception e)
                {
                    this.isChartValid = false;
                    this.chartError = e.Message;
                }
                finally
                {
                    this.testChartConfig = null;
                    this.testDataTable = null;
                }
            }

            if (!this.isChartValid)
            {
                MessageBox.Show(this.chartError, Properties.Resources.ChartDesigner, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            return this.isChartValid;
        }

        private void TestChart()
        {
            try
            {
                Application.ThreadException += Application_ThreadException;

                Chart chart = new Chart();
                Form form = new Form();
                form.Controls.Add(chart);

                // Make the testing pop-up form transparent
                form.AllowTransparency = true;
                form.TransparencyKey = form.BackColor;

                ChartRenderer render = ChartRendererFactory.GetChartRenderer(this.testChartConfig);
                render.RenderChart(chart, this.testChartConfig, this.testDataTable);

                // Make the testing pop-up form same as parent form so that it fully overlaps with partent form and users cannot feel its existence
                form.Load += (sender, arg) =>
                {
                    form.Location = this.ParentForm.Location;
                    form.Size = this.ParentForm.Size;
                };

                // Make the testing pop-up form out of the screen so that it will be invisible to users.
                form.Shown += (sender, arg) =>
                {
                    form.Location = new Point(-1000, -1000);
                };

                // If chart is painted successfully, close the form. It indicates the chart is valid
                chart.PostPaint += (sender, arg) =>
                {
                    form.Close();
                };

                // Show the form containing chart to verify if the chart can be successfully painted
                form.ShowDialog();
            }
            finally
            {
                Application.ThreadException -= Application_ThreadException;
            }
        }

        void Application_ThreadException(object sender, System.Threading.ThreadExceptionEventArgs e)
        {
            this.isChartValid = false;
            this.chartError = "Error";
            if (e.Exception != null)
            {
                this.chartError = e.Exception.Message;
            }

            Application.ExitThread();
        }

        #endregion

        /// <summary>
        /// Show the tree on the left hand side
        /// </summary>
        private void ShowTree()
        {
            if (_theChartConfig == null)
                return;

            this.PopulateTree();
            this._theChartsTreeView.ExpandAll();
        }

        /// <summary>
        /// Populate the tree using the given Chart Config parent.
        /// </summary>
        private void PopulateTree()
        {
            _theChartsTreeView.Nodes.Clear();

            // First, add a root node.
            TreeNode root = new TreeNode(Properties.Resources.ChartRootPrefix);
            root.Name = root.Text;
            root.ImageKey = CHART_ROOT_ICON;
            root.SelectedImageKey = CHART_ROOT_ICON;
            root.Tag = _theChartConfig; // keep the root chart config
            this._theChartsTreeView.Nodes.Add(root);

            // Enumerate and display all key and value pairs.
            foreach (ChartAreaConfig areaConfig in _theChartConfig.ChartAreaConfigs)
            {
                TreeNode node = CreateChartAreaNode(areaConfig.Name);
                node.Tag = areaConfig;
                root.Nodes.Add(node);

                // Then going through the series in a chart area.
                foreach (ChartSeriesConfig seriesConfig in areaConfig.ChartSeriesConfigs)
                {
                    TreeNode childNode = CreateChartSeriesNode(seriesConfig.Name);
                    childNode.Tag = seriesConfig;
                    node.Nodes.Add(childNode);
                }
            }
        }

        private bool IsNodeNameChanged(TreeNode node, string name)
        {
            if (node.Tag is ChartAreaConfig)
            {
                string nodeName = node.Name.Substring(Properties.Resources.ChartAreaPrefix.Length);
                return (!nodeName.Equals(name));
            }
            else
            {
                string nodeName = node.Name.Substring(Properties.Resources.ChartSeriesPrefix.Length);
                return (!nodeName.Equals(name));
            }

        }

        private TreeNode CreateChartAreaNode(string name)
        {
            TreeNode node = new TreeNode(string.Format(Properties.Resources.ChartAreaPrefix + "{0}", name));
            node.Name = node.Text;
            node.ImageKey = CHART_AREA_ICON;
            node.SelectedImageKey = CHART_AREA_ICON;
            return node;
        }

        private TreeNode CreateChartSeriesNode(string name)
        {
            TreeNode node = new TreeNode(string.Format(Properties.Resources.ChartSeriesPrefix + "{0}", name));
            node.Name = node.Text;
            node.ImageKey = CHART_SERIES_ICON;
            node.SelectedImageKey = CHART_SERIES_ICON;
            return node;
        }

        /// <summary>
        /// Display the selected node's property in a property grid on the right panel.
        /// </summary>
        /// <param name="node"></param>
        private void ShowRightPanel(TreeNode node)
        {
            if (node.Tag != null)
            {
                PropertyGrid property = new PropertyGrid();
                property.SelectedObject = node.Tag;
                property.Dock = DockStyle.Fill;
                GridItemCollection items = GetAllGridEntries(property);
                if (items.Count > 0)
                {
                    items[0].Select();
                }
                else
                {
                    property.CollapseAllGridItems();
                }

                TrafodionGroupBox box = new TrafodionGroupBox();
                box.Text = node.Name;
                box.Controls.Add(property);
                box.Dock = DockStyle.Fill;

                _theAttributePanel.Controls.Clear();
                _theAttributePanel.Controls.Add(box);

                if (node.Tag is ChartSeriesConfig)
                {
                    property.PropertyValueChanged += Property_PropertyValueChanged;
                    property.Disposed += Property_Disposed;
                }

            }
            else
            {
                _theAttributePanel.Controls.Clear();
            }
        }

        void Property_Disposed(object sender, EventArgs e)
        {
            PropertyGrid property = sender as PropertyGrid;
            property.PropertyValueChanged -= Property_PropertyValueChanged;
            property.Disposed -= Property_Disposed;
        }

        private void Property_PropertyValueChanged(object s, PropertyValueChangedEventArgs e)
        {
            if (e.ChangedItem.Label.Equals("Y Axis Column Name") ||
                e.ChangedItem.Label.Equals("X Axis Column Name"))
            {
                ValidateChartsTree();
            }
        }

        private static GridItemCollection GetAllGridEntries(PropertyGrid grid) 
        { 
            if (grid == null)         
                throw new ArgumentNullException("grid"); 
            object view = grid.GetType().GetField("gridView", BindingFlags.NonPublic | BindingFlags.Instance).GetValue(grid); 
            return (GridItemCollection)view.GetType().InvokeMember("GetAllGridEntries", BindingFlags.InvokeMethod | BindingFlags.NonPublic | BindingFlags.Instance, null, view, null); 
        } 

        /// <summary>
        /// The handler for after node selection event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theChartsTreeView_AfterSelect(object sender, TreeViewEventArgs e)
        {
            ShowRightPanel(e.Node);
            if (_theChartsTreeView.SelectedNode.Tag != null)
            {
                if (_theChartsTreeView.SelectedNode.Tag is ChartConfig)
                {
                    // Selecting Chart root
                    _theAddToolStripButton.Enabled = true;
                    _theDeleteToolStripButton.Enabled = false;
                }
                else if (_theChartsTreeView.SelectedNode.Tag is ChartAreaConfig)
                {
                    // Selecting a Chart Area
                    _theAddToolStripButton.Enabled = true;
                    _theDeleteToolStripButton.Enabled = (_theChartsTreeView.SelectedNode.Nodes.Count == 0);
                }
                else
                {
                    // Selecting a Chart Series
                    _theAddToolStripButton.Enabled = false;
                    _theDeleteToolStripButton.Enabled = true;
                }
            }

            _theChartsTreeView.Select();
            ValidateChartsTree();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        private void ValidateChartsTree()
        {
            StatusMessage = "";

            if (_theChartsTreeView == null)
            {
                StatusMessage = "Error: Chart does not exist!";
                return;
            }

            if (_theChartsTreeView.Nodes.Count != 1)
            {
                StatusMessage = "Error: More than one chart root exist!";
                return;
            }

            TreeNode root = _theChartsTreeView.Nodes[0];
            if (!(root.Tag is ChartConfig))
            {
                StatusMessage = "Error: Chart root does not exist!";
                return;
            }

            if (root.Nodes.Count == 0)
            {
                StatusMessage = "Warning: There is no chart area configured.";
                return;
            }

            foreach (TreeNode area in root.Nodes)
            {
                if (area.Nodes.Count > 0)
                {
                    foreach (TreeNode series in area.Nodes)
                    {
                        ChartSeriesConfig seriesConfig = series.Tag as ChartSeriesConfig;
                        if (seriesConfig == null)
                        {
                            StatusMessage = string.Format("Error: Chart series ({0}) is not properly configured!", seriesConfig.Name);
                        }
                        else
                        {
                            if (string.IsNullOrEmpty(seriesConfig.XValueColumnName) ||
                                string.IsNullOrEmpty(seriesConfig.YValueColumnName))
                            {
                                StatusMessage = string.Format("Chart series ({0}) configuration has not been completed; Please specify X and/or Y column names!", seriesConfig.Name);
                            }
                            else
                            {
                                StatusMessage = "";
                            }
                        }
                    }
                }
                else
                {
                    if (string.IsNullOrEmpty(StatusMessage))
                    {
                        StatusMessage = "Warning: One or more chart areas do not have chart series configured.";
                    }
                }
            }
        }

        /// <summary>
        /// The event handler for Add button.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theAddToolStripButton_Click(object sender, EventArgs e)
        {
            if (_theChartsTreeView.SelectedNode == null)
            {
                // User did not select any nodes, so just go ahead to add an chart area to the root.
                TreeNode root = _theChartsTreeView.Nodes[0];
                if (root != null)
                {
                    ChartAreaConfig areaConfig = new ChartAreaConfig();
                    areaConfig.Name = FindAUniqueName(root.Nodes, "area");
                    TreeNode node = CreateChartAreaNode(areaConfig.Name);
                    node.Tag = areaConfig;
                    root.Nodes.Add(node);

                    this._theChartsTreeView.SelectedNode = node;
                }
                // else, there must be something wrong, there should always a root exist
            }
            else
            {
                TreeNode parent = this._theChartsTreeView.SelectedNode;
                TreeNode node = null;
                if (parent.Tag is ChartConfig)
                {
                    // Selecting the root node, so let's add a new chart area
                    ChartAreaConfig areaConfig = new ChartAreaConfig();
                    areaConfig.Name = FindAUniqueName(parent.Nodes, "chart");
                    node = CreateChartAreaNode(areaConfig.Name);
                    node.Tag = areaConfig;
                    parent.Nodes.Add(node);
                }
                else if (parent.Tag is ChartAreaConfig)
                {
                    // Selecting a chart area, so let's add a new series to the chart area
                    ChartSeriesConfig seriesConfig = new ChartSeriesConfig();
                    seriesConfig.ChartAreaName = parent.Name;
                    seriesConfig.LegendName = parent.Name;
                    seriesConfig.Name = FindAUniqueName(parent.Nodes, string.Format("{0}_series", parent.Name));
                    node = CreateChartSeriesNode(seriesConfig.Name);
                    node.Tag = seriesConfig;
                    parent.Nodes.Add(node);
                }
                //else if (parent.Tag is ChartSeriesConfig)
                //{
                //    // Selecting a series, so let's back up to the parent and add a new series to the chart area
                //    parent = parent.Parent;
                //    ChartSeriesConfig seriesConfig = new ChartSeriesConfig();
                //    seriesConfig.ChartAreaName = parent.Name;
                //    seriesConfig.Name = FindAUniqueName(parent.Nodes, string.Format("{0}_series", parent.Name));
                //    node = new TreeNode(seriesConfig.Name);
                //    node.Name = seriesConfig.Name;
                //    node.Tag = seriesConfig;
                //    parent.Nodes.Add(node);
                //}
                else
                {
                    // Selecting a series, no children can be added. 
                    StatusMessage = "Please select a Chart Area to add a Chart Series.";
                }

                this._theChartsTreeView.SelectedNode = node;
            }
        }

        /// <summary>
        /// Find a unique name in the given tree node collection
        /// </summary>
        /// <param name="nodes"></param>
        /// <param name="prefix"></param>
        /// <returns></returns>
        private string FindAUniqueName(TreeNodeCollection nodes, string prefix)
        {
            int idx = nodes.Count;
            bool found = false;
            while (!found)
            {
                string name = string.Format("{0}{1}", prefix, idx);
                if (!nodes.ContainsKey(name))
                {
                    return name;
                }
                else
                {
                    idx++;
                }
            }

            return null;
        }

        /// <summary>
        /// The event handler for delete button.
        /// Note: a node can be removed only if there is no more child nodes.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theDeleteToolStripButton_Click(object sender, EventArgs e)
        {
            TreeNode node = _theChartsTreeView.SelectedNode;

            if (node != null)
            {
                if (node.Nodes.Count > 0)
                {
                    string message = "";
                    if (node.Tag is ChartConfig)
                    {
                        StatusMessage = "Chart root cannot be removed.";
                    }
                    else if (node.Tag is ChartAreaConfig)
                    {
                        StatusMessage = "There are still chart series in the chart area. Please remove them before removing the chart area.";
                    }
                }
                else
                {
                    this._theChartsTreeView.Nodes.Remove(_theChartsTreeView.SelectedNode);
                }
            }
        }

        /// <summary>
        /// Search a legend configuration from a ChartConfig parent 
        /// </summary>
        /// <param name="aChartConfig"></param>
        /// <param name="aChartAreaName"></param>
        /// <returns></returns>
        private LegendConfig GetLegendConfigForChartArea(ChartConfig aChartConfig, string aChartAreaName)
        {
            foreach (LegendConfig legend in aChartConfig.LegendConfigs)
            {
                if (!string.IsNullOrEmpty(legend.DockedChartAreaName) && legend.DockedChartAreaName.Equals(aChartAreaName))
                {
                    return legend;
                }
            }

            return null;
        }

        /// <summary>
        /// Search a title configuration from a ChartConfig parent
        /// </summary>
        /// <param name="aChartConfig"></param>
        /// <param name="aChartAreaName"></param>
        /// <returns></returns>
        private TitleConfig GetTitleConfigForChartArea(ChartConfig aChartConfig, string aChartAreaName)
        {
            foreach (TitleConfig title in aChartConfig.TitleConfigs)
            {
                if (!string.IsNullOrEmpty(title.DockedChartAreaName) && title.DockedChartAreaName.Equals(aChartAreaName))
                {
                    return title;
                }
            }

            return null;
        }

        /// <summary>
        /// Fire the chart configuration change event
        /// </summary>
        /// <param name="e"></param>
        private void FireOnStatusChanged(EventArgs e)
        {
            if (OnStatusChanged != null)
            {
                OnStatusChanged(this, e);
            }
        }

        #endregion Private methods
    }

    #region Class: ChartsObjectNode

    /// <summary>
    /// This class shall be used to display a user control
    /// </summary>
    public class ChartsObjectNode : TreeNode
    {
        #region Member
        String _title;
        Control _chartPanel = null;
        #endregion

        #region Constructor
        public ChartsObjectNode(String aTitle, Object aChartObject)
        {
            _title = aTitle;
            this.Text = aTitle;
            this.Tag = aChartObject;

            _chartPanel = new PropertyGrid();
            ((PropertyGrid)_chartPanel).SelectedObject = aChartObject;

        }
        #endregion

        #region OptionNode
        /// <summary>
        /// Returns the Control that shall be displayed on the right
        /// </summary>
        public Control ChartsPanel
        {
            get
            {
                return _chartPanel;
            }
        }

        /// <summary>
        /// Returns the title of the option
        /// </summary>
        public string ChartsTitle
        {
            get
            {
                return _title;
            }
        }

        #endregion
    }

    #endregion Class: ChartsObjectNode

    #region Class: ChartNode
    /// <summary>
    /// Interface to be implemented by all TreeNode objects that shall be used to 
    /// display options
    /// </summary>
    public interface ChartNode
    {
        /// <summary>
        /// Returns the Control that shall be displayed on the right
        /// </summary>
        Control ChartPanel { get; }

        /// <summary>
        /// Returns the title of the chart
        /// </summary>
        string ChartTitle { get; }

        /// <summary>
        /// The name of the option provider
        /// </summary>
        string ChartName { get; }
    }

    #endregion Class: ChartNode
}
