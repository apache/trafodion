//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class AlertSymptomEventsControl : UserControl
    {
        static readonly string AlertSymptomEventsPersistenceKey = "AlertSymptomEventsPersistence";

        UniversalWidgetConfig _universalWidgetConfig;
        GenericUniversalWidget _alertSymptomEventsWidget;
        TrafodionIGrid _alertSymptomEventsGrid;
        ConnectionDefinition _connectionDefinition;
        Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider _dateFormatProvider = new Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider();
        string _createTime;
        string _creatorHostId;
        string _creatorProcessId;

        public static readonly string SYMPTOM_GEN_TS_LCT_COL_NAME = "GEN_TS_LCT";
        public static readonly string SYMPTOM_COMPONENT_COL_NAME = "COMPONENT_NAME";
        public static readonly string SYMPTOM_EVENT_ID_COL_NAME = "EVENT_ID";
        public static readonly string SYMPTOM_SEVERITY_NAME_COL_NAME = "SEVERITY_NAME";
        public static readonly string SYMPTOM_PROCESS_NAME_COL_NAME = "PROCESS_NAME";
        public static readonly string SYMPTOM_TEXT_COL_NAME = "TEXT";
        public static readonly string SYMPTOM_PROCESS_ID_COL_NAME = "PROCESS_ID";
        public static readonly string SYMPTOM_THREAD_ID_COL_NAME = "THREAD_ID";
        public static readonly string SYMPTOM_NODE_ID_COL_NAME = "NODE_ID";
        public static readonly string SYMPTOM_PNID_ID_COL_NAME = "PNID_ID";
        public static readonly string SYMPTOM_HOST_ID_COL_NAME = "HOST_ID";
        public static readonly string SYMPTOM_IP_ADDRESS_ID_COL_NAME = "IP_ADDRESS_ID";

        public const string EVENT_VIEW = "PROBLEM_INSTANCE_EVENTMANYTOMANYBRIEF_1";
        public const string EVENT_VIEW_DEPRECATED = "PROBLEM_INSTANCE_SYMPTOMEVENT_2";

        public Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider TheDateTimeFormatProvider
        {
            get { return _dateFormatProvider; }
        }

        private string EventView
        {
            get
            {
                return this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140
                    ? EVENT_VIEW : EVENT_VIEW_DEPRECATED;
            }
        }

        public AlertSymptomEventsControl(ConnectionDefinition aConnectionDefinition, DataTable dataTable)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            SetupComponents(dataTable);
        }

        void MyDispose()
        {

        }

        /// <summary>
        /// Sets up the Alerts Widget
        /// </summary>
        void SetupComponents(DataTable aDataTable)
        {
            _createTime = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_CREATE_TS_LCT_COL_NAME].ToString();
            _creatorHostId = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_CREATOR_HOST_ID_COL_NAME].ToString();
            _creatorProcessId = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_CREATOR_PROCESS_ID_COL_NAME].ToString();
            _createTimeTextBox.Text = _createTime;
            _processIDTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_PROCESS_ID_COL_NAME].ToString();
            _componentNameTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_COMPONENT_NAME_COL_NAME].ToString();
            _processNameTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_PROCESS_NAME_COL_NAME].ToString();
            _closeTimeTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_CLOSE_TS_LCT_COL_NAME].ToString();
            if (aDataTable.Columns.Contains(SystemAlertsUserControl.ALERT_TYPE_DESCRIPTION_COL_NAME))
            {
                _typeDescriptionTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_TYPE_DESCRIPTION_COL_NAME].ToString();
            }
            else
            {
                _typeDescriptionTextBox.Text = "";
            }
            _severityTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_SEVERITY_NAME_COL_NAME].ToString();
            _statusTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_STATUS_COL_NAME].ToString();
            _lastUpdateTimeTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME].ToString();
            _descriptionTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_DESCRIPTION_COL_NAME].ToString();
            _resourceNameTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_RESOURCE_COL_NAME].ToString();
            _resourceTypeTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_RESOURCE_TYPE_COL_NAME].ToString();

            //Read the alert options from persistence. 
            //If the alert options does not exist or there is an error reading the persistence, create
            //a default alerts option
            _universalWidgetConfig = WidgetRegistry.GetConfigFromPersistence(AlertSymptomEventsPersistenceKey);

            if (_universalWidgetConfig == null)
            {
                //Create the Universal widget configuration
                _universalWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _universalWidgetConfig.Name = AlertSymptomEventsPersistenceKey;
                _universalWidgetConfig.DataProviderConfig.TimerPaused = false;
                _universalWidgetConfig.ShowProviderStatus = false;
                _universalWidgetConfig.Title = "Symptom Events";
                _universalWidgetConfig.ShowProperties = false;
                _universalWidgetConfig.ShowToolBar = true;
                _universalWidgetConfig.ShowChart = false;
                _universalWidgetConfig.ShowTimerSetupButton = false;
                _universalWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }
            List<string> defaultVisibleColumns = new List<string>();
            defaultVisibleColumns.Add(SYMPTOM_GEN_TS_LCT_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_COMPONENT_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_EVENT_ID_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_SEVERITY_NAME_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_PROCESS_NAME_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_TEXT_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_PROCESS_ID_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_THREAD_ID_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_NODE_ID_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_PNID_ID_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_HOST_ID_COL_NAME);
            defaultVisibleColumns.Add(SYMPTOM_IP_ADDRESS_ID_COL_NAME);

            _universalWidgetConfig.DataProviderConfig.DefaultVisibleColumnNames = defaultVisibleColumns;
            List<ColumnMapping> columnMappings = new List<ColumnMapping>();
            columnMappings.Add(new ColumnMapping(SYMPTOM_GEN_TS_LCT_COL_NAME, "Gen Time LCT", 120));
            columnMappings.Add(new ColumnMapping(SYMPTOM_COMPONENT_COL_NAME, "Component Name", 120));
            columnMappings.Add(new ColumnMapping(SYMPTOM_EVENT_ID_COL_NAME, "Event ID", 120));
            columnMappings.Add(new ColumnMapping(SYMPTOM_SEVERITY_NAME_COL_NAME, "Severity Name", 120));
            columnMappings.Add(new ColumnMapping(SYMPTOM_PROCESS_NAME_COL_NAME, "Process Name", 120));
            columnMappings.Add(new ColumnMapping(SYMPTOM_TEXT_COL_NAME, "Text", 120));
            columnMappings.Add(new ColumnMapping(SYMPTOM_PROCESS_ID_COL_NAME, "Process ID", 120));
            columnMappings.Add(new ColumnMapping(SYMPTOM_THREAD_ID_COL_NAME, "Thread ID", 120));
            columnMappings.Add(new ColumnMapping(SYMPTOM_NODE_ID_COL_NAME, "Node ID", 120));
            columnMappings.Add(new ColumnMapping(SYMPTOM_PNID_ID_COL_NAME, "Pnid ID", 200));
            columnMappings.Add(new ColumnMapping(SYMPTOM_HOST_ID_COL_NAME, "Host ID", 200));
            columnMappings.Add(new ColumnMapping(SYMPTOM_IP_ADDRESS_ID_COL_NAME, "IP Address ID", 120));
            _universalWidgetConfig.DataProviderConfig.ColumnMappings = columnMappings;

            _universalWidgetConfig.ShowHelpButton = true;
            _universalWidgetConfig.HelpTopic = HelpTopics.AlertSymptomEvents;
            DateTime createDateTime = DateTime.Now;
            DateTime.TryParse(_createTime, out createDateTime);

            DatabaseDataProviderConfig _dbConfig = _universalWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            string sqlText = string.Format("SELECT * FROM MANAGEABILITY.INSTANCE_REPOSITORY.{0} " +
                                "WHERE PROBLEM_CREATE_TS_LCT = TIMESTAMP '{1}' " +
                                "AND PROBLEM_CREATOR_HOST_ID = {2} " +
                                "AND PROBLEM_CREATOR_PROCESS_ID = {3} " +
                                "FOR READ UNCOMMITTED ACCESS", EventView, createDateTime.ToString("yyyy-MM-dd HH:mm:ss.FFFFFF"), _creatorHostId.Trim(), _creatorProcessId.Trim());

            _dbConfig.SQLText = sqlText;
            _dbConfig.CommandTimeout = 0;
            _dbConfig.ConnectionDefinition = _connectionDefinition;

            _universalWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            if (_universalWidgetConfig.DataProviderConfig.ColumnSortObjects == null)
            {
                _universalWidgetConfig.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>();
            }
            //Create the Alert Symptoms Widget
            _alertSymptomEventsWidget = new GenericUniversalWidget();
            _alertSymptomEventsWidget.DataProvider = new DatabaseDataProvider(_dbConfig);
            ((TabularDataDisplayControl)_alertSymptomEventsWidget.DataDisplayControl).LineCountFormat = "Symptom Events";

            //Set the widget configuration 
            _alertSymptomEventsWidget.UniversalWidgetConfiguration = _universalWidgetConfig;

            _alertSymptomEventsWidget.Dock = DockStyle.Fill;
            _alertSymptomEventsWidget.DataDisplayControl.DataDisplayHandler = new SymptomEventsDataHandler(this);

            //Initialize the Alerts iGrid
            _alertSymptomEventsGrid = ((TabularDataDisplayControl)_alertSymptomEventsWidget.DataDisplayControl).DataGrid;
            _alertSymptomEventsGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);


            // Remove all current contents and add the alerts widget
            this._symptomsGroupBox.Controls.Clear();
            _symptomsGroupBox.Controls.Add(_alertSymptomEventsWidget);

            _alertSymptomEventsWidget.DataProvider.Start();
        }

        private void _closeButton_Click(object sender, EventArgs e)
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }
    }

    public class SymptomEventsDataHandler : TabularDataDisplayHandler
    {
        #region private member variables

        private AlertSymptomEventsControl _alertsSymtomsUserControl;

        #endregion private member variables

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aSystemAlertsUserControl"></param>
        public SymptomEventsDataHandler(AlertSymptomEventsControl anAlertSymptomEventsControl)
        {
            _alertsSymtomsUserControl = anAlertSymptomEventsControl;
        }

        #region UniversalWidget DataDisplayHandler methods

        /// <summary>
        /// Populate the Alerts Grid
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                                Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            //Populate the grid with the data table passed
            if (aDataTable != null)
            {
                aDataGrid.Clear();
                TrafodionIGridUtils.PopulateGrid(aConfig, aDataTable, aDataGrid);

                //Enable status and notes columns for editing
                //Other columns remain readonly
                foreach (iGCol column in aDataGrid.Cols)
                {
                    if (column.CellStyle.ValueType == typeof(System.DateTime))
                    {
                        column.CellStyle.FormatProvider = _alertsSymtomsUserControl.TheDateTimeFormatProvider;
                        column.CellStyle.FormatString = "{0}";
                    }
                }
                TrafodionIGridUtils.updateIGridColumnHeadings(aDataGrid);
                aDataGrid.UpdateCountControlText(string.Format("There are {0} symptom events", aDataGrid.Rows.Count));
                aDataGrid.ResizeGridColumns(aDataTable, 7, 20);

                if (aConfig.DataProviderConfig.ColumnSortObjects!= null && aConfig.DataProviderConfig.ColumnSortObjects.Count == 0)
                {
                    if (aDataGrid.Cols.KeyExists("GEN_TS_LCT"))
                    {
                        aConfig.DataProviderConfig.ColumnSortObjects.Add(new ColumnSortObject(aDataGrid.Cols["GEN_TS_LCT"].Index, 0, (int)iGSortOrder.Descending));
                    }
                }
                TabularDataDisplayControl.ApplyColumnAttributes(aDataGrid, aConfig.DataProviderConfig);

                aDataGrid.EndUpdate();
            }
        }
        #endregion UniversalWidget DataDisplayHandler methods
    }
}
