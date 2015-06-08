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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.MetricMiner.Controls.Tree
{
    public class AdhocReportNode : MetricMinerWidgetNode
    {
        public AdhocReportNode() : base (WidgetRegistry.GetDefaultDBConfig())
        {
            UniversalWidgetConfig config = Tag as UniversalWidgetConfig;
            config.Name = "Ad-Hoc Report";
            config.Title = "Ad-Hoc Report";
            ((DatabaseDataProviderConfig)config.DataProviderConfig).SQLText = "";
            
            //config.ShowChart = true;
            //config.ShowTable = true;
            //LineChartConfig lineChartConfig = new LineChartConfig();
            //lineChartConfig.Title = "Test";
            //lineChartConfig.XaxisTitle = "Test X";
            //lineChartConfig.YAxisTitle = "Test Y";
            //lineChartConfig.XaxisType = ZedGraph.AxisType.Linear;
            //lineChartConfig.ThemeName = DefaultLineChartTheme.DefaultLineChartThemeName;
            //lineChartConfig.UseLineSymbols = true;
            //lineChartConfig.UseLineSymbolFills = true;
            //lineChartConfig.UseLineFills = true;
            //lineChartConfig.UseChartFill = true;
            
            //List<ChartLine> chartLines = new List<ChartLine>();
            //ChartLine line1 = new ChartLine();
            //line1.PointConfig = new PointConfig();
            //line1.PointConfig.XColName = "aaa";
            //line1.PointConfig.YColName = "bbb";
            //line1.PointConfig.XColIsLinear = true;
            //chartLines.Add(line1);

            //lineChartConfig.ChartLines = chartLines;
            //config.ChartConfig = lineChartConfig;



            Text = config.Name;
            ToolTipText = "Used to run Ad-hoc Reports";
            ImageKey = MetricMinerWidgetsTreeView.MM_ADHOC_REPORT_ICON;
            SelectedImageKey = MetricMinerWidgetsTreeView.MM_ADHOC_REPORT_ICON;

        }
    }
}
