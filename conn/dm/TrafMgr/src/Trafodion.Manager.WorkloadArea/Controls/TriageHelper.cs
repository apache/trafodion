#region Copyright info
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
#endregion Copyright info
using System;
using System.Collections;
using System.Data;
using System.Data.Odbc;
using System.Globalization;
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public class TriageHelper
    {
        #region Member variables
        public const String TRACE_SUB_AREA_NAME = "Triage Area";

        private ConnectionDefinition _theConnectionDefinition = null;
        private Connection _theCurrentConnection = null;

        Trafodion.Manager.WorkloadArea.Controls.TriageGridUserControl _theTriageGridUserControl;
        Trafodion.Manager.WorkloadArea.Controls.TriageFilterPropertyGrid _theTriageFilterPropertyGrid;

        public const string TRIAGE_SUB_AREA_NAME = "Triage Area";
        private static String R2DOT3_REPOSITORY_SCHEMA_NAME = "manageability.instance_repository";
        private static String R2DOT3_REPOSITORY_CPU_METRICS_VIEW = "NODE_STATS_V1";
        public static TimeSpan TWELVE_HOURS = TimeSpan.FromHours(12);
        /**
         *  Constant for United States English Culture (en_US locale).
         */
        public const String US_ENGLISH_CULTURE_NAME = "en-US";

        private DataTable m_dataTable = null;
        TriageFilterQueryData m_filterQueryData = null;
        private String _schemaName = R2DOT3_REPOSITORY_SCHEMA_NAME;
        private String _cpuMetricsView = R2DOT3_REPOSITORY_CPU_METRICS_VIEW;

        Trafodion.Manager.WorkloadArea.Controls.WorkloadOption _wsOptions;
        private static NumberFormatInfo _numFormatInfo = null;
        private Hashtable hiddenQIDs_ht = new Hashtable();

        /**
         *  Static variable to hold information about the United States English culture
         *	(en_US locale).
         */
        private static CultureInfo _enUS_cultureInfo = null;

        /*
         *  Repository version information.
         */
        public enum REPOSITORY_PROD_VERSION { R20 = 20, R21 = 21, R22 = 22, R23 = 23, R24 = 24, R25 = 25 };
        #endregion

        #region Constructors
        public TriageHelper()
        {
            try
            {
                _enUS_cultureInfo = new CultureInfo(US_ENGLISH_CULTURE_NAME);

            }
            catch (Exception)
            {
                _enUS_cultureInfo = null;

            }
        }
        #endregion

        #region Properties
        public ConnectionDefinition TheConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set
            {
                _theConnectionDefinition = value;
                m_filterQueryData = new TriageFilterQueryData(this);
            }
        }

        public Trafodion.Manager.WorkloadArea.Controls.TriageGridUserControl TriageGridUserControl
        {
            get { return _theTriageGridUserControl; }
            set { _theTriageGridUserControl = value; }
        }

        public String SchemaName
        {
            get { return this._schemaName; }
        }

        public String getCharsetPrefix(String colName)
        {
            return "";
        }
        public bool IsViewSingleRowPerQuery
        {
            get { return (true); }
        }
        public REPOSITORY_PROD_VERSION Version
        {
            get { return REPOSITORY_PROD_VERSION.R25; }
        }

        public bool IsConnected
        {
            get 
            {
                return true; // (null != _trafodionDBConnection) && this._trafodionDBConnection.IsConnected; 
            }
        }

        public DataTable AggregatedDataTable
        {
            get { return m_dataTable; }
            set { m_dataTable = value; }
        }
        public TriageFilterQueryData FilterQueryData
        {
            get { return m_filterQueryData; }
            set { m_filterQueryData = value; }
        }
        public Trafodion.Manager.WorkloadArea.Controls.WorkloadOption Options
        {
            get 
            {
                return this._wsOptions; 
            }
            set 
            { 
                this._wsOptions = value; 
            }
        }

        public WorkloadOptionModel OptionModel
        {
            get 
            {
                return null; // this.Options.OptionModel; 
            }
            set
            {
                //this.Options.OptionModel = value;
                //if (null != this._parent)
                //    this._parent.updateTimeZoneButtonConfig(value.ShowTimesLocally);
            }
        }

        public Hashtable HiddenQIDs_ht
        {
            get { return hiddenQIDs_ht; }
            set { hiddenQIDs_ht = value; }
        }

        public Trafodion.Manager.WorkloadArea.Controls.TriageFilterPropertyGrid TriageFilterPropertyGrid
        {
            get { return _theTriageFilterPropertyGrid; }
            set { _theTriageFilterPropertyGrid = value; }
        }

        #endregion

        #region Public methods
        public String GetTrafodionSystemTime()
        {
            String sql = "select current_timestamp as Trafodion_System_Time " +
                         "from  (VALUES(1) ) trafodion(\"current_time\")";
            String systemTime = "";
            if (GetConnection())
            {
                try
                {
                    OdbcCommand theQuery = new OdbcCommand(sql);
                    theQuery.Connection = _theCurrentConnection.OpenOdbcConnection;
                    OdbcDataReader reader = Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, "Workload Area", false);
                    while (reader.Read())
                    {
                        systemTime = reader.GetString(0).ToString();
                    }
                }
                finally
                {
                    CloseConnection();
                }
            }
            return systemTime;
        }
        /// <summary>
        /// Gets a new connection object
        /// </summary>
        /// <returns></returns>
        public bool GetConnection()
        {
            if (this._theCurrentConnection == null && this._theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                _theCurrentConnection = new Connection(_theConnectionDefinition);
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Closes a connecttion
        /// </summary>
        public void CloseConnection()
        {
            if (this._theCurrentConnection != null)
            {
                _theCurrentConnection.Close();
                _theCurrentConnection = null;
            }
        }

        public void applyFilterToTriageData(String theFilter)
        {
            ((TriageGridDataDisplayHandler)_theTriageGridUserControl.DataDisplayHandler).applyFilterToTriageData(theFilter);
        }

        //m_parentWorkspace
        public void loadTriageDataTable(DataTable dt, bool clearIGrid)
        {

        }

        //m_parentWorkspace
        public void invokeTriageFromFilter(String sqlFilterClause)
        {
            _theTriageGridUserControl.RefreshWithNewFilter(sqlFilterClause);
        }

        //m_parentWorkspace
        public void clearTheFilters()
        {
            hiddenQIDs_ht.Clear();
            DataTable originalTable = ((DatabaseDataProvider)_theTriageGridUserControl.DataProvider).GetDataTable();

            DataTable currentDataTable = ((TriageGridDataDisplayHandler)_theTriageGridUserControl.DataDisplayHandler).DisplayDataTable;
            if (currentDataTable == this.AggregatedDataTable)
                applyFilterToTriageData(null);
            else
                loadTriageDataTable(this.AggregatedDataTable, true);
        }

        //m_parentWorkspace
        public void resetBackgroundConnection() 
        {
        }

        public void loadFilterQueryDataCompleted()
        {
            //showHideTriageButtons();
        }

        public void HighlightQuery(String queryId, string queryStartTime)
        {
            _theTriageGridUserControl.HighlightQuery(queryId, queryStartTime);
        }

        public bool isHighlightedQueryRunning()
        {
            return _theTriageGridUserControl.isHighlightedQueryRunning();
        }

        public void EnablePropertyGridButtons()
        {
            if (_theTriageFilterPropertyGrid != null)
            {
                _theTriageFilterPropertyGrid.EnableButtons(true);
            }
        }
        /**
         *  <summary>
         *  Returns the number format to use for parsing -- this is the data sent to and from Trafodion.
         *  We handle the number formatting on the clients -- Trafodion sends a number something like :
         *	1,234,567.89  --  but this may be displayed in German as 1.234.567,89  -- ',' is the new '.'!!
         *  Most European locales follow a similar convention.
         *  </summary>
         *
         *  <remarks>Since: NPA1.1</remarks>
         */
        public static NumberFormatInfo getNumberFormatInfoForParsing()
        {
            if (null != _numFormatInfo)
                return _numFormatInfo;

            CultureInfo ci = System.Globalization.CultureInfo.InvariantCulture;
            _numFormatInfo = (NumberFormatInfo)ci.NumberFormat.Clone();
            _numFormatInfo.NumberDecimalSeparator = ".";

            return _numFormatInfo;

        }   /*  End of  getNumberFormatInfoForParsing.  */

        /**
         *  <summary>
         *  Converts the .NET DateTime value into a Trafodion SQL Timestamp (timestamp form that
         *  Trafodion SQL can grok).
         *  </summary>
         *
         *  <param name="clientTimestamp">Local client-side timestamp (Date and Time)</param>
         *  <remarks>Since: NPA1.5</remarks>
         */
        public static String dateToTrafodionSQLTimestamp(DateTime clientTimestamp)
        {
            String trafSQLTimestamp = null;

            if (null == _enUS_cultureInfo)
            {
                trafSQLTimestamp = String.Format("{0:yyyy}", clientTimestamp) + "/" +
                                  String.Format("{0:MM}", clientTimestamp) + "/" +
                                  String.Format("{0:dd}", clientTimestamp) + " " +
                                  String.Format("{0:HH}", clientTimestamp) + ":" +
                                  String.Format("{0:mm}", clientTimestamp) + ":" +
                                  String.Format("{0:ss}", clientTimestamp);
            }
            else
                trafSQLTimestamp = clientTimestamp.ToString("yyyy/MM/dd HH:mm:ss", _enUS_cultureInfo);

            return trafSQLTimestamp;

        }  /*  End of  dateToTrafodionSQLTimestamp.  */

        /**
         *  <summary>
         *  Returns a string containing the time difference between the specified start and 
         *  end time. The elapsed time string is formated using the locale specific time seperator.
         *  </summary>
         *
         *  <param name="startTime">Starting Time</param>
         *  <param name="endTime">Ending Time</param>
         *  <remarks>Since: NPA1.5</remarks>
         */
        public static String getFormattedElapsedTime(DateTime startTime, DateTime endTime)
        {
            String retValue = "";

            try
            {
                TimeSpan runTime = endTime.Subtract(startTime);
                retValue = getFormattedElapsedTime(runTime);

            }
            catch (Exception e)
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "getFormattedElapsedTime error = " + e.Message);
            }

            return retValue;

        }   /*  End of  getFormattedElapsedTime.  */

        /**
		 *  <summary>
		 *  Returns a string formated using the locale specific time seperator.
		 *  </summary>
		 *
		 *  <param name="runTime">TimeSpan value</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
        public static String getFormattedElapsedTime(TimeSpan runTime)
        {
            String retValue = "";

            try
            {
                if (runTime.Ticks < 0)
                    runTime = new TimeSpan(0);


                String timeSeparator = getLocaleTimeSeparator();

                int numHours = runTime.Hours + (runTime.Days * 24);

                retValue = String.Format("{0:00}", numHours) + timeSeparator +
                            String.Format("{0:00}", runTime.Minutes) + timeSeparator +
                            String.Format("{0:00}", runTime.Seconds);

            }
            catch (Exception e)
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "getFormattedElapsedTime(TimeSpan) error = " + e.Message);
            }

            return retValue;

        }   /*  End of  getFormattedElapsedTime.  */
        /**
         *  <summary>
         *  Returns the time separator to use for the current locale. Normally ":" but for
         *  a Danish locale this could be '.'. 
         *  </summary>
         *
         *  <remarks>Since: NPA1.1</remarks>
         */
        public static String getLocaleTimeSeparator()
        {
            String timeSeparator = ":";
            try
            {
                timeSeparator = CultureInfo.CurrentCulture.DateTimeFormat.TimeSeparator;

            }
            catch (Exception)
            {
            }

            return timeSeparator;

        }  /*  End of  getLocaleTimeSeparator  method.  */

        /**
         *  <summary>
         *  Returns the number format for the current locale.
         *  </summary>
         *
         *  <param name="numDecimalDigits">Precision for number of digits in the decimal place</param>
         *  <remarks>Since: NPA1.1</remarks>
         */
        public static String getNumberFormatForCurrentLocale(int numDecimalDigits)
        {
            return getLocaleNumberFormat(numDecimalDigits, false);

        }  /*  End of  getNumberFormatForCurrentLocale  method.  */
        /**
         *  <summary>
         *  Returns the number format for the current locale zero padded if so desired.
         *  </summary>
         *
         *  <param name="numDecimalDigits">Precision for number of digits in the decimal place</param>
         *  <param name="zeroPad">Whether or not to zero-pad</param>
         *  <remarks>Since: NPA1.1</remarks>
         */
        public static String getLocaleNumberFormat(int numDecimalDigits, bool zeroPad)
        {
            return "{0:N" + numDecimalDigits + "}";

        }  /*  End of  getLocaleNumberFormat  method.  */

        public static void saveQueriesToScriptFile(String queryTextAndInfo)
        {
            SaveFileDialog saveDialog = new SaveFileDialog();
            saveDialog.AddExtension = true;
            saveDialog.CreatePrompt = false;
            saveDialog.DefaultExt = "sql";
            saveDialog.OverwritePrompt = true;

            saveDialog.FileName = DateTime.Now.ToString("TrafodionManager-MMM dd yyyy - hh_mm_ss tt") + ".sql";

            DialogResult result = saveDialog.ShowDialog();
            if (DialogResult.OK != result)
                return;

            StreamWriter theFile = null;
            try
            {
                theFile = new StreamWriter(saveDialog.FileName);
                theFile.WriteLine(queryTextAndInfo);

            }
            catch (Exception e)
            {
                MessageBox.Show("\nError: Error saving selected queries to script file.\n" +
                                "\t File Name = " + saveDialog.FileName + "\n\n" +
                                "Problem: \t Unable to write information to the specified file. \n\n" +
                                "Solution: \t Please see the error details for recovery information.\n\n" +
                                "Details: \t " + e.Message + "\n\n", "Generate Consolidated SQL Script Error",
                                MessageBoxButtons.OK, MessageBoxIcon.Error);


            }
            finally
            {
                if (null != theFile)
                    theFile.Close();

            }

        }  /*  End of  saveQueriesToScriptFile  method.  */
        #endregion
    }
}
