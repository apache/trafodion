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
﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms.DataVisualization.Charting;

namespace Trafodion.Manager.UniversalWidget.Controls
{
    public partial class ChartDesigner : TrafodionForm
    {
        ChartConfig _theChartConfig = null;
        int _theCurrentPage = 1;
        DataTable _theDataTable;
        string _theSelectedColumn;
        Dictionary<string, SeriesChartType> _theChartTypes = new Dictionary<string, SeriesChartType>();

        public ChartDesigner()
        {
            InitializeComponent();
            _theChartName.Text      = "Chart Name";
            _theChartType.Text      = "Chart Type";
            _theSeriesColumn.Text   = "Y Axis column";
            _theSeriesColumnDataType.Text = "Y Axis Data Type";
            _theLegend.Text         = "Series Legend Text";
            _theLegend.ShowRequired = false;
            _theXAxis.Text          = "X Axis Column";
            _theXAxisDataType.Text = "X Axis Data Type";
            _theXAxisDataType.ShowRequired = false;
            _theXAxis.ShowRequired  = false;

            //load the chart types

            //_theChartTypes.Add("Area", SeriesChartType.Area);
            //_theChartTypes.Add("Bar", SeriesChartType.Bar);
            //_theChartTypes.Add("Box Plot", SeriesChartType.BoxPlot);
            //_theChartTypes.Add("Bubble", SeriesChartType.Bubble);
            //_theChartTypes.Add("Candlestick", SeriesChartType.Candlestick);
            //_theChartTypes.Add("Column", SeriesChartType.Column);
            //_theChartTypes.Add("Doughnut", SeriesChartType.Doughnut);
            //_theChartTypes.Add("ErrorBar", SeriesChartType.ErrorBar);
            //_theChartTypes.Add("FastLine", SeriesChartType.FastLine);
            //_theChartTypes.Add("FastPoint", SeriesChartType.FastPoint);
            //_theChartTypes.Add("Funnel", SeriesChartType.Funnel);
            //_theChartTypes.Add("Kagi", SeriesChartType.Kagi);
            //_theChartTypes.Add("Line", SeriesChartType.Line);
            //_theChartTypes.Add("Pie", SeriesChartType.Pie);
            //_theChartTypes.Add("Point", SeriesChartType.Point);
            //_theChartTypes.Add("PointAndFigure", SeriesChartType.PointAndFigure);
            //_theChartTypes.Add("Polar", SeriesChartType.Polar);
            //_theChartTypes.Add("Pyramid", SeriesChartType.Pyramid);
            //_theChartTypes.Add("Radar", SeriesChartType.Radar);
            //_theChartTypes.Add("Range", SeriesChartType.Range);
            //_theChartTypes.Add("RangeBar", SeriesChartType.RangeBar);
            //_theChartTypes.Add("RangeColumn", SeriesChartType.RangeColumn);
            //_theChartTypes.Add("Renko", SeriesChartType.Renko);
            //_theChartTypes.Add("Spline", SeriesChartType.Spline);
            //_theChartTypes.Add("SplineArea", SeriesChartType.SplineArea);
            //_theChartTypes.Add("SplineRange", SeriesChartType.SplineRange);
            //_theChartTypes.Add("StackedArea", SeriesChartType.StackedArea);
            //_theChartTypes.Add("StackedArea100", SeriesChartType.StackedArea100);
            //_theChartTypes.Add("StackedBar", SeriesChartType.StackedBar);
            //_theChartTypes.Add("StackedBar100", SeriesChartType.StackedBar100);
            //_theChartTypes.Add("StackedColumn", SeriesChartType.StackedColumn);
            //_theChartTypes.Add("StackedColumn100", SeriesChartType.StackedColumn100);
            //_theChartTypes.Add("StepLine", SeriesChartType.StepLine);
            //_theChartTypes.Add("Stock", SeriesChartType.Stock);
            //_theChartTypes.Add("ThreeLineBreak", SeriesChartType.ThreeLineBreak);

            _theChartTypeCombo.Items.AddRange(Enum.GetNames(typeof(System.Windows.Forms.DataVisualization.Charting.SeriesChartType)));

            //foreach (KeyValuePair<string, SeriesChartType> kvp in _theChartTypes)
            //{
            //    _theChartTypeCombo.Items.Add(kvp.Key);
            //}
            _theChartTypeCombo.SelectedItem = SeriesChartType.Line.ToString();

            _theSeriesColumnDataTypeCombo.Items.AddRange(Enum.GetNames(typeof(System.Windows.Forms.DataVisualization.Charting.ChartValueType)));
            _theXAxisDataTypeCombo.Items.AddRange(Enum.GetNames(typeof(System.Windows.Forms.DataVisualization.Charting.ChartValueType)));
            _theNextButton.Text = "Create";
            this.CenterToParent();
        }

