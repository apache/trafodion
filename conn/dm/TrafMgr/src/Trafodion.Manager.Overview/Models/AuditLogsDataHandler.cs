//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using System.Data;
using System.Drawing;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.OverviewArea.Models
{
    //Class to handle the proper formatting of the diaply of the Audit Logs data
    public class AuditLogsDataHandler : TabularDataDisplayHandler
    {
        #region Fields
        private const int NORMAL_ROW_HEIGHT = 19;
        private const int PREVIEW_ROW_HEIGHT = 38;        
        int limit = 1000;
        private Trafodion.Manager.OverviewArea.Controls.AuditLogUserControl _theAuditLogUserControl;
        private object _locker;
        private ArrayList _Columns = new ArrayList();
        private bool _loadFromStore = false;
        private string _theServerTimeZone = "";
        private int _iGridNormalRowHeight = NORMAL_ROW_HEIGHT;
        private int _iGridRowHeightWithPreview = PREVIEW_ROW_HEIGHT;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid3RowTextColCellStyle;
        private Color m_messagePreviewColor = Color.Gray;
        #endregion Fields


        #region Properties

        public bool LoadFromStore
        {
            get { return _loadFromStore; }
            set { _loadFromStore = value; }
        }

        public ArrayList Columns
        {
            get { return _Columns; }
            set { _Columns = value; }
        }

        /// <summary>
        /// Property: ServerTimeZone - the server's time zone
        /// </summary>
        public string ServerTimeZone
        {
            get { return _theServerTimeZone; }
            set { _theServerTimeZone = value; }
        }

        #endregion Properties


        #region Constructors
        public AuditLogsDataHandler()
        {
            computeRowHeightForMessagePreview();
        }
        public AuditLogsDataHandler(Trafodion.Manager.OverviewArea.Controls.AuditLogUserControl anAuditLogUserControl):this()
        {
            _theAuditLogUserControl = anAuditLogUserControl;
            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_GENERATION_TIME_TS_COL_ID, 
                                        "GEN_TS_LCT", 
                                        AuditLogFilterModel.AUDITLOG_TIME, 
                                        typeof(string), 
                                        GetFormattedTimestamp));
            
            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_AUDITTYPE_COL_ID, 
                                        "audit_type",
                                        AuditLogFilterModel.AUDITTYPE,
                                        typeof(string)));
            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_USER_ID_COL_ID, 
                                        "user_id", 
                                        AuditLogFilterModel.USER_ID, 
                                        typeof(int)));
            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_EXTERNALUSER_NAME_COL_ID, "external_user_name", AuditLogFilterModel.EXTERNALUSERNAME, typeof(string)));
            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_INTERNALUSER_NAME_COL_ID, "internal_user_name", AuditLogFilterModel.INTERNALUSERNAME, typeof(string)));

            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_SESSION_COL_ID, "session_id", AuditLogFilterModel.SESSION_ID, typeof(string)));
            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_TRANSACTION_ID_COL_ID, "transaction_id", AuditLogFilterModel.TRANSACTION_ID, typeof(Int64)));
            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_SQLCODE_COL_ID, "sql_code", AuditLogFilterModel.SQLCODE, typeof(int)));
            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_OUTCOME_COL_ID, "outcome", AuditLogFilterModel.OUTCOME, typeof(string), GetFormattedOutcome));
            _Columns.Add(new ColumnDetails(AuditLogFilterModel.AUDITLOG_MESSAGE_COL_ID, "message", AuditLogFilterModel.MESSAGE, typeof(string), GetFormattedMessage));
        
        }

        #endregion Constructors


        #region Public methods
        //Populates the UI
        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            lock (this)
            {
                populate(aConfig, aDataTable, aDataGrid);
            }
        }

        /// <summary>
        /// Helper method to get the column details given a column name
        /// </summary>
        /// <param name="aName"></param>
        /// <returns></returns>
        public ColumnDetails getColumnDetailsForName(string aName)
        {
            ColumnDetails details = null;
            int idx = _Columns.IndexOf(aName);
            if (idx >= 0)
            {
                details = _Columns[idx] as ColumnDetails;
            }
            return details;
        }



        #endregion Public methods


        #region Private methods


        private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            if ((null == aDataTable) || (null == aConfig) || (null == aDataGrid))
            {
                return;
            }
            DataTable dataTable = new DataTable();
            foreach (DataColumn dc in aDataTable.Columns)
            {
                ColumnDetails details = getColumnDetailsForName(dc.ColumnName);
                if (details != null)
                {
                    dataTable.Columns.Add(details.DisplayName, details.ColumnType);
                }
                else
                {
                    dataTable.Columns.Add(dc.ColumnName, dc.DataType);
                }
            }

            foreach (DataRow dr in aDataTable.Rows)
            {
                DataRow row = dataTable.NewRow();
                foreach (DataColumn dc in aDataTable.Columns)
                {
                    ColumnDetails details = getColumnDetailsForName(dc.ColumnName);
                    if (details != null)
                    {
                        row[details.DisplayName] = details.GetFormattedValueImpl(dr[dc.ColumnName]);
                    }
                    else
                    {
                        row[dc.ColumnName] = dr[dc.ColumnName];
                    }
                }
                dataTable.Rows.Add(row);
            }

