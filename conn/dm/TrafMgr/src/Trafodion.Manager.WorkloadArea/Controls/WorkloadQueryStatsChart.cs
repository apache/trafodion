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
using System.Data;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WorkloadQueryStatsChart : UserControl
    {
        public WorkloadQueryStatsChart()
        {
            InitializeComponent();
        }

        public void PopulateChart(DataTable adataSource, string aTitle, string serverNameTitle)
        {
            this.chart1.Titles["HeaderTitle"].Text = aTitle;
            this.chart1.Titles["RunTimeTitle"].Text = serverNameTitle;
           
            this.chart1.DataSource = adataSource;
            this.chart1.DataBind();
            chart1.Series[0].ShadowOffset = 2;

            foreach (DataPoint point in chart1.Series[0].Points)
            {
                if (point.AxisLabel.Equals("Executed"))
                {
                    point.Color = Color.LimeGreen;
                    point.BackSecondaryColor = Color.DarkGreen;
                }
                if (point.AxisLabel.Equals("Completed"))
                {
                    point.Color = Color.DodgerBlue;
                    point.BackSecondaryColor = Color.DarkBlue;
                }
                if (point.AxisLabel.Equals("Canceled"))
                {
                    point.Color = Color.Gold;
                    point.BackSecondaryColor = Color.DarkOrange;
                }
                if (point.AxisLabel.Equals("Rejected"))
                {
                    point.Color = Color.Red;
                    point.BackSecondaryColor = Color.DarkRed;
                }
                if (point.AxisLabel.Equals("Waiting"))
                {
                    point.Color = Color.Orchid;
                    point.BackSecondaryColor = Color.DarkOrchid;
                }
                if (point.AxisLabel.Equals("Holding"))
                {
                    point.Color = Color.DarkKhaki;
                    point.BackSecondaryColor = Color.Olive;
                }
                if (point.AxisLabel.Equals("Suspended"))
                {
                    point.Color = Color.LightSlateGray;
                    point.BackSecondaryColor = Color.DarkGray;
                }
            }
            chart1.Series[0].ToolTip = "#VALX #VALY";
        }
    }

   
}
