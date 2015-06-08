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

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class EventsSymptomDetailControl : UserControl
    {
        static readonly string EventsSymptomDetailPersistenceKey = "EventsSymptomDetailPersistence";

        UniversalWidgetConfig _universalWidgetConfig;
        GenericUniversalWidget _EventsSymptomDetailWidget;
        TrafodionIGrid _EventsSymptomDetailGrid;
        ConnectionDefinition _connectionDefinition;
        Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider _dateFormatProvider = new Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider();
        string _createTime;
        string _creatorHostId;
        string _creatorProcessId;
        string _sequenceNumber;
        string _eventTableName;

        internal static readonly string SYMPTOM_GEN_TS_LCT_COL_NAME = "GEN_TS_LCT";
        internal static readonly string SYMPTOM_COMPONENT_COL_NAME = "COMPONENT_NAME";
        internal static readonly string SYMPTOM_EVENT_ID_COL_NAME = "EVENT_ID";
        internal static readonly string SYMPTOM_SEVERITY_NAME_COL_NAME = "SEVERITY_NAME";
        internal static readonly string SYMPTOM_PROCESS_NAME_COL_NAME = "PROCESS_NAME";
        internal static readonly string SYMPTOM_TEXT_COL_NAME = "TEXT";
        internal static readonly string SYMPTOM_PROCESS_ID_COL_NAME = "PROCESS_ID";
        internal static readonly string SYMPTOM_THREAD_ID_COL_NAME = "THREAD_ID";
        internal static readonly string SYMPTOM_NODE_ID_COL_NAME = "NODE_ID";
        internal static readonly string SYMPTOM_PNID_ID_COL_NAME = "PNID_ID";
        internal static readonly string SYMPTOM_HOST_ID_COL_NAME = "HOST_ID";
        internal static readonly string SYMPTOM_IP_ADDRESS_ID_COL_NAME = "IP_ADDRESS_ID";
        internal static readonly string EVENT_SEQUENCE_NUMBER_COL_NAME = "SEQUENCE_NUMBER";
        internal static readonly string EVENT_TOKENIZED_EVENT_TABLE_COL_NAME = "TOKENIZED_EVENT_TABLE";

        public Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider TheDateTimeFormatProvider
        {
            get { return _dateFormatProvider; }
        }

        public EventsSymptomDetailControl(ConnectionDefinition aConnectionDefinition, DataTable dataTable)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            SetupComponents(dataTable);
        }

        /// <summary>
        /// Sets up the Events Widget
        /// </summary>
        void SetupComponents(DataTable aDataTable)
        {
            _createTime = aDataTable.Rows[0][SYMPTOM_GEN_TS_LCT_COL_NAME].ToString();
            _creatorHostId = aDataTable.Rows[0][SYMPTOM_HOST_ID_COL_NAME].ToString();
            _creatorProcessId = aDataTable.Rows[0][SYMPTOM_PROCESS_ID_COL_NAME].ToString();
            _sequenceNumber = aDataTable.Rows[0][EVENT_SEQUENCE_NUMBER_COL_NAME].ToString();
            _eventTableName = aDataTable.Rows[0][EVENT_TOKENIZED_EVENT_TABLE_COL_NAME].ToString();
            _genTextBox.Text = _createTime;
            _processIDTextBox.Text = _creatorProcessId;
            _componentNameTextBox.Text = aDataTable.Rows[0][SYMPTOM_COMPONENT_COL_NAME].ToString();
            _processNameTextBox.Text = aDataTable.Rows[0][SYMPTOM_PROCESS_NAME_COL_NAME].ToString();
            _hostIDTextBox.Text = _creatorHostId;
            _pnidTextBox.Text = aDataTable.Rows[0][SYMPTOM_PNID_ID_COL_NAME].ToString();
            _severityTextBox.Text = aDataTable.Rows[0][SYMPTOM_SEVERITY_NAME_COL_NAME].ToString();
            _eventTextBox.Text = aDataTable.Rows[0][SYMPTOM_EVENT_ID_COL_NAME].ToString();
            _nodeIDTextBox.Text = aDataTable.Rows[0][SYMPTOM_NODE_ID_COL_NAME].ToString();
            _ipTextBox.Text = aDataTable.Rows[0][SYMPTOM_IP_ADDRESS_ID_COL_NAME].ToString();
            _textTextBox.Text = aDataTable.Rows[0][SYMPTOM_TEXT_COL_NAME].ToString();
            _threadTextBox.Text = aDataTable.Rows[0][SYMPTOM_THREAD_ID_COL_NAME].ToString();

            //Read the options from persistence. 
            //If the options does not exist or there is an error reading the persistence, create
            //a default option
            _universalWidgetConfig = WidgetRegistry.GetConfigFromPersistence(EventsSymptomDetailPersistenceKey);

            if (_universalWidgetConfig == null)
            {
                //Create the Universal widget configuration
                _universalWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _universalWidgetConfig.Name = EventsSymptomDetailPersistenceKey;
                _universalWidgetConfig.DataProviderConfig.TimerPaused = false;
                _universalWidgetConfig.ShowProviderStatus = false;
                _universalWidgetConfig.Title = "Symptom Events";
                _universalWidgetConfig.ShowProperties = false;
                _universalWidgetConfig.ShowToolBar = true;
                _universalWidgetConfig.ShowChart = false;
                _universalWidgetConfig.ShowTimerSetupButton = false;
                _universalWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }

            _universalWidgetConfig.ShowHelpButton = true;
            _universalWidgetConfig.HelpTopic = HelpTopics.EventsSymptomDetail;
            DateTime createDateTime = DateTime.Now;
            DateTime.TryParse(_createTime, out createDateTime);

            DatabaseDataProviderConfig _dbConfig = _universalWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            string sqlText = string.Format("SELECT * FROM MANAGEABILITY.INSTANCE_REPOSITORY.{0} " +
                                "WHERE GEN_TS_LCT = TIMESTAMP '{1}' " +
                                "AND HOST_ID = {2} " +
                                "AND PROCESS_ID = {3} " +
                                "AND SEQUENCE_NUMBER = {4} " +  
                                "FOR READ UNCOMMITTED ACCESS",
                                _eventTableName, createDateTime.ToString("yyyy-MM-dd HH:mm:ss.FFFFFF"),
                                _creatorHostId.Trim(), _creatorProcessId.Trim(), _sequenceNumber.Trim());
            _dbConfig.SQLText = sqlText;
            _dbConfig.CommandTimeout = 0;
            _dbConfig.ConnectionDefinition = _connectionDefinition;

            _universalWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            if (_universalWidgetConfig.DataProviderConfig.ColumnSortObjects == null)
            {
                _universalWidgetConfig.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>();
            }
            //Create the event Symptoms Widget
            _EventsSymptomDetailWidget = new GenericUniversalWidget();
            _EventsSymptomDetailWidget.DataProvider = new DatabaseDataProvider(_dbConfig);
            ((TabularDataDisplayControl)_EventsSymptomDetailWidget.DataDisplayControl).LineCountFormat = "Symptom Events";

            //Set the widget configuration 
            _EventsSymptomDetailWidget.UniversalWidgetConfiguration = _universalWidgetConfig;

            _EventsSymptomDetailWidget.Dock = DockStyle.Fill;
            _EventsSymptomDetailWidget.DataDisplayControl.DataDisplayHandler = new EventsSymptomDetailDataHandler(this);

            //Initialize the events iGrid
            _EventsSymptomDetailGrid = ((TabularDataDisplayControl)_EventsSymptomDetailWidget.DataDisplayControl).DataGrid;
            _EventsSymptomDetailGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);


            // Remove all current contents and add the event widget
            this._symptomsGroupBox.Controls.Clear();
            _symptomsGroupBox.Controls.Add(_EventsSymptomDetailWidget);

            _EventsSymptomDetailWidget.DataProvider.Start();
        }

        private void _closeButton_Click(object sender, EventArgs e)
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }
    }

    public class EventsSymptomDetailDataHandler : TabularDataDisplayHandler
    {
        #region private member variables

        private EventsSymptomDetailControl _eventsSymtomsUserControl;

        #endregion private member variables

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="anEventsSymptomDetailControl"></param>
        public EventsSymptomDetailDataHandler(EventsSymptomDetailControl anEventsSymptomDetailControl)
        {
            _eventsSymtomsUserControl = anEventsSymptomDetailControl;
        }

        #region UniversalWidget DataDisplayHandler methods

        /// <summary>
        /// Populate the events Grid
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
                        column.CellStyle.FormatProvider = _eventsSymtomsUserControl.TheDateTimeFormatProvider;
                        column.CellStyle.FormatString = "{0}";
                    }
                }
                TrafodionIGridUtils.updateIGridColumnHeadings(aDataGrid);
                aDataGrid.UpdateCountControlText(string.Format("There are {0} event symptom records", aDataGrid.Rows.Count));
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
