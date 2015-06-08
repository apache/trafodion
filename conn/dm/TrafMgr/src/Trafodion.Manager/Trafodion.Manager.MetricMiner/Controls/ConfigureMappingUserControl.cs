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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class ConfigureMappingUserControl : UserControl
    {
        private TrafodionIGrid _theParamGrid = new TrafodionIGrid();
        private TrafodionIGrid _theLinkedReportGrid = new TrafodionIGrid();
        private UniversalWidgetConfig _theConfig;

        public delegate void ButtonStatus(object sender, EventArgs e);
        public event ButtonStatus OnButtonStatus;

        //control save button status
        private void FireButtonStatus(EventArgs e)
        {
            if (OnButtonStatus != null)
            {
                OnButtonStatus(this, e);
            }
        }

        public TrafodionIGrid LinkedReportGrid
        {
            get
            {
                return _theLinkedReportGrid;
            }
        }

        public ConfigureMappingUserControl()
        {
            InitializeComponent();

            _theParamGrid.Dock = DockStyle.Fill;
            _theParameterGridPanel.Controls.Add(_theParamGrid);

            _theLinkedReportGrid.DoubleClickHandler = HandleDoubleClick;
            _theLinkedReportGrid.Dock = DockStyle.Fill;
            _theReportNamesGrid.Controls.Add(_theLinkedReportGrid);
        }

        void HandleDoubleClick(int row)
        {
            //override the default double click handler which causes the show row details dialog to pop up
        }

        public UniversalWidgetConfig Config
        {
            get { return _theConfig; }
            set 
            {
                    _theConfig = value;
                    if (value != null)
                    {
                        PopulateUi();
                    }
            }
        }

        public void PopulateUi()
        {
            _theCallingReportLabel.Text = WidgetRegistry.GetWidgetDisplayName(_theConfig.Name);
            Dictionary<string, UniversalWidgetConfig> widgets =  WidgetRegistry.GetInstance().Widgets;
            if (widgets.Count > 0)
            {
                List<object> widgetList = new List<object>();

                DataTable dataTable = new DataTable();
                dataTable.Columns.Add("Report Name");
                dataTable.Columns.Add("Report File Name");
                dataTable.Columns.Add("Report ID");

                foreach (KeyValuePair<string, UniversalWidgetConfig> kv in widgets)
                {
                    if (_theConfig.ReportID == kv.Value.ReportID)
                    {
                        continue;
                    }
                    if (!isAlreadyAssociated(_theConfig, kv.Key))
                    {
                        UniversalWidgetConfig widgetConfig = kv.Value;
                        widgetList.Add(widgetConfig);
                        object[] dataRow = { widgetConfig.Name, widgetConfig.ReportPath + "\\" + widgetConfig.ReportFileName, widgetConfig.ReportID };
                        dataTable.Rows.Add(dataRow);
                    }
                }

                _theLinkedReportGrid.FillWithData(dataTable);
                _theLinkedReportGrid.ResizeGridColumns(dataTable);

                _theLinkedReportGrid.Cols["Report Name"].CellStyle.ReadOnly = iGBool.False;
                _theLinkedReportGrid.Cols["Report File Name"].CellStyle.ReadOnly = iGBool.False;
                _theLinkedReportGrid.Cols["Report ID"].Visible = false;
                _theLinkedReportGrid.AutoResizeCols = true;
                _theLinkedReportGrid.Font = new System.Drawing.Font("Tahoma", 8F);

                _theLinkedReportGrid.SelectionChanged += new EventHandler(_theLinkedReportGrid_SelectionChanged);

                _theLinkedReportGrid.HighlightSelCells = true;
                _theLinkedReportGrid.RowMode = true;
                _theLinkedReportGrid.Focus();

                FireButtonStatus(null);
            }
        }

        void _theLinkedReportGrid_SelectionChanged(object sender, EventArgs e)
        {
            TrafodionIGrid linkedReportGrid = sender as TrafodionIGrid;
            string reportID = linkedReportGrid.SelectedRows[0].Cells[2].Value as string;
            UniversalWidgetConfig config = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(reportID);     
            PopulateGridFromConfig(config);
        }

        /// <summary>
        /// Set the associated widget so that the user can update the column mappings
        /// </summary>
        /// <param name="aAssociatedWidgetConfig"></param>
        public void SetAssociatedWidgetConfigForEdit(AssociatedWidgetConfig aAssociatedWidgetConfig)
        {
            bool isContained = false;
            foreach (iGRow row in _theLinkedReportGrid.Rows)
            {
                string reportID = row.Cells[2].Value as string;
                if (reportID == aAssociatedWidgetConfig.CalledWidgetID)
                {
                    isContained = true;
                    row.Selected = true;
                    break;
                }
            }
            if (!isContained)
            {
                iGRow row = _theLinkedReportGrid.Rows.Add();
                UniversalWidgetConfig widgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(aAssociatedWidgetConfig.CalledWidgetID);
                row.Cells[0].Value = widgetConfig.Name;
                row.Cells[1].Value = widgetConfig.ReportPath + "\\" + widgetConfig.ReportFileName;
                row.Cells[2].Value = widgetConfig.ReportID;
                row.Selected = true;
                _theLinkedReportGrid.Focus();
            }
            _theLinkedReportGrid.Enabled = false;

            //if (!_theReportNamesCombo.Items.Contains(aAssociatedWidgetConfig.CalledWidgetName))
            //{
            //    _theReportNamesCombo.Items.Add(aAssociatedWidgetConfig.CalledWidgetName);
            //}
            //_theReportNamesCombo.SelectedItem = aAssociatedWidgetConfig.CalledWidgetName;
            _theAssociationReason.Text = aAssociatedWidgetConfig.AssociationReason;
            //_theReportNamesCombo.Enabled = false;
            FireButtonStatus(null);
        }

        public AssociatedWidgetConfig getAssociatedWidgetConfig()
        {
            //AssociatedWidgetConfig aAssociatedWidgetConfig = new AssociatedWidgetConfig(_theReportNamesCombo.SelectedItem as String);
            string calledWidgetID = _theLinkedReportGrid.SelectedRows[0].Cells[2].Value as String;
            AssociatedWidgetConfig aAssociatedWidgetConfig = new AssociatedWidgetConfig(calledWidgetID);
            aAssociatedWidgetConfig.CalledWidgetName = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(calledWidgetID).Name;
            aAssociatedWidgetConfig.CallingWidgetName = _theConfig.Name;
            aAssociatedWidgetConfig.CallingWidgetID = _theConfig.ReportID;
            aAssociatedWidgetConfig.AssociationReason = _theAssociationReason.Text;
            aAssociatedWidgetConfig.ParameterMappings = new List<ParameterMapping>();
            foreach (TenTec.Windows.iGridLib.iGRow row in _theParamGrid.Rows)
            {
                if (row.Cells[0].Value != null)
                {
                    ParameterMapping paramMapping = new ParameterMapping();
                    paramMapping.ColumnName = row.Cells[0].Value.ToString();
                    paramMapping.ParamName = row.Cells[1].Value.ToString();
                    aAssociatedWidgetConfig.ParameterMappings.Add(paramMapping);
                }
            }
            return aAssociatedWidgetConfig;
        }

        private bool isAlreadyAssociated(UniversalWidgetConfig aConfig, AssociatedWidgetConfig aAssociatedWidgetConfig)
        {
            return isAlreadyAssociated(aConfig, aAssociatedWidgetConfig.CalledWidgetName);
        }
        private bool isAlreadyAssociated(UniversalWidgetConfig aConfig, string aAssociatedWidgetConfigID)
        {
            if (aConfig.AssociatedWidgets != null)
            {
                foreach (AssociatedWidgetConfig aw in aConfig.AssociatedWidgets)
                {
                    //if (aw.CalledWidgetName.Equals(aAssociatedWidgetConfigName))
                    if (aw.CalledWidgetID.Equals(aAssociatedWidgetConfigID))
                    {
                        return true;
                    }
                }
            }
            return false;
        }



        private void initGrid()
        {
            _theParamGrid.Clear();
            //_theParamGrid.Cols.Add("Column Name");
            //_theParamGrid.Cols.Add("Mapped To Parameter");
        }
        private void PopulateGridFromConfig(UniversalWidgetConfig aConfig)
        {
            initGrid();
            _theParamGrid.ReadOnly = false;
            //AssociatedWidgetConfig assocConfig = _theConfig.GetAssociationByName(_theConfig.Name, aConfig.Name);
            AssociatedWidgetConfig assocConfig = _theConfig.GetAssociationByID(_theConfig.ReportID, aConfig.ReportID);
            if ((aConfig != null) && (aConfig.DataProviderConfig != null) && (aConfig.DataProviderConfig is DatabaseDataProviderConfig))
            {
                DatabaseDataProviderConfig dbProviderConfig = aConfig.DataProviderConfig as DatabaseDataProviderConfig;
                List<ReportParameter> requiredParams = getParametersForQuery(dbProviderConfig.SQLText);
                if (requiredParams.Count > 0)
                {
                    DataTable dataTable = new DataTable();
                    dataTable.Columns.Add("Column Name");
                    dataTable.Columns.Add("Mapped To Parameter");

                    foreach(ReportParameter reportParam in requiredParams)
                    {
                        if (!reportParam.IsInternal)
                        {
                            dataTable.Rows.Add(new Object[] { getColumnNameForParam(assocConfig, reportParam.ParamName), reportParam.ParamName });
                        }
                    }
                    _theParamGrid.FillWithData(dataTable);
                    _theParamGrid.ResizeGridColumns(dataTable);

                    _theParamGrid.Cols["Column Name"].CellStyle.ReadOnly = iGBool.False;
                    _theParamGrid.Cols["Column Name"].Pattern = CreateColumnNameColPattern("Column Name");
                    _theParamGrid.AutoResizeCols = true;
                }

            }
        }

        private string getColumnNameForParam(AssociatedWidgetConfig assocConfig, string paramName)
        {
            if ((assocConfig != null) && (assocConfig.ParameterMappings != null) && (paramName != null))
            {
                foreach (ParameterMapping pm in assocConfig.ParameterMappings)
                {
                    if (pm.ParamName.Equals(paramName))
                    {
                        return pm.ColumnName;
                    }
                }
            }
            return "";

        }

        /// <summary>
        /// Creates the iGrid column pattern for the column Names
        /// Creates the drop down list control for the column names
        /// </summary>
        /// <param name="key"></param>
        /// <returns></returns>
        iGColPattern CreateColumnNameColPattern(string key)
        {
            iGCellStyle statusStyle = new iGCellStyle();
            statusStyle.Flags = iGCellFlags.DisplayText;
            statusStyle.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            statusStyle.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            statusStyle.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            statusStyle.Type = TenTec.Windows.iGridLib.iGCellType.Text;
            statusStyle.ReadOnly = iGBool.False;
            
            if ((_theConfig.DataProviderConfig.ColumnMappings != null) && (_theConfig.DataProviderConfig.ColumnMappings.Count > 0))
            {
                iGDropDownList columnsDropDownList = new iGDropDownList();
                foreach (ColumnMapping col in _theConfig.DataProviderConfig.ColumnMappings)
                {
                    columnsDropDownList.Items.Add(col.InternalName, col.InternalName);
                }
                statusStyle.DropDownControl = columnsDropDownList;
                statusStyle.TypeFlags = TenTec.Windows.iGridLib.iGCellTypeFlags.NoTextEdit;
            }
            iGColPattern colPattern = new iGColPattern();
            colPattern.CellStyle = statusStyle;
            colPattern.Text = key;
            colPattern.Key = key;
            colPattern.MinWidth = 60;
            colPattern.Width = 108;
            colPattern.AllowMoving = false;
            colPattern.AllowSizing = false;
            
            return colPattern;
        }

        private List<ReportParameter> getParametersForQuery(string aQuery)
        {
            SimpleReportDefinition reportDefinition = new SimpleReportDefinition("");
            reportDefinition.QueryText = aQuery;

            ReportParameterProcessor theReportParameterProcessor = ReportParameterProcessor.Instance;
            return theReportParameterProcessor.getReportParams(reportDefinition);
        }

        //private void _theReportNamesCombo_SelectedIndexChanged(object sender, EventArgs e)
        //{
        //    //UniversalWidgetConfig config = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(_theReportNamesCombo.SelectedItem as String);         
        //    UniversalWidgetConfig config = _theReportNamesCombo.SelectedItem as UniversalWidgetConfig;
        //    //UniversalWidgetConfig config = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(_theReportNamesCombo.SelectedItem as String);      
        //    PopulateGridFromConfig(config);
        //}
    }
}
