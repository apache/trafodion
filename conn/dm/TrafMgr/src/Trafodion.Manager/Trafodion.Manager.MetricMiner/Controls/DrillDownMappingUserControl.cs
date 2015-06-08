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
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.MetricMiner.Controls
{
    public partial class DrillDownMappingUserControl : UserControl
    {
        UniversalWidgetConfig _theConfig;
        private TrafodionIGrid _theLinkedReportGrid = new TrafodionIGrid();

        public delegate void FieldsChanged(object sender, EventArgs args);

        public event FieldsChanged OnFieldsChanged;

        public UniversalWidgetConfig Config
        {
            get { return _theConfig; }
            set
            {
                _theConfig = value;
                populateUI();
                selectFirstRow();
            }
        }

        public DrillDownMappingUserControl()
        {
            InitializeComponent();

            _theLinkedReportGrid.DoubleClickHandler = HandleDoubleClick;
            _theLinkedReportGrid.Dock = DockStyle.Fill;
            _theWidgetMappingPanel.Controls.Add(_theLinkedReportGrid);
        }

        void HandleDoubleClick(int row)
        {
            //override the default double click handler which causes the show row details dialog to pop up
        }

        private void populateUI()
        {
            //_theWidgetMappingListBox.Items.Clear();
            if ((_theConfig != null) && (_theConfig.AssociatedWidgets != null))
            {
                _theLinkedReportGrid.Clear();

                DataTable dataTable = new DataTable();
                dataTable.Columns.Add("Report Name");
                dataTable.Columns.Add("Report File Name");
                DataColumn widgetConfigColumn = new DataColumn("AssociatedWidgetConfig", typeof(AssociatedWidgetConfig));
                dataTable.Columns.Add(widgetConfigColumn);

                foreach (AssociatedWidgetConfig associatedWidget in _theConfig.AssociatedWidgets)
                {
                    UniversalWidgetConfig widgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(associatedWidget.CalledWidgetID);
                    if (widgetConfig == null)
                    {
                        continue;
                    }
                    object[] dataRow = { widgetConfig.Name, widgetConfig.ReportPath + "\\" + widgetConfig.ReportFileName, associatedWidget };
                    dataTable.Rows.Add(dataRow);
                }

                _theLinkedReportGrid.FillWithData(dataTable);
                _theLinkedReportGrid.ResizeGridColumns(dataTable);

                _theLinkedReportGrid.Cols["Report Name"].CellStyle.ReadOnly = iGBool.False;
                _theLinkedReportGrid.Cols["Report File Name"].CellStyle.ReadOnly = iGBool.False;
                _theLinkedReportGrid.Cols["AssociatedWidgetConfig"].Visible = false;
                _theLinkedReportGrid.AutoResizeCols = true;
                _theLinkedReportGrid.Font = new System.Drawing.Font("Tahoma", 8F);

                _theLinkedReportGrid.SelectionChanged += new EventHandler(_theLinkedReportGrid_SelectionChanged);
            }

        }

        void _theLinkedReportGrid_SelectionChanged(object sender, EventArgs e)
        {
            if (_theLinkedReportGrid.SelectedRows.Count > 0)
            {
                AssociatedWidgetConfig assoWidgetConfig = _theLinkedReportGrid.SelectedRows[0].Cells[2].Value as AssociatedWidgetConfig;
                if (assoWidgetConfig != null)
                {
                    showDetails(assoWidgetConfig);
                }
            }
        }

        //private void _theWidgetMappingListBox_SelectedIndexChanged(object sender, EventArgs e)
        //{
        //    if (_theWidgetMappingListBox.SelectedItem != null)
        //    {
        //        showDetails(_theWidgetMappingListBox.SelectedItem as AssociatedWidgetConfig);
        //    }
        //}

        private void showDetails(AssociatedWidgetConfig associatedWidget)
        {
            this._theMappingDetailsGroupBox.Controls.Clear();
            if (associatedWidget != null)
            {
                _theAssociationReason.Text = (associatedWidget.AssociationReason != null) ? associatedWidget.AssociationReason.Trim() : "";

                int row = 0;
                StringBuilder sb = new StringBuilder();
                sb.AppendLine("<html><head></head><body>");
                sb.AppendLine(string.Format("<div style=\"font-family:Tahoma; font-weight:bold; font-size:12px;\">Mapping for drill down Report {0} </div>", associatedWidget.CalledWidgetName));
                sb.AppendLine("<table width=\"100%\">");
                sb.AppendLine(getTableHeader());
                foreach (ParameterMapping paramMapping in associatedWidget.ParameterMappings)
                {
                    sb.AppendLine(getTableRow(paramMapping.ColumnName, paramMapping.ParamName, row++));
                }
                sb.AppendLine("</table></body></html>");


                _theDetailedBrowser = new WebBrowser();
                this._theDetailedBrowser.AllowNavigation = false;
                this._theDetailedBrowser.Dock = System.Windows.Forms.DockStyle.Fill;
                this._theDetailedBrowser.Location = new System.Drawing.Point(0, 0);
                this._theDetailedBrowser.MinimumSize = new System.Drawing.Size(20, 20);
                this._theDetailedBrowser.Name = "_theDetailedBrowser";
                this._theDetailedBrowser.ScrollBarsEnabled = true;
                this._theDetailedBrowser.TabIndex = 1;
                this._theDetailedBrowser.DocumentText = sb.ToString();

                this._theMappingDetailsGroupBox.Controls.Add(_theDetailedBrowser);

            }
        }

        private string getTableHeader()
        {
            return string.Format("<tr><th colspan=\"2\" style=\"font-family:Tahoma; font-weight:bold; font-size:12px; background-color:Silver; \" width=\"100%\" align=\"left\" valign=\"top\" >{0}</th></tr>", "Table column to parameter mappings");
        }

        private string getTableRow(string key, string value, int row)
        {
            value = ((value != null) && (value.Trim().Length > 0)) ? value : "&nbsp;";
            if ((row % 2) == 0)
            {
                return string.Format("<tr style=\"border-bottom:dotted; \"><td style=\"font-family:Tahoma; font-weight:bold; font-size:12px;\" width=\"30%\" align=\"left\" valign=\"top\" >{0}</td><td style=\"font-family:Tahoma; font-size:12px; \"  width=\"70%\" align=\"left\" valign=\"top\" >{1}</td></tr>", key, value);
            }
            else
            {
                return string.Format("<tr style=\"border-bottom:dotted; \"><td style=\"font-family:Tahoma; font-weight:bold; font-size:12px; background-color:WhiteSmoke;\" width=\"30%\" align=\"left\" valign=\"top\" >{0}</td><td style=\"font-family:Tahoma; font-size:12px;  background-color:WhiteSmoke;\"  width=\"70%\" align=\"left\" valign=\"top\" >{1}</td></tr>", key, value);
            }

        }

        private void _theAddButton_Click(object sender, EventArgs e)
        {
            ConfigureMappingDialog dialog = new ConfigureMappingDialog(_theConfig);
            dialog.ShowDialog();
            if (dialog.SelectedOption == DialogResult.OK)
            {
                //in order to check the associate report status, we have to reserve all report before add new one.
                System.Collections.ArrayList tempList = new System.Collections.ArrayList();
                for (int i = 0; i < _theLinkedReportGrid.Rows.Count; i++)
                {
                    AssociatedWidgetConfig aw = _theLinkedReportGrid.Rows[i].Cells["AssociatedWidgetConfig"].Value as AssociatedWidgetConfig;
                    if (aw != null)
                    {
                        tempList.Add(aw.CalledWidgetID);
                    }
                }
                
                populateUI();

                bool statusChanged = false;
                for (int i = 0; i < _theLinkedReportGrid.Rows.Count; i++)
                {
                    AssociatedWidgetConfig aw = _theLinkedReportGrid.Rows[i].Cells["AssociatedWidgetConfig"].Value as AssociatedWidgetConfig;
                    if (aw != null)
                    {
                        if (tempList.Contains(aw.CalledWidgetID))
                        {
                            continue;
                        }
                        else
                        {
                            statusChanged = true;
                            break;
                        }
                    }
                }

                AssociatedWidgetConfig ac = dialog.ConfiguredAssociatedWidgetConfig;
                foreach (iGRow row in _theLinkedReportGrid.Rows)
                {
                    AssociatedWidgetConfig temp = row.Cells[2].Value as AssociatedWidgetConfig;
                    if (ac.CalledWidgetID.Equals(temp.CalledWidgetID))
                    {
                        row.Selected = true;
                        break;
                    }
                }
                if (_theLinkedReportGrid.SelectedRows.Count == 0)
                {
                    _theLinkedReportGrid.Rows[0].Selected = true;
                }

                //if new item is added then we fire event to change save button status.
                if (statusChanged)
                {
                    FireFieldsChanged(new EventArgs());
                }
            }
        }

        private void _theEditButton_Click(object sender, EventArgs e)
        {
            if (_theLinkedReportGrid.SelectedRowIndexes.Count > 0)
            {
                AssociatedWidgetConfig associatedConfig = _theLinkedReportGrid.Rows[_theLinkedReportGrid.SelectedRowIndexes[0]].Cells[2].Value as AssociatedWidgetConfig;
                //AssociatedWidgetConfig associatedConfig = _theWidgetMappingListBox.SelectedItem as AssociatedWidgetConfig;
                if (associatedConfig != null)
                {
                    ConfigureMappingDialog dialog = new ConfigureMappingDialog(_theConfig);
                    dialog.SetAssociatedWidgetConfigForEdit(associatedConfig);
                    //dialog.Size = this.ParentForm.Size;
                    dialog.ShowDialog();
                    if (dialog.SelectedOption == DialogResult.OK)
                    {
                        AssociatedWidgetConfig ac = dialog.GetAssociatedWidgetConfig();
                        if (ac != null)
                        {
                            int rowIndex = _theLinkedReportGrid.SelectedRowIndexes[0];

                            bool statusChanged = false;
                            //reserve data before edit operation, so we can check whether the data has been changed.
                            AssociatedWidgetConfig tempAw = _theLinkedReportGrid.SelectedRows[0].Cells[2].Value as AssociatedWidgetConfig;
                            string tempReason = tempAw.AssociationReason;
                            if (ac.AssociationReason != tempReason)
                            {
                                statusChanged = true;
                            }

                            //get parameter list before edit and compare it with the new parameter list.
                            System.Collections.ArrayList paramList = new System.Collections.ArrayList();
                            foreach (ParameterMapping paramMapping in tempAw.ParameterMappings)
                            {
                                paramList.Add(paramMapping.ColumnName + paramMapping.ParamName);
                            }
                            foreach (ParameterMapping paramMapping in ac.ParameterMappings)
                            {
                                if (paramList.Contains(paramMapping.ColumnName + paramMapping.ParamName))
                                {
                                    continue;
                                }
                                else
                                {
                                    statusChanged = true;
                                }
                            }

                            _theLinkedReportGrid.Rows.RemoveAt(rowIndex);


                            iGRow dataRow = _theLinkedReportGrid.Rows.Add();
                            UniversalWidgetConfig widgetConfig = WidgetRegistry.GetInstance().GetUniversalWidgetConfig(ac.CalledWidgetID);
                            dataRow.Cells[0].Value = widgetConfig.Name;
                            dataRow.Cells[1].Value = widgetConfig.ReportPath + "\\" + widgetConfig.ReportFileName;
                            dataRow.Cells[2].Value = ac;

                            dataRow.Selected = true;

                            //if there are any changes happend then we fire event to change save button status.
                            if (statusChanged)
                            {
                                FireFieldsChanged(new EventArgs());
                            }
                        }
                    }
                }
            }

        }
        //private AssociatedWidgetConfig getAssociatedWidgetConfigFromList(AssociatedWidgetConfig associatedConfig)
        //{
        //    //foreach (object item in _theWidgetMappingListBox.Items)
        //    //{
        //    //    AssociatedWidgetConfig ac = item as AssociatedWidgetConfig;
        //    //    if (ac != null)
        //    //    {
        //    //        if (ac.CalledWidgetName.Equals(associatedConfig.CalledWidgetName) && (ac.CallingWidgetName.Equals(associatedConfig.CallingWidgetName)))
        //    //        {
        //    //            return ac;
        //    //        }
        //    //    }
        //    //}
        //}



        private void _theRemoveButton_Click(object sender, EventArgs e)
        {
            if (_theLinkedReportGrid.SelectedRows.Count > 0)
            {
                AssociatedWidgetConfig associatedWidget = _theLinkedReportGrid.SelectedRows[0].Cells[2].Value as AssociatedWidgetConfig;
                if (associatedWidget != null)
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
                selectFirstRow();
            }
            FireFieldsChanged(new EventArgs());
        }

        private void selectFirstRow()
        {
            if (_theLinkedReportGrid.Rows.Count > 0)
            {
                _theLinkedReportGrid.Rows[0].Selected = true;
            }
            else
            {
                showDetails(null);
            }
        }

        private void _theWidgetMappingListBox_SelectedIndexChanged(object sender, EventArgs e)
        {

        }

        private void FireFieldsChanged(EventArgs e)
        {
            if (OnFieldsChanged != null)
            {
                OnFieldsChanged(this, e);
            }
        }
    }
}