        public ChartDesigner(ChartConfig aChartConfig) : this()
        {
            _theChartConfig = aChartConfig;
        }
        public DataTable DataTable
        {
            set { _theDataTable = value; }
        }

        public string SelectedColumn
        {
            set { _theSelectedColumn = value; }
        }

        public void PopulateUI()
        {
            if (_theChartConfig.ChartAreaConfigs.Count > 0)
            {
                //showPage(1);
                populateGraphList(_theChartConfig);
            }
            else
            {
                //showPage(2);
            }

        }

        private void populateGraphList(ChartConfig aChartConfig)
        {
            _theGraphsList.Items.Clear();
            _theGraphsList.Items.Add(new ListItem("Create a new chart", null));
            foreach (ChartAreaConfig areaConfig in aChartConfig.ChartAreaConfigs)
            {
                _theGraphsList.Items.Add(new ListItem(areaConfig.Name, areaConfig));
            }
        }

        private void showPage(int page)
        {
            _theCurrentPage = page;
            this.Size = new Size(335, 250);
            this.Controls.Clear();
            switch (page)
            {
                case 1:
                    {
                        _theGraphListPanel.Visible = true;
                        _theChartAttributePanel.Visible = false;
                        _thePreviousButton.Visible = false;
                        _theNextButton.Text = "Next >>";

                        _theMainPanel.Controls.Clear();
                        _theButtonPanel.Dock = DockStyle.Bottom;
                        this.Controls.Add(_theButtonPanel);

                        _theGraphListPanel.Dock = DockStyle.Fill;
                        this.Controls.Add(_theGraphListPanel);
                    }
                    break;
                case 2:
                    {
                        _theGraphListPanel.Visible = false;
                        _theChartAttributePanel.Visible = true;
                        _thePreviousButton.Visible = (_theChartConfig.ChartAreaConfigs.Count > 0);
                        _theNextButton.Text = "OK";
                        populatePropertyPanel();
                        
                        _theMainPanel.Controls.Clear();
                        _theButtonPanel.Dock = DockStyle.Bottom;
                        this.Controls.Add(_theButtonPanel);

                        _theChartAttributePanel.Dock = DockStyle.Fill;
                        this.Controls.Add(_theChartAttributePanel);
                    }
                    break;

            }
            this.Validate();
        }

        private void _thePreviousButton_Click(object sender, EventArgs e)
        {
            showPage(--_theCurrentPage);
        }

        private void _theNextButton_Click(object sender, EventArgs e)
        {
            //if (_theNextButton.Text == "Create")
            //{
                //populate the chart config from input
                GetAreaConfigFromUI();

                this.DialogResult = DialogResult.OK;
                this.Close();
            //}
            //else
            //{
            //    showPage(++_theCurrentPage);
            //}
        }

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void populatePropertyPanel()
        {
            _theSeriesColumnCombo.Items.Clear();
            _theXAxisCombo.Items.Clear();
            List<string> columnNames = GetColumnList();
            //foreach (string col in columnNames)
            //{
            //    _theSeriesColumnCombo.Items.Add(col);
            //    _theXAxisCombo.Items.Add(col);
            //}
            //_theXAxisCombo.SelectedIndex = 0;
            //_theSeriesColumnCombo.SelectedItem = _theSelectedColumn;
            //_theLegendText.Text = (string)_theSeriesColumnCombo.SelectedItem;
            //ListItem selectedItem = _theGraphsList.SelectedItem as ListItem;
            //if ((selectedItem == null) || (selectedItem.AreaConfig == null))
            //{
            //    //its a new report
            //    _theChartNameText.Text = "";
            //}
            //else
            //{
            //    _theChartNameText.Text = selectedItem.AreaConfig.Name;
                
            //}
            _theSeriesColumnCombo.Items.AddRange(columnNames.ToArray());
            _theXAxisCombo.Items.AddRange(columnNames.ToArray());
            _theChartTypeCombo.SelectedItem = SeriesChartType.Line.ToString();
            _theSeriesColumnCombo.SelectedItem = "";
            _theSeriesColumnDataTypeCombo.SelectedIndex = -1;
            _theXAxisCombo.SelectedItem = "";
            _theXAxisDataTypeCombo.SelectedIndex = -1;
            _theChartNameText.Text = "";
            _theLegendText.Text = "";

            ListItem selectedItem = _theGraphsList.SelectedItem as ListItem;
            if (selectedItem == null)
                return;

            ChartAreaConfig areaConfig = selectedItem.AreaConfig;
            if (areaConfig == null)
                return;

            _theChartNameText.Text = areaConfig.Name;
            if (areaConfig.ChartSeriesConfigs.Count > 0)
            {
                ChartSeriesConfig seriesConfig = areaConfig.ChartSeriesConfigs[0];
                _theChartTypeCombo.SelectedItem = seriesConfig.ChartType.ToString();
                _theSeriesColumnCombo.SelectedItem = seriesConfig.YValueColumnName;
                _theSeriesColumnDataTypeCombo.SelectedItem = seriesConfig.YValueColumnDataType;
                _theXAxisCombo.SelectedItem = seriesConfig.XValueColumnName;
                _theXAxisDataTypeCombo.SelectedItem = seriesConfig.XValueColumnDataType;
                _theLegendText.Text = seriesConfig.Text;
                _theChartTypeCombo.SelectedItem = seriesConfig.ChartType.ToString();
            }
        }

