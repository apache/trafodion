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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Data.Odbc;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class SQLOffenderCanvas : UserControl, ICloneToWindow
	{
		#region Static Members
        static readonly string CPUOffenderConfigName = "CPUOffenderConfig";
        static readonly string SEOffenderConfigName = "SEOffenderConfig";
        static readonly string SLOWOffenderConfigName = "SLOWOffenderConfig";
        private static Size ChildrenWindowSize = new Size(800, 600);
		#endregion

		#region Members
        public delegate void UpdateStatus(Object obj, EventArgs e);

        ConnectionDefinition _theConnectionDefinition;

        String _cpuOffenderTitle = "CPU Offender";
        String _seOffenderTitle = "SE Offender";
        String _slowOffenderTitle = "Slow Workloads";
        SQLOffenderWidget _cpuOffenderWidget;
        SQLOffenderWidget _seOffenderWidget;
        SQLOffenderWidget _slowOffenderWidget;


		#endregion

		public ConnectionDefinition ConnectionDefinition
		{
			get { return _theConnectionDefinition; }
			set
			{
				_theConnectionDefinition = value;

				if (_theConnectionDefinition != null)
				{
                    _cpuOffenderWidget.ConnectionDefinition = _theConnectionDefinition;
                    _seOffenderWidget.ConnectionDefinition = _theConnectionDefinition;
                    _slowOffenderWidget.ConnectionDefinition = _theConnectionDefinition;
                }
			}
		}

		public SQLOffenderCanvas()
		{
			InitializeComponent();
            
		}

        public SQLOffenderCanvas(ConnectionDefinition aConnectionDefinition)
		{
			InitializeComponent();

            _theConnectionDefinition = aConnectionDefinition;
            _cpuOffenderWidget = new CPUOffenderWidget(aConnectionDefinition);
            _seOffenderWidget = new SEOffenderWidget(aConnectionDefinition);
            _slowOffenderWidget = new SlowOffenderWidget(aConnectionDefinition);
            
            GridLayoutManager gridLayoutManager = new GridLayoutManager(3, 1);
            gridLayoutManager.CellSpacing = 4;
            this._widgetCanvas.LayoutManager = gridLayoutManager; 
            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            WidgetContainer m_widgetContainer1 = new WidgetContainer(_widgetCanvas, _cpuOffenderWidget, _cpuOffenderTitle);
            m_widgetContainer1.Name = _cpuOffenderTitle;
            m_widgetContainer1.AllowDelete = false;
            m_widgetContainer1.Moveable = false;
            m_widgetContainer1.Resizable = false;
            this._widgetCanvas.AddWidget(m_widgetContainer1, gridConstraint, -1);

            gridConstraint = new GridConstraint(1, 0, 1, 1);
            WidgetContainer m_widgetContainer2 = new WidgetContainer(_widgetCanvas, _seOffenderWidget, _seOffenderTitle);
            m_widgetContainer2.Name = _seOffenderTitle;
            m_widgetContainer2.AllowDelete = false;
            m_widgetContainer2.Moveable = false;
            m_widgetContainer2.Resizable = false;
            this._widgetCanvas.AddWidget(m_widgetContainer2, gridConstraint, -1);

            gridConstraint = new GridConstraint(2, 0, 1, 1);
            WidgetContainer m_widgetContainer3 = new WidgetContainer(_widgetCanvas, _slowOffenderWidget, _slowOffenderTitle);
            m_widgetContainer3.Name = _slowOffenderTitle;
            m_widgetContainer3.AllowDelete = false;
            m_widgetContainer3.Moveable = false;
            m_widgetContainer3.Resizable = false;
            this._widgetCanvas.AddWidget(m_widgetContainer3, gridConstraint, -1);
            this._widgetCanvas.InitializeCanvas();
            _cpuOffenderWidget.SqlOffenderWidget.DataDisplayControl.DataDisplayHandler = new MySQLOffenderDataHandler(_cpuOffenderWidget);
            _seOffenderWidget.SqlOffenderWidget.DataDisplayControl.DataDisplayHandler = new MySQLOffenderDataHandler(_seOffenderWidget);
        }

        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if(_cpuOffenderWidget != null) _cpuOffenderWidget.Dispose();
                if (_seOffenderWidget != null) _seOffenderWidget.Dispose();
                if (_slowOffenderWidget != null) _slowOffenderWidget.Dispose();
            }
        }

        public Control Clone()
        {
            return new SQLOffenderCanvas(_theConnectionDefinition);
        }

        public string WindowTitle
        {
            get { return Properties.Resources.SQLOffender; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
        }
    }

    public class MySQLOffenderDataHandler : TabularDataDisplayHandler
    {
        private SQLOffenderWidget _sqlOffenderWidget = null;

        public MySQLOffenderDataHandler(SQLOffenderWidget aSqlOffenderWidget)
        {
            _sqlOffenderWidget = aSqlOffenderWidget;
        }

        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable, TrafodionIGrid aDataGrid)
        {
            DataTable newDataTable = new DataTable(); 
            foreach (DataColumn dc in aDataTable.Columns)
            {
                if (dc.ColumnName.Equals("SQL_DIFF_CPU_TIME_MICROSECS") || dc.ColumnName.Equals("SQL_DIFF_SE_CPU_TIME_MICROSECS"))
                {
                    newDataTable.Columns.Add("SQL_DIFF_CPU_TIME_SECS", System.Type.GetType("System.Double"));
                }
                else
                    newDataTable.Columns.Add(dc.ColumnName, dc.DataType);
            }

            foreach (DataRow r in aDataTable.Rows)
            {
                DataRow newDR = newDataTable.NewRow();
                for (int i = 0; i < aDataTable.Columns.Count; i++)
                {
                    string colName = aDataTable.Columns[i].ToString();
                    if (colName.Equals("SQL_DIFF_CPU_TIME_MICROSECS") || colName.Equals("SQL_DIFF_SE_CPU_TIME_MICROSECS"))
                    {
                        newDR[i] = String.Format("{0:#,0.000000}", Double.Parse(r[colName].ToString()) / 1000000);
                    }
                    else
                        newDR[i] = r[i];
                }
                newDataTable.Rows.Add(newDR);
            }
            base.DoPopulate(aConfig, newDataTable, aDataGrid);
        }
    }
}
