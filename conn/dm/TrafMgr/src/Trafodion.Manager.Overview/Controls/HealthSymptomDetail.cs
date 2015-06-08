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
    public partial class HealthSymptomDetailControl : UserControl
    {
        static readonly string HealthSymptomDetailPersistenceKey = "HealthSymptomDetailPersistence";

        UniversalWidgetConfig _universalWidgetConfig;
        GenericUniversalWidget _HealthSymptomDetailWidget;
        TrafodionIGrid _HealthSymptomDetailGrid;
        ConnectionDefinition _connectionDefinition;
        Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider _dateFormatProvider = new Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider();
        string _createTime;
        string _hostId;
        string _processId;
        string _healthTable;

        internal static readonly string HEALTH_HOST_ID_COL_NAME = "HOST_ID";
        internal static readonly string HEALTH_BASE_TABLE_NAME_COL_NAME = "BASE_TABLE_NAME";
        internal static readonly string HEALTH_LOGICAL_OBJECT_TYPE_COL_NAME = "LOGICAL_OBJECT_TYPE";
        internal static readonly string HEALTH_GEN_TS_LCT_COL_NAME = "GEN_TS_LCT";
        internal static readonly string HEALTH_COMPONENT_NAME_COL_NAME = "COMPONENT_NAME";
        internal static readonly string HEALTH_PUBLICATION_TYPE_COL_NAME = "PUBLICATION_TYPE";
        internal static readonly string HEALTH_CHECK_INTERVAL_SEC_COL_NAME = "CHECK_INTERVAL_SEC";
        internal static readonly string HEALTH_LOGICAL_OBJECT_TYPE_NAME_COL_NAME = "LOGICAL_OBJECT_TYPE_NAME";
        internal static readonly string HEALTH_LOGICAL_OBJECT_NAME_COL_NAME = "LOGICAL_OBJECT_NAME";
        internal static readonly string HEALTH_LOGICAL_OBJECT_PATH_COL_NAME = "LOGICAL_OBJECT_PATH";
        internal static readonly string HEALTH_LOGICAL_OBJECT_QUAL_1_COL_NAME = "LOGICAL_OBJECT_QUAL_1";
        internal static readonly string HEALTH_LOGICAL_OBJECT_QUAL_2_COL_NAME = "LOGICAL_OBJECT_QUAL_2";
        internal static readonly string HEALTH_CURRENT_HEALTH_NAME_COL_NAME = "CURRENT_HEALTH_NAME";
        internal static readonly string HEALTH_PREVIOUS_HEALTH_NAME_COL_NAME = "PREVIOUS_HEALTH_NAME";
        internal static readonly string HEALTH_HEALTH_CHANGE_TS_LCT_COL_NAME = "HEALTH_CHANGE_TS_LCT";

        public Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider TheDateTimeFormatProvider
        {
            get { return _dateFormatProvider; }
        }

        public HealthSymptomDetailControl(ConnectionDefinition aConnectionDefinition, DataTable dataTable)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            SetupComponents(dataTable);
        }

        /// <summary>
        /// Sets up the health Widget
        /// </summary>
        void SetupComponents(DataTable aDataTable)
        {
            _createTime = aDataTable.Rows[0][HEALTH_GEN_TS_LCT_COL_NAME].ToString();
            _healthTable = aDataTable.Rows[0][HEALTH_BASE_TABLE_NAME_COL_NAME].ToString();
            _hostId = aDataTable.Rows[0][HEALTH_HOST_ID_COL_NAME].ToString();
            _componentNameTextBox.Text = aDataTable.Rows[0][HEALTH_COMPONENT_NAME_COL_NAME].ToString();
            _publicationTypeTextBox.Text = aDataTable.Rows[0][HEALTH_PUBLICATION_TYPE_COL_NAME].ToString();
            _genTextBox.Text = _createTime;
            _intervalSecondsTextBox.Text = aDataTable.Rows[0][HEALTH_CHECK_INTERVAL_SEC_COL_NAME].ToString();
            _logicalObjectTypeNameTextBox.Text = aDataTable.Rows[0][HEALTH_LOGICAL_OBJECT_TYPE_NAME_COL_NAME].ToString();
            _logicalObjectNameTextBox.Text = aDataTable.Rows[0][HEALTH_LOGICAL_OBJECT_NAME_COL_NAME].ToString();
            _logicObjectPathTextBox.Text = aDataTable.Rows[0][HEALTH_LOGICAL_OBJECT_PATH_COL_NAME].ToString();
            _currentHealthTextBox.Text = aDataTable.Rows[0][HEALTH_CURRENT_HEALTH_NAME_COL_NAME].ToString();
            _previousHealthTextBox.Text = aDataTable.Rows[0][HEALTH_PREVIOUS_HEALTH_NAME_COL_NAME].ToString();
            _healthLCTTextBox.Text = aDataTable.Rows[0][HEALTH_HEALTH_CHANGE_TS_LCT_COL_NAME].ToString();
            _logical1TextBox.Text = aDataTable.Rows[0][HEALTH_LOGICAL_OBJECT_QUAL_1_COL_NAME].ToString();
            _logical2TextBox.Text = aDataTable.Rows[0][HEALTH_LOGICAL_OBJECT_QUAL_2_COL_NAME].ToString();

            //Read the options from persistence. 
            //If the options does not exist or there is an error reading the persistence, create
            //a default option
            _universalWidgetConfig = WidgetRegistry.GetConfigFromPersistence(HealthSymptomDetailPersistenceKey);

            if (_universalWidgetConfig == null)
            {
                //Create the Universal widget configuration
                _universalWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _universalWidgetConfig.Name = HealthSymptomDetailPersistenceKey;
                _universalWidgetConfig.DataProviderConfig.TimerPaused = false;
                _universalWidgetConfig.ShowProviderStatus = false;
                _universalWidgetConfig.Title = "Health Events";
                _universalWidgetConfig.ShowProperties = false;
                _universalWidgetConfig.ShowToolBar = true;
                _universalWidgetConfig.ShowChart = false;
                _universalWidgetConfig.ShowTimerSetupButton = false;
                _universalWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }
            _universalWidgetConfig.ShowHelpButton = true;
            _universalWidgetConfig.HelpTopic = HelpTopics.HealthSymptomDetail;
            DateTime createDateTime = DateTime.Now;
            DateTime.TryParse(_createTime, out createDateTime);

            DatabaseDataProviderConfig _dbConfig = _universalWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            string sqlText = string.Format("SELECT * FROM MANAGEABILITY.INSTANCE_REPOSITORY.{0} " +
                                "WHERE GEN_TS_LCT = TIMESTAMP '{1}' " +
                                "AND HOST_ID = {2} " +
                                "AND LOGICAL_OBJECT_TYPE = {3} " +
                                "AND LOGICAL_OBJECT_NAME = '{4}' " +
                                "AND LOGICAL_OBJECT_PATH = '{5}' " +
                                "FOR READ UNCOMMITTED ACCESS",
                                _healthTable,
                                createDateTime.ToString("yyyy-MM-dd HH:mm:ss.FFFFFF"), 
                                _hostId.Trim(),
                                aDataTable.Rows[0][HEALTH_LOGICAL_OBJECT_TYPE_COL_NAME],
                                _logicalObjectNameTextBox.Text,
                                _logicObjectPathTextBox.Text);

            _dbConfig.SQLText = sqlText;
            _dbConfig.CommandTimeout = 0;
            _dbConfig.ConnectionDefinition = _connectionDefinition;

            _universalWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            if (_universalWidgetConfig.DataProviderConfig.ColumnSortObjects == null)
            {
                _universalWidgetConfig.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>();
            }
            //Create the health Symptoms Widget
            _HealthSymptomDetailWidget = new GenericUniversalWidget();
            _HealthSymptomDetailWidget.DataProvider = new DatabaseDataProvider(_dbConfig);
            ((TabularDataDisplayControl)_HealthSymptomDetailWidget.DataDisplayControl).LineCountFormat = "Symptom Health";

            //Set the widget configuration 
            _HealthSymptomDetailWidget.UniversalWidgetConfiguration = _universalWidgetConfig;

            _HealthSymptomDetailWidget.Dock = DockStyle.Fill;
            _HealthSymptomDetailWidget.DataDisplayControl.DataDisplayHandler = new HealthSymptomDetailDataHandler(this);

            //Initialize the health iGrid
            _HealthSymptomDetailGrid = ((TabularDataDisplayControl)_HealthSymptomDetailWidget.DataDisplayControl).DataGrid;
            _HealthSymptomDetailGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);


            // Remove all current contents and add the health widget
            this._symptomsGroupBox.Controls.Clear();
            _symptomsGroupBox.Controls.Add(_HealthSymptomDetailWidget);

            _HealthSymptomDetailWidget.DataProvider.Start();
        }

        private void _closeButton_Click(object sender, EventArgs e)
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }
    }

    public class HealthSymptomDetailDataHandler : TabularDataDisplayHandler
    {
        #region private member variables

        private HealthSymptomDetailControl _healthSymtomsUserControl;

        #endregion private member variables

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="healthSymptomDetailControl"></param>
        public HealthSymptomDetailDataHandler(HealthSymptomDetailControl healthSymptomDetailControl)
        {
            _healthSymtomsUserControl = healthSymptomDetailControl;
        }

        #region UniversalWidget DataDisplayHandler methods

        /// <summary>
        /// Populate the health Grid
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
                        column.CellStyle.FormatProvider = _healthSymtomsUserControl.TheDateTimeFormatProvider;
                        column.CellStyle.FormatString = "{0}";
                    }
                }
                TrafodionIGridUtils.updateIGridColumnHeadings(aDataGrid);
                aDataGrid.UpdateCountControlText(string.Format("There are {0} health/state symptom records", aDataGrid.Rows.Count));
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
