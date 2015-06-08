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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class ReportSelectorControl : UserControl
    {

        WidgetLinkerObject _theWidgetLinker;
        UniversalWidgetConfig _theConfig;
        List<ReportParameter> reportParams = null;
        ReportParamDialog paramDialog = null;
        int initialWidth = 550;
        int initialHeight = 230;

        private TrafodionIGrid _theLinkedReportGrid = new TrafodionIGrid();

        public delegate void ButtonStatus(object sender, EventArgs e);
        public event ButtonStatus OnButtonStatus;

        public bool IsReportParametersValid
        {
            get { return paramDialog.IsParametersValid; }
        }

        private void FireButtonStatus(EventArgs e)
        {
            if (OnButtonStatus != null)
            {
                OnButtonStatus(this, e);
            }
        }

        public ReportSelectorControl()
        {
            InitializeComponent();
            _theLinkedReportGrid.Dock = DockStyle.Fill;
            _theWidgetMappingGrid.Controls.Add(_theLinkedReportGrid);
        }

        public TrafodionIGrid LinkedReportGrid
        {
            get
            {
                return _theLinkedReportGrid;
            }
        }

        public void PopulateUI(UniversalWidgetConfig aConfig, WidgetLinkerObject aWidgetLinkerObject)
        {
            _theWidgetLinker = aWidgetLinkerObject;
            _theConfig = aConfig;
            populateLinkedReports();
        }

        public AssociatedWidgetConfig GetSelectedReport()
        {
            if (_theLinkedReportGrid.SelectedRows.Count > 0)
            {
                return _theLinkedReportGrid.SelectedRows[0].Cells[2].Value as AssociatedWidgetConfig;
            }
            else
            {
                return null;
            }
            //return _theWidgetMappingListBox.SelectedItem as AssociatedWidgetConfig;
        }

        public List<ReportParameter> GetUpdatedParameters()
        {
            return reportParams;
        }

        public void UpdateParamsFromUserInput()
        {
            paramDialog.UpdateParamsFromUserInput();
        }

        private void populateLinkedReports()
        {
            //this._theWidgetMappingListBox.Items.Clear();
            //foreach (AssociatedWidgetConfig associatedWidget in _theConfig.AssociatedWidgets)
            //{
            //    //UniversalWidgetConfig calledWidgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(associatedWidget.CalledWidgetName);
            //    //this._theWidgetMappingListBox.Items.Add(calledWidgetConfig.Name + "\t" + calledWidgetConfig.ReportPath);
            //    _theWidgetMappingListBox.Items.Add(associatedWidget);
            //}

            //if (this._theWidgetMappingListBox.Items.Count > 0)
            //{
            //    this._theWidgetMappingListBox.SelectedIndex = 0;
            //}

            _theLinkedReportGrid.Clear();

            DataTable dataTable = new DataTable();
            dataTable.Columns.Add("Report Name");
            dataTable.Columns.Add("Report File Name");
            DataColumn widgetConfigColumn = new DataColumn("AssociatedWidgetConfig", typeof(AssociatedWidgetConfig));
            dataTable.Columns.Add(widgetConfigColumn);

            foreach (AssociatedWidgetConfig associatedWidget in _theConfig.AssociatedWidgets)
            {
                UniversalWidgetConfig widgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(associatedWidget.CalledWidgetID);
                if (widgetConfig != null)
                {
                    object[] dataRow = { widgetConfig.Name, widgetConfig.ReportPath + "\\" + widgetConfig.ReportFileName, associatedWidget };
                    dataTable.Rows.Add(dataRow);
                }
            }

            _theLinkedReportGrid.FillWithData(dataTable);
            _theLinkedReportGrid.ResizeGridColumns(dataTable);

            _theLinkedReportGrid.Cols["Report Name"].CellStyle.ReadOnly = iGBool.False;
            _theLinkedReportGrid.Cols["Report File Name"].CellStyle.ReadOnly = iGBool.False;
            _theLinkedReportGrid.Cols["AssociatedWidgetConfig"].Visible = false;
            _theLinkedReportGrid.AutoResizeCols = true;
            _theLinkedReportGrid.Font = new System.Drawing.Font("Tahoma", 8F);

            //Set selected rows color while losed focus.
            _theLinkedReportGrid.SelCellsBackColorNoFocus = System.Drawing.SystemColors.Highlight;
            _theLinkedReportGrid.SelCellsForeColorNoFocus = System.Drawing.SystemColors.HighlightText;     

            _theLinkedReportGrid.SelectionChanged += new EventHandler(_theLinkedReportGrid_SelectionChanged);

            if (dataTable.Rows.Count > 0)
            {
                _theLinkedReportGrid.Rows[0].Selected = true;
                _theLinkedReportGrid.Focus();
            }

        }

        void _theLinkedReportGrid_SelectionChanged(object sender, EventArgs e)
        {
            TrafodionIGrid linkedReportGrid = sender as TrafodionIGrid;
            AssociatedWidgetConfig associatedConfig = linkedReportGrid.SelectedRows[0].Cells[2].Value as AssociatedWidgetConfig;
            //AssociatedWidgetConfig associatedConfig = _theWidgetMappingListBox.SelectedItem as AssociatedWidgetConfig;
            if (associatedConfig != null)
            {
                if (_theConfig != null)
                {
                    UniversalWidgetConfig calledWidgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(associatedConfig.CalledWidgetID);
                    if ((calledWidgetConfig != null) && (calledWidgetConfig.DataProviderConfig is DatabaseDataProviderConfig))
                    {
                        DatabaseDataProviderConfig dbConfig = calledWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;

                        //Set the called report name based on the user selection
                        _theWidgetLinker.CalledWidget = associatedConfig.CalledWidgetID;

                        //Create a temporary report def object so that we can parse the parameters
                        SimpleReportDefinition reportDef = new SimpleReportDefinition(_theConfig.Name);
                        reportDef.QueryText = dbConfig.SQLText;

                        //Use the report param processor to parse the SQL text to get the list of parameters
                        ReportParameterProcessor paramProcessor = ReportParameterProcessor.Instance;
                        reportParams = paramProcessor.getReportParams(reportDef);

                        //Get the values that will be mapped to the parameters based on the parameter mapping
                        Hashtable mappedParams = _theWidgetLinker.MappedParameters;

                        //Populate the parameters with the value returned by the row selected by the user
                        paramProcessor.populateDefinedParameters(reportParams, mappedParams);

                        //Display the populated parameters to the user
                        DisplayParameters(reportParams);

                        //Display the association reason
                        _theAssociationReason.Text = associatedConfig.AssociationReason;
                    }
                }
            }
            FireButtonStatus(null);
        }

        private void DisplayParameters(List<ReportParameter> parameters)
        {
            _theParameterInputPanel.Controls.Clear();
            paramDialog = new ReportParamDialog();
            paramDialog.populatePanel(_theParameterInputPanel, parameters, true);
            paramDialog.OnParametersChangedImpl += new ReportParamDialog.OnParametersChanged(paramDialog_OnParametersChangedImpl);
            this.ParentForm.ClientSize = new Size(initialWidth, initialHeight + paramDialog.PanelHeight);
        }

        private void paramDialog_OnParametersChangedImpl()
        {
            FireButtonStatus(null);
            //if (OnReportParametersChangedImpl != null)
            //{
            //    OnReportParametersChangedImpl();
            //}
        }

        //private void _theWidgetMappingListBox_SelectedIndexChanged(object sender, EventArgs e)
        //{
        //    AssociatedWidgetConfig associatedConfig = _theWidgetMappingListBox.SelectedItem as AssociatedWidgetConfig;
        //    if (associatedConfig != null)
        //    {
        //        if (_theConfig != null)
        //        {
        //            UniversalWidgetConfig calledWidgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(associatedConfig.CalledWidgetName);
        //            if ((calledWidgetConfig != null) && (calledWidgetConfig.DataProviderConfig is DatabaseDataProviderConfig))
        //            {
        //                DatabaseDataProviderConfig dbConfig = calledWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;

        //                //Set the called report name based on the user selection
        //                _theWidgetLinker.CalledWidget = associatedConfig.CalledWidgetName;

        //                //Create a temporary report def object so that we can parse the parameters
        //                SimpleReportDefinition reportDef = new SimpleReportDefinition(_theConfig.Name);
        //                reportDef.QueryText = dbConfig.SQLText;

        //                //Use the report param processor to parse the SQL text to get the list of parameters
        //                ReportParameterProcessor paramProcessor = ReportParameterProcessor.Instance;
        //                reportParams =  paramProcessor.getReportParams(reportDef);

        //                //Get the values that will be mapped to the parameters based on the parameter mapping
        //                Hashtable mappedParams = _theWidgetLinker.MappedParameters;

        //                //Populate the parameters with the value returned by the row selected by the user
        //                paramProcessor.populateDefinedParameters(reportParams, mappedParams);

        //                //Display the populated parameters to the user
        //                DisplayParameters(reportParams);

        //                //Display the association reason
        //                _theAssociationReason.Text = associatedConfig.AssociationReason;
        //            }
        //        }
        //    }
        //}

        private void _theAddButton_Click(object sender, EventArgs e)
        {
            ConfigureMappingDialog dialog = new ConfigureMappingDialog(_theConfig);
            //dialog.Size = this.ParentForm.Size;
            dialog.ShowDialog();
            if (dialog.SelectedOption == DialogResult.OK)
            {
                AssociatedWidgetConfig associatedConfig = dialog.GetAssociatedWidgetConfig();
                if (associatedConfig != null)
                {
                    iGRow row = _theLinkedReportGrid.Rows.Add();
                    UniversalWidgetConfig widgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(associatedConfig.CalledWidgetID);
                    row.Cells[0].Value = widgetConfig.Name;
                    row.Cells[1].Value = widgetConfig.ReportPath + "\\" + widgetConfig.ReportFileName;
                    row.Cells[2].Value = associatedConfig;

                    row.Selected = true;
                }
            }
            FireButtonStatus(null);
        }

        private void _theDeleteButton_Click(object sender, EventArgs e)
        {
            if (_theLinkedReportGrid.SelectedRows.Count == 0)
            {
                MessageBox.Show("Please select the report link you want to delete.", "Delete Link", MessageBoxButtons.OK);
                return;
            }
            if (_theLinkedReportGrid.SelectedRows.Count > 0)
            {
                AssociatedWidgetConfig associatedWidget = _theLinkedReportGrid.SelectedRows[0].Cells[2].Value as AssociatedWidgetConfig;
                if (associatedWidget != null)
                {
                    DialogResult result = MessageBox.Show("Are you sure you want to delete this report link?", "Delete Link?", MessageBoxButtons.OKCancel, MessageBoxIcon.Question);
                    if (result == DialogResult.OK)
                    {
                        List<AssociatedWidgetConfig> tmp = new List<AssociatedWidgetConfig>();
                        foreach (AssociatedWidgetConfig widgetConfig in _theConfig.AssociatedWidgets)
                        {
                            if (widgetConfig.CalledWidgetID != associatedWidget.CalledWidgetID)
                            {
                                tmp.Add(widgetConfig);
                            }
                        }
                        _theConfig.AssociatedWidgets.Clear();
                        _theConfig.AssociatedWidgets.AddRange(tmp);
                        _theLinkedReportGrid.Rows.RemoveAt(_theLinkedReportGrid.SelectedRowIndexes[0]);

                    }
                }
            }
            if (_theLinkedReportGrid.SelectedRows.Count > 0)
            {
                _theLinkedReportGrid.Rows[0].Selected = true;
            }
            else
            {
                _theAssociationReason.Text = "";
                _theParameterInputPanel.Controls.Clear();
            }

            FireButtonStatus(null);
        }

        private void _theEditButton_Click(object sender, EventArgs e)
        {
            if (_theLinkedReportGrid.SelectedRows.Count == 0)
            {
                MessageBox.Show("Please select linked report which you want to edit.", "Warning message", MessageBoxButtons.OK);
                return;
            }
            AssociatedWidgetConfig associatedConfig = _theLinkedReportGrid.SelectedRows[0].Cells[2].Value as AssociatedWidgetConfig; //_theLinkedReportGrid.Rows[0].Cells[2].Value as AssociatedWidgetConfig;
            if (associatedConfig != null)
            {
                ConfigureMappingDialog dialog = new ConfigureMappingDialog(_theConfig);
                dialog.SetAssociatedWidgetConfigForEdit(associatedConfig);
                //dialog.Size = this.ParentForm.Size;
                dialog.ShowDialog();
                if (dialog.SelectedOption == DialogResult.OK)
                {
                    AssociatedWidgetConfig ac = dialog.GetAssociatedWidgetConfig();// getAssociatedWidgetConfigFromList(associatedConfig);
                    if (ac != null)
                    {
                        int rowIndex = _theLinkedReportGrid.SelectedRowIndexes[0];
                        _theLinkedReportGrid.Rows.RemoveAt(rowIndex);

                        iGRow dataRow = _theLinkedReportGrid.Rows.Insert(rowIndex);
                        UniversalWidgetConfig widgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(ac.CalledWidgetID);
                        dataRow.Cells[0].Value = widgetConfig.Name;
                        dataRow.Cells[1].Value = widgetConfig.ReportPath + "\\" + widgetConfig.ReportFileName;
                        dataRow.Cells[2].Value = ac;

                        dataRow.Selected = true;

                        //AssociatedWidgetConfig temp = getAssociatedWidgetConfigFromList(associatedConfig);
                        //if (temp != null)
                        //{
                        //    _theWidgetMappingListBox.Items.Remove(temp);
                        //}
                        //_theWidgetMappingListBox.Items.Add(ac);
                        //_theWidgetMappingListBox.SelectedItem = ac;
                    }
                }
            }
        }

        //private AssociatedWidgetConfig getAssociatedWidgetConfigFromList(AssociatedWidgetConfig associatedConfig)
        //{
        //    foreach (object item in _theWidgetMappingListBox.Items)
        //    {
        //        AssociatedWidgetConfig ac = item as AssociatedWidgetConfig;
        //        if (ac != null)
        //        {
        //            if (ac.CalledWidgetName.Equals(associatedConfig.CalledWidgetName) && (ac.CallingWidgetName.Equals(associatedConfig.CallingWidgetName)))
        //            {
        //                return ac;
        //            }
        //        }
        //    }
        //    return null;
        //}
    }
}