        private ChartAreaConfig GetAreaConfigFromUI()
        {
            bool alterExistingConfig = false;
            ChartAreaConfig areaConfig = null;
            ListItem selectedItem = _theGraphsList.SelectedItem as ListItem;
            if ((selectedItem == null) || (selectedItem.AreaConfig == null))
            {
                areaConfig = new ChartAreaConfig();
                areaConfig.Name = _theChartNameText.Text.Trim();
                _theChartConfig.ChartAreaConfigs.Add(areaConfig);
            }
            else
            {
                areaConfig = selectedItem.AreaConfig;
                selectedItem.AreaConfig.ChartSeriesConfigs.Clear();
            }

            ChartSeriesConfig seriesConfig = new ChartSeriesConfig();
            seriesConfig.ChartAreaName = areaConfig.Name;
            seriesConfig.Name = areaConfig.Name + "_" + (string)_theSeriesColumnCombo.SelectedItem;
            seriesConfig.Text = _theLegendText.Text.Trim();
            LegendConfig legendConfig = null;
            if ((_theChartConfig.LegendConfigs != null) && (_theChartConfig.LegendConfigs.Count > 0))
            {
                legendConfig = _theChartConfig.LegendConfigs[0];
            }
            else
            {
                legendConfig = new LegendConfig("Legend", "Legend");
                if (_theChartConfig.LegendConfigs == null)
                {
                    _theChartConfig.LegendConfigs = new List<LegendConfig>();
                }
                _theChartConfig.LegendConfigs.Add(legendConfig);
            }
            seriesConfig.ChartType = _theChartTypes[(string)_theChartTypeCombo.SelectedItem];
            seriesConfig.LegendName = legendConfig.Name;
            seriesConfig.YValueColumnName = (string)_theSeriesColumnCombo.SelectedItem;
            seriesConfig.XValueColumnName = (_theXAxisCombo.SelectedItem != null) ? (string)_theXAxisCombo.SelectedItem : "";
            seriesConfig.XValueColumnDataType = (string)_theXAxisDataTypeCombo.SelectedItem;
            seriesConfig.YValueColumnDataType = (string)_theSeriesColumnDataTypeCombo.SelectedItem;
            areaConfig.ChartSeriesConfigs.Add(seriesConfig);
            return areaConfig;

        }

        private List<string> GetColumnList()
        {
            List<string> ret = new List<string>();
            ret.Add("");
            if (_theDataTable != null)
            {
                foreach (DataColumn dc in _theDataTable.Columns)
                {
                    ret.Add(dc.ColumnName);
                }
            }
            return ret;
        }

        private void _removeButton_Click(object sender, EventArgs e)
        {

        }

        private void _theGraphsList_SelectedValueChanged(object sender, EventArgs e)
        {
            if (_theGraphsList.SelectedItem != null)
            {
                ListItem listItem = (ListItem) _theGraphsList.SelectedItem;
                if (listItem.AreaConfig == null)
                {
                    _theNextButton.Text = "Create";
                }
                else
                {
                    _theNextButton.Text = "Save";
                }

                populatePropertyPanel();
            }
        }
    }


    class ListItem
    {
        string _theText;
        ChartAreaConfig _theAreaConfig;
        
        public ListItem(string text, ChartAreaConfig aChartAreaConfig)
        {
            _theText = text;
            _theAreaConfig = aChartAreaConfig;
        }
        
        public ChartAreaConfig AreaConfig
        {
            get { return _theAreaConfig; }
            set { _theAreaConfig = value; }
        }

        public string Text
        {
            get { return _theText; }
            set { _theText = value; }
        }



        public override string ToString()
        {
            return _theText;
        }
    }
}
