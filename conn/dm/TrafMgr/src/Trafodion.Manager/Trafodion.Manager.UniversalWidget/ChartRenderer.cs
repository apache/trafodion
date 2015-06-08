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
using System.Data;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Text;
//using ZedGraph;
namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// Abstract class that is the parent of the different kinds of charts that can be
    /// rendered.
    /// </summary>
    public abstract class ChartRenderer
    {
        public enum ChartTypes { Line, Pie, Candlestick, Bar };
        public abstract void RenderChart(Control aControl, ChartConfig aConfig, DataTable aDataTable);
        //public abstract void RenderChart(GraphPane aGraphPane, UniversalWidgetConfig aConfig, DataProvider aDataProvider);
        public abstract void RenderChart(Control aControl, UniversalWidgetConfig aConfig, DataProvider aDataProvider);

        public abstract ChartTypes ChartRendererType
        {
            get;
        }
    }
}