#if DEBUG
            Console.WriteLine("Datatable size = " + dataTable.Rows.Count);
#endif

            base.DoPopulate(aConfig, dataTable, aDataGrid);

            showHidePreviewMessage(_theAuditLogUserControl.ShowMessagePreview());

            string gridHeaderText;
            gridHeaderText = string.Format("{0} Audit Logs - Last Refreshed : {1}",
                                            aDataGrid.Rows.Count,
                                            DateTime.Now.ToString());
            aDataGrid.UpdateCountControlText(gridHeaderText);            
            setFormattedFilterMessage();
        }
           
        public void showHidePreviewMessage(bool preview)
        {
            iGrid thegrid = ((TabularDataDisplayControl)_theAuditLogUserControl.AuditLogsWidget.DataDisplayControl).DataGrid;
            if (preview)
            {
                thegrid.RowMode = true;
                thegrid.RowTextVisible = true;
                thegrid.RowTextStartColNear = 0;
            }
            else
            {
                thegrid.RowMode = false;
                thegrid.RowTextVisible = false;
            }

            foreach (iGRow row in thegrid.Rows)
            {
                if (preview)
                {
                    row.Height = PREVIEW_ROW_HEIGHT;
                    string sqlPreview = row.Cells[AuditLogsDataProvider.AUDIT_LOG_MESSAGE].Value.ToString();
                    sqlPreview = sqlPreview.Replace("\r\n", " ");
                    sqlPreview = sqlPreview.Replace("\r", " ");
                    sqlPreview = sqlPreview.Replace("\n", " ");
                    row.RowTextCell.Value = sqlPreview;
                    row.RowTextCell.Style = iGrid3RowTextColCellStyle;
                    row.RowTextCell.ForeColor = m_messagePreviewColor;
                }
                else
                {
                    row.Height = _iGridNormalRowHeight;
                }
            }
        }

        private void computeRowHeightForMessagePreview()
        {
            iGrid3RowTextColCellStyle = new iGCellStyle();
            this.iGrid3RowTextColCellStyle.ContentIndent = new iGIndent(3, 3, 3, 3);
            this.iGrid3RowTextColCellStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
            this.iGrid3RowTextColCellStyle.Font = new System.Drawing.Font("Tahoma", 8.25F, FontStyle.Regular);
            this.iGrid3RowTextColCellStyle.ForeColor = Color.Gray;

            TrafodionIGrid tempGrid = new TrafodionIGrid();

            TenTec.Windows.iGridLib.iGCol sqlTextCol = tempGrid.Cols.Add(AuditLogsDataProvider.AUDIT_LOG_MESSAGE, AuditLogsDataProvider.AUDIT_LOG_MESSAGE);

            TenTec.Windows.iGridLib.iGRow row = tempGrid.Rows.Add();
            iGCell cell = row.Cells[0];

            cell.Value = "SAMPLE TEXT";

            tempGrid.RowTextVisible = true;
            row.RowTextCell.Style = tempGrid.GroupRowLevelStyles[0];

            row.RowTextCell.Value = " First Line #1 " + Environment.NewLine + " Second Line #2 ";
            row.AutoHeight();
            this._iGridRowHeightWithPreview = (int)(1.15 * row.Height);

            tempGrid.RowTextVisible = false;
            row.AutoHeight();
            this._iGridNormalRowHeight = (int)(1.15 * row.Height);
        }
    

        private void setFormattedFilterMessage()
        {
            _theAuditLogUserControl.ShowFilterMessage();
        }



        /// <summary>
        /// returns a formatted audit type
        /// </summary>
        /// <param name="auditType"></param>
        /// <returns></returns>
        private string GetFormattedAuditType(object auditType)
        {
            string ret = "";
            string strAuditType=auditType as string;
            foreach (NameValuePair nv in _theAuditLogUserControl.TheAuditLogDetails.AuditTypes)
            {
                if (string.Equals(strAuditType, nv.Value))
                {
                    ret = nv.Name;
                    break;
                }
            }
            return ret;
        }

        /// <summary>
        /// returns a formatted audit type
        /// </summary>
        /// <param name="auditType"></param>
        /// <returns></returns>
        private string GetFormattedOutcome(object outComeValue)
        {
            string ret = "";
            string outComeString = outComeValue as string;
            foreach (NameValuePair nv in _theAuditLogUserControl.TheAuditLogDetails.Outcomes)
            {
                if (string.Equals(outComeString.Trim(), nv.Value))
                {
                    ret = nv.Name;
                    break;
                }
            }
            return ret;
        }
        
        private string GetFormattedTimestamp(object timestamp)
        {
            return Trafodion.Manager.Framework.Utilities.GetTrafodionSQLLongDateTime((DateTime)timestamp, false);
        }

        /// <summary>
        /// returns a formatted Message field,each comma with a new line
        /// </summary>
        /// <param name="auditType"></param>
        /// <returns></returns>
        private string GetFormattedMessage(object messageValue)
        {
            string ret = "";
            string msg = messageValue as string;
            string newline = "," + System.Environment.NewLine;
            ret = msg.Replace(",", newline);
            return ret;
        }

        #endregion Private methods
    }
}
