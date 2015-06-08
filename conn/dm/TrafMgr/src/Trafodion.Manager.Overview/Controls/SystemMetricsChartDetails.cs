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
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Controls;
using System.Windows.Forms.DataVisualization.Charting;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class SystemMetricsChartDetails : TrafodionForm
    {
        private DataTable _theDataTable = null;
        private Chart theChart = null;
        private string _theName = null;

        public SystemMetricsChartDetails()
        {
            InitializeComponent();
        }

        public SystemMetricsChartDetails(string name, OverallSummaryControl3 control)
        {
            InitializeComponent();
            this.Text += " - "+name;
            theChart = new Chart();
            _theName = name;
            _theDataTable = control.ChartCounterDataTable;

            if (name.Equals("Core"))
            {
                string[] chartTitleArray = new string[] { "Core"};
                string[] xValues = new string[] { "Node&CoreID" };
                string[] yValues = new string[] { "%Busy"};
                theChart = control.AddChart(chartTitleArray, control.ChartCounterDataTable, xValues, yValues, new Color[] { Color.Blue });
            }
            if (name.Equals("Network")) 
            {
                string[] chartTitleArray = new string[] { "Network", "Network", "Network", "Network", "Network", "Network" };
                string[] xValues = new string[] { "Node ID", "Node ID", "Node ID", "Node ID", "Node ID", "Node ID" };
                string[] yValues = new string[] { "Rcv Packets", "Txn Packets", "Rcv Drops", "Rcv Errs", "Txn Drops", "Txn Errs" };
                Color[] colorChoices = new Color[] { Color.Olive, Color.Indigo, Color.DeepSkyBlue, Color.DarkSalmon, Color.Orange, Color.Green };
                theChart = control.AddChart(chartTitleArray, control.ChartCounterDataTable, xValues, yValues, colorChoices);
            }

            if (name.Equals("Load Avg")) 
            {
                string[] chartTitleArray = new string[] { "Load Avg", "Load Avg", "Load Avg"};
                string[] xValues = new string[] { "Node ID", "Node ID", "Node ID" };
                string[] yValues = new string[] { "1 Min Avg", "5 Min Avg", "15 Min Avg" };
                Color[] colorChoices = new Color[] { Color.Gold, Color.DarkCyan, Color.Green };
                theChart = control.AddChart(chartTitleArray, control.ChartCounterDataTable, xValues, yValues, colorChoices);
            }

            //Hide the title in Chart 
            foreach (Title theTitle in theChart.Titles) 
            {
                theTitle.Visible = false;
            }

            this.Controls.Add(theChart);
            theChart.Dock = DockStyle.Fill;

            control.OnNewDataArrival += new OverallSummaryControl3.NewDataArrivalHandler(control3_OnNewDataArrival);
            
        }

        void control3_OnNewDataArrival(object sender, EventArgs e)
        {
            DataView dv = new DataView(_theDataTable);

            //chart1.Invalidate();
            //chart1.Series["Series1"].Points.DataBindY(dv, "%Avg Core Total");
            //chart1.Series["Series2"].Points.DataBindY(dv, "Reads + Writes");
            //chart1.Series["Series3"].Points.DataBindY(dv, "1 Min Avg");
            ////chart1.Series["Series4"].Points.DataBindY(dv, "Context Switches");
            //chart1.Update();
            //chart2.Invalidate();
            ////chart2.Series["Series1"].Points.DataBindY(dv, "Rcv Packets");
            ////chart2.Series["Series1"].Points.DataBindY(dv, "Txn Packets");
            //chart2.Series["Series3"].Points.DataBindY(dv, "%Memory Used");
            //chart2.Series["Series4"].Points.DataBindY(dv, "%Swap Used");
            ////chart2.Series["Series5"].Points.DataBindY(dv, "%Consumded");
            //chart2.Update();

            theChart.Invalidate();
            if (_theName.Equals("Core"))
            {
                theChart.Series["Series1"].Points.DataBindY(dv, "%Busy");
            }

            else if (_theName.Equals("Network"))
            {
                theChart.Series["Series1"].Points.DataBindY(dv, "Rcv Packets");
                theChart.Series["Series2"].Points.DataBindY(dv, "Txn Packets");
                theChart.Series["Series3"].Points.DataBindY(dv, "Rcv Drops");
                theChart.Series["Series4"].Points.DataBindY(dv, "Rcv Errs");
                theChart.Series["Series5"].Points.DataBindY(dv, "Txn Drops");
                theChart.Series["Series6"].Points.DataBindY(dv, "Txn Errs");
            }

            else if (_theName.Equals("Load Avg"))
            {
                theChart.Series["Series1"].Points.DataBindY(dv, "1 Min Avg");
                theChart.Series["Series2"].Points.DataBindY(dv, "5 Min Avg");
                theChart.Series["Series3"].Points.DataBindY(dv, "15 Min Avg");
            }
            theChart.Update();
        }

        void control4_OnNewDataArrival(object sender, NewDataEventArgs e)
        {
            DataTable table = e.DataTable;
            if ((table != null) && (_theName.Equals(table.TableName)))
            {
                DataView dv0 = new DataView(table);
                theChart.Invalidate();
                if (_theName.Equals("Core"))
                {
                    //theChart.Series["Series1"].Points.DataBindY(dv0, "%Busy");
                    theChart.Series["Series1"].XValueType = ChartValueType.String;
                    theChart.ChartAreas[0].AxisX.IsLabelAutoFit = true;
                    theChart.ChartAreas[0].AxisX.LabelStyle.Enabled = true;
                    theChart.ChartAreas[0].AxisX.LabelAutoFitStyle = LabelAutoFitStyles.LabelsAngleStep30;
                    theChart.Series["Series1"].Points.DataBindXY(dv0, "Node&CoreID", dv0, "Busy");
                }
                else if (_theName.Equals("Load Avg"))
                {
                    theChart.Series["Series1"].Points.DataBindY(dv0, "1 Min Avg");
                    theChart.Series["Series2"].Points.DataBindY(dv0, "5 Min Avg");
                    theChart.Series["Series3"].Points.DataBindY(dv0, "15 Min Avg");
                }
                else if (_theName.Equals("Network"))
                {
                    try
                    {
                        //theChart.Series["Series1"].XValueType = ChartValueType.String;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Enabled = false;
                        theChart.ChartAreas[0].AxisX.MajorTickMark.Enabled = false;
                        theChart.ChartAreas[1].AxisX.LabelStyle.Enabled = false;
                        theChart.ChartAreas[1].AxisX.MajorTickMark.Enabled = false;
                        theChart.ChartAreas[2].AxisX.LabelStyle.Enabled = false;
                        theChart.ChartAreas[2].AxisX.MajorTickMark.Enabled = false;
                        theChart.ChartAreas[3].AxisX.LabelStyle.Enabled = false;
                        theChart.ChartAreas[3].AxisX.MajorTickMark.Enabled = false;
                        theChart.ChartAreas[4].AxisX.LabelStyle.Enabled = false;
                        theChart.ChartAreas[4].AxisX.MajorTickMark.Enabled = false;
                        theChart.ChartAreas[5].AxisX.IsLabelAutoFit = true;
                        theChart.ChartAreas[5].AxisX.LabelStyle.Enabled = true;
                        theChart.ChartAreas[5].AxisX.LabelAutoFitStyle = LabelAutoFitStyles.LabelsAngleStep90;
                        theChart.Series["Series1"].Points.DataBindY( dv0, "Rcv Packets");
                        theChart.Series["Series2"].Points.DataBindY(dv0, "Txn Packets");
                        theChart.Series["Series3"].Points.DataBindY(dv0, "Rcv Drops");
                        theChart.Series["Series4"].Points.DataBindY(dv0, "Rcv Errs");
                        theChart.Series["Series5"].Points.DataBindY(dv0, "Txn Drops");
                        theChart.Series["Series6"].Points.DataBindXY(dv0, "NodeID&IFName", dv0, "Txn Errs");
                    }
                    catch (Exception ex)
                    { }
                }
                theChart.Update();
                return;
            }

            //DataView dv = new DataView(_theDataTable);

            //chart1.Invalidate();
            //chart1.Series["Series1"].Points.DataBindY(dv, "%Avg Core Total");
            //chart1.Series["Series2"].Points.DataBindY(dv, "Reads + Writes");
            //chart1.Series["Series3"].Points.DataBindY(dv, "1 Min Avg");
            ////chart1.Series["Series4"].Points.DataBindY(dv, "Context Switches");
            //chart1.Update();
            //chart2.Invalidate();
            ////chart2.Series["Series1"].Points.DataBindY(dv, "Rcv Packets");
            ////chart2.Series["Series1"].Points.DataBindY(dv, "Txn Packets");
            //chart2.Series["Series3"].Points.DataBindY(dv, "%Memory Used");
            //chart2.Series["Series4"].Points.DataBindY(dv, "%Swap Used");
            ////chart2.Series["Series5"].Points.DataBindY(dv, "%Consumded");
            //chart2.Update();

            //theChart.Invalidate();
            //if (_theName.Equals("Core"))
            //{
            //    theChart.Series["Series1"].Points.DataBindY(dv0, "%Busy");
            //}
            //else if (_theName.Equals("Load Avg"))
            //{
            //    theChart.Series["Series1"].Points.DataBindY(dv, "1 Min Avg");
            //    theChart.Series["Series2"].Points.DataBindY(dv, "5 Min Avg");
            //    theChart.Series["Series3"].Points.DataBindY(dv, "15 Min Avg");
            //}
            //theChart.Update();
        }

        public SystemMetricsChartDetails(string name, OverallSummaryControl4 control)
        {
            InitializeComponent();
            this.Text += " - " + name;
            theChart = new Chart();
            _theName = name;
            _theDataTable = control.ChartCounterDataTable;

            if (name.Equals("Core"))
            {
                string[] chartTitleArray = new string[] { "Core" };
                string[] xValues = new string[] { "Node & Core ID" };
                string[] yValues = new string[] { "%Busy" };
                theChart = control.AddChart(chartTitleArray, control.CoreTable, xValues, yValues, new Color[] { Color.Blue });
            }
            if (name.Equals("Network"))
            {
                string[] chartTitleArray = new string[] { "Network", "Network", "Network", "Network", "Network", "Network" };
                string[] xValues = new string[] { "NodeID&IFName", "NodeID&IFName", "NodeID&IFName", "NodeID&IFName", "NodeID&IFName", "NodeID&IFName" };
                string[] yValues = new string[] { "Rcv Packets", "Txn Packets", "Rcv Drops", "Rcv Errs", "Txn Drops", "Txn Errs" };
                Color[] colorChoices = new Color[] { Color.Olive, Color.Indigo, Color.DeepSkyBlue, Color.DarkSalmon, Color.Orange, Color.Green };
                theChart = control.AddChart(chartTitleArray, control.ChartCounterDataTable, xValues, yValues, colorChoices);
            }

            if (name.Equals("Load Avg"))
            {
                string[] chartTitleArray = new string[] { "Load Avg", "Load Avg", "Load Avg" };
                string[] xValues = new string[] { "Node ID", "Node ID", "Node ID" };
                string[] yValues = new string[] { "1 Min Avg", "5 Min Avg", "15 Min Avg" };
                Color[] colorChoices = new Color[] { Color.Gold, Color.DarkCyan, Color.Green };
                theChart = control.AddChart(chartTitleArray, control.ChartCounterDataTable, xValues, yValues, colorChoices);
            }

            //Hide the title in Chart 
            foreach (Title theTitle in theChart.Titles)
            {
                theTitle.Visible = false;
            }

            this.Controls.Add(theChart);
            theChart.Dock = DockStyle.Fill;

            control.OnNewDataArrival += new OverallSummaryControl4.NewDataArrivalHandler(control4_OnNewDataArrival);

        }

        //public SystemMetricsChartDetails(string name, OverallSummaryControl2 control) 
        //{
        //    InitializeComponent();
        //    Chart theChart=new Chart();
        //    if (name.Equals("Core"))
        //    {
        //        theChart = control.AddChart("Core", control.ChartCounterDataTable, new string[] { "Node ID" }, new string[] { "%Avg Core Total" }, new Color[] { Color.Blue }, new string[0]);
        //    }

        //    if (name.Equals("Network"))
        //    {
        //        theChart = control.AddChart("Network", control.ChartCounterDataTable, new string[] { "Node ID", "Node ID", "Node ID", "Node ID", "Node ID", "Node ID" },
        //            new string[] { "Rcv Packets", "Txn Packets", "Rcv Drops", "Rcv Errs", "Txn Drops", "Txn Errs" }, new Color[] { Color.Blue, Color.Purple, Color.Green, Color.Blue, Color.Crimson, Color.Yellow }, new string[]{"", "", "", "", "", ""});
        //    }



        //    this.Controls.Add(theChart);
        //    theChart.Dock = DockStyle.Bottom;
        //}

        //Chart swapChart = AddChart("Swap", _dataTable, new string[] { "Node ID", "Node ID", "Node ID", "Node ID" },
        //    new string[] { "Swap In", "Swap Out", "Major Page Faults", "Minor Page Faults" },
        //    new Color[] { Color.Purple, Color.Green, Color.Blue, Color.Crimson}, new string[] { "Swap In", "Swap Out", "Major Page Faults", "Minor Page Faults" });

        //aRow["Rcv Packets"] = random.Next(60, 80);
        //        aRow["Txn Packets"] = random.Next(40, 60);
        //        aRow["Rcv Drops"] = random.Next(10, 40);
        //        aRow["Rcv Errs"] = random.Next(10, 40);
        //        aRow["Txn Drops"] = random.Next(15, 45);
        //        aRow["Txn Errs"] = random.Next(15, 15);       


    }
}
