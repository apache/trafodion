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

using  System;
using System.Collections;
using System.IO;
using  System.Text;
using  System.Text.RegularExpressions;
using  System.Windows.Forms;
using  System.Xml.Serialization;
using Trafodion.Manager.WorkloadArea.Model;


namespace Trafodion.Manager.WorkloadArea.Controls
{
	public partial class TriageFilterPropertyGrid : UserControl
	{
		#region Constants
		private const String  NPA_FILTER_FILES = "Workload Filters (*.xml)|*.xml|All files (*.*)|*.*";
		#endregion

		#region Members
        private TriageHelper _theTriageHelper = null;
		private TriageFilterInfo m_filterInfo = new TriageFilterInfo();
		private String _lastFilter = "";

		#endregion Members

        #region Constructors
        public TriageFilterPropertyGrid()
		{
			InitializeComponent();
		}

        public TriageFilterPropertyGrid(TriageHelper aTriageHelper)
        {
            InitializeComponent();
            _theTriageHelper = aTriageHelper;
            setupProperties();
        }
        #endregion

        #region Public Methods
        public String LastFilter {
			get { return this._lastFilter; }
		}

		public void setupProperties()
		{
            m_filterInfo.TriageHelper = _theTriageHelper;
            this.filterPropertyGrid.SelectedObject = m_filterInfo;
            //m_filterInfo.EnablePrompts = true;
        }

        public String generateTrafodionWorkloadSQLClause()
        {
            String filter = generateSQLFilter(true, true);
            if (0 < filter.Length)
                filter = "(1 = 1)\n" + filter;

            return filter;
        }

        public void applyFilterToDataTable()
        {
            string sqlFilter = "";

            try
            {
                String theFilter = generateSQLFilter(false);
                if (0 < theFilter.Trim().Length)
                    sqlFilter = "(1 = 1)\n" + theFilter;


            }
            catch (Exception ex)
            {
                Trafodion.Manager.Framework.Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "applyFilterToTriageData error - " + ex.Message);

                return;
            }

            Trafodion.Manager.Framework.Logger.OutputToLog(
                TraceOptions.TraceOption.DEBUG,
                TraceOptions.TraceArea.Monitoring,
                TriageHelper.TRACE_SUB_AREA_NAME,
                "applyFilterToTriageData - " + sqlFilter );

            this._theTriageHelper.applyFilterToTriageData(sqlFilter);
        }

        //public void applyFilterToDataTable(String sqlFilter)
        //{
        //    Trafodion.Manager.Framework.Logger.OutputToLog(
        //        TraceOptions.TraceOption.DEBUG,
        //        TraceOptions.TraceArea.Monitoring,
        //        TriageHelper.TRACE_SUB_AREA_NAME,
        //        "DataTable.Select(" + sqlFilter + ")");

        //    DataTable dt = this._theTriageHelper.AggregatedDataTable;
        //    if (dt != null)
        //    {
        //        if (0 >= sqlFilter.Length)
        //        {
        //            this._theTriageHelper.loadTriageDataTable(dt, true);
        //            return;
        //        }

        //        bool caseSensitiveFlag = dt.CaseSensitive;
        //        DataTable dtFiltered = dt.Clone();
        //        dtFiltered.Rows.Clear();
        //        try
        //        {
        //            dt.CaseSensitive = false;
        //            DataRow[] dataRows = dt.Select(sqlFilter, "START_TIME");
        //            foreach (DataRow row in dataRows)
        //                dtFiltered.ImportRow(row);

        //        }
        //        catch (Exception e)
        //        {
        //            MessageBox.Show("\nError: Failed to apply the workload filter. \n\n" +
        //                            "Problem: \t Error applying the workload filter.\n\n" +
        //                            "Solution: \t Please see error details for recovery information.\n\n" +
        //                            "Details: \t " + e.Message + "\n\n",
        //                            "Workload Filter Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        //            return;

        //        }
        //        finally
        //        {
        //            dt.CaseSensitive = caseSensitiveFlag;
        //            this._theTriageHelper.loadTriageDataTable(dtFiltered, true);
        //        }


        //        if (0 >= dtFiltered.Rows.Count)
        //            MessageBox.Show("\nWarning:  No matching Row(s) were found for the \n" +
        //                            "         \t\t specified workload filter.\n\n",
        //                            "Workload Filter: No Matching Row(s) Found Warning",
        //                            MessageBoxButtons.OK, MessageBoxIcon.Warning);

        //    }

        //}

        public void loadFilterFromFile(String fileName, object ws)
        {
            Stream loadFrom = null;

            try
            {
                loadFrom = File.OpenRead(fileName);
                XmlSerializer xmls = new XmlSerializer(typeof(TriageFilterInfo));
                TriageFilterInfo loadedFilter = (TriageFilterInfo)xmls.Deserialize(loadFrom);
                if (null != loadedFilter)
                    this.m_filterInfo.copyFilterInformation(loadedFilter);

                this.filterPropertyGrid.Refresh();
                if (null != ws)
                    this._theTriageHelper = ws as TriageHelper;

            }
            finally
            {
                if (null != loadFrom)
                    loadFrom.Close();
            }

        }

        /// <summary>
        /// Setting all of the buttons
        /// </summary>
        /// <param name="enable"></param>
        public void EnableButtons(bool enable)
        {
            this.nccWorkloadFilterPropertyGridApplyButton.Enabled = enable;
            this.nccWorkloadFilterPropertyGridFetchButton.Enabled = enable;
            this.nccWorkloadFilterPropertyGridLoadButton.Enabled = enable;
            this.nccWorkloadFilterPropertyGridResetButton.Enabled = enable;
            this.nccWorkloadFilterPropertyGridSaveButton.Enabled = enable;
        }
        #endregion

        #region Private Methods
        private void nccFilterPropertyGridApplyButton_Click(object sender, EventArgs e)
        {
            try
            {
                this.nccWorkloadFilterPropertyGridApplyButton.Enabled = false;
                applyFilterToDataTable();
            }
            finally
            {
                this.nccWorkloadFilterPropertyGridApplyButton.Enabled = true;
            }
        }

        private String generateWhereClause(String colName, String[] values, bool useCharsetPrefix) {
			String valuePrefix = "";

            if (useCharsetPrefix && (null != _theTriageHelper))
                valuePrefix = _theTriageHelper.getCharsetPrefix(colName);

			if ((null == values)  ||  (0 >= values.Length) )
				return "";

            bool isDimension = false;

            if (string.Compare(colName, "APPLICATION_NAME", true) == 0 ||
                string.Compare(colName, "CLIENT_NAME", true) == 0  ||
                string.Compare(colName, "USER_NAME", true) == 0)
            {
                isDimension = true;
            }

			StringBuilder  sb  = new StringBuilder();
			sb.Append("    (TRIM(" + colName + ") = ").Append(valuePrefix).Append("'");
            if (!isDimension)
            {
                sb.Append(values[0].ToString().Trim()).Append("'");
            }
            else
            {
                sb.Append(values[0].ToString().Trim().ToUpper()).Append("'");
            }

			for (int idx = 1; idx < values.Length; idx++) {
				sb.Append(" \n       OR  TRIM(" + colName + ") = ").Append(valuePrefix);
                if (!isDimension)
                {
                    sb.Append("'").Append(values[idx].ToString().Trim()).Append("'");
                }
                else
                {
                    sb.Append("'").Append(values[idx].ToString().Trim().ToUpper()).Append("'");
                }
			}

            //APPLICATION_NAME
            if (string.Compare(colName, "APPLICATION_NAME", true) == 0)
            {
                sb.Replace("TRIM(APPLICATION_NAME)", "TRIM(UPPER(APPLICATION_NAME))");
            }

            //CLIENT_NAME
            if (string.Compare(colName, "CLIENT_NAME", true) == 0)
            {
                sb.Replace("TRIM(CLIENT_NAME)", "TRIM(UPPER(CLIENT_NAME))");
            }

            //USER_NAME
            if (string.Compare(colName, "USER_NAME", true) == 0)
            {
                sb.Replace("TRIM(USER_NAME)", "TRIM(UPPER(USER_NAME))");
            }

			sb.Append(")\n");

			return sb.ToString();

		}

		private DateTime[] getFilterDateTimes(TriageFilterInfo info) {
			DateTime[] retValues = new DateTime[2] {info.From.ToUniversalTime(), info.To.ToUniversalTime() };
			if (FilterTimePeriod.Custom == info.TimePeriod)
				return retValues;

			if (FilterTimePeriod.Any == info.TimePeriod) {
				retValues[0] = DateTime.MinValue;
				retValues[1] = DateTime.MaxValue;
				return retValues;
			}

			DateTime rightNow = DateTime.Now;
			DateTime startTime = new DateTime();

			if ((FilterTimePeriod.Today == info.TimePeriod) ||
				(FilterTimePeriod.Yesterday == info.TimePeriod)) {
				startTime = rightNow.Date;

				if (FilterTimePeriod.Yesterday == info.TimePeriod) {
					rightNow = rightNow.Date;
					startTime = rightNow.Date.Subtract(TimeSpan.FromDays(1.0));
				} else
					rightNow = DateTime.MaxValue;

				retValues[0] = startTime.ToUniversalTime();
				retValues[1] = rightNow.ToUniversalTime();
				return retValues;
			}


			TimeSpan  timeDiff = TimeSpan.FromHours(1.0);
			switch (info.TimePeriod) {
                case FilterTimePeriod.Last_15_Minutes:
                    timeDiff = TimeSpan.FromMinutes(15.0); break;

                case FilterTimePeriod.Last_30_Minutes:
                    timeDiff = TimeSpan.FromMinutes(30.0); break;

                case FilterTimePeriod.Last_Hour:
					timeDiff = TimeSpan.FromHours(1.0); break;

				case FilterTimePeriod.Last_2_Hours:
					timeDiff = TimeSpan.FromHours(2.0); break;

				case FilterTimePeriod.Last_4_Hours:
					timeDiff = TimeSpan.FromHours(4.0); break;

				case FilterTimePeriod.Last_8_Hours:
					timeDiff = TimeSpan.FromHours(8.0); break;

				case FilterTimePeriod.Last_12_Hours:
					timeDiff = TimeSpan.FromHours(12.0); break;

				case FilterTimePeriod.Last_1_Day:
					timeDiff = TimeSpan.FromDays(1.0); break;

				case FilterTimePeriod.Last_2_Days:
					timeDiff = TimeSpan.FromDays(2.0); break;

				case FilterTimePeriod.Last_5_Days:
					timeDiff = TimeSpan.FromDays(5.0); break;

				case FilterTimePeriod.Last_Week:
					timeDiff = TimeSpan.FromDays(7.0); break;

				default:
					timeDiff = TimeSpan.FromHours(1.0);
					break;
			}



			startTime = rightNow.Subtract(timeDiff);
			//retValues[0] = startTime.ToUniversalTime();

            TimeSpan timeSpan = _theTriageHelper.TheConnectionDefinition.ServerGMTOffset;
            retValues[0] = startTime.ToUniversalTime() + timeSpan;
			retValues[1] = DateTime.MaxValue;
			return retValues;
		}

		private String generateCSharpDataTableTextSearchWhereClause(String textLike) {
			if ((null == textLike) || (0 >= textLike.Length))
				return  "";

			Match  kinderbox = Regex.Match(textLike, @"([\*\%_?])");
			if (0 >= kinderbox.Length)
				return "";

			
			Hashtable  lookup_ht = new Hashtable();
			lookup_ht.Add('%', "%");
			lookup_ht.Add('_', "_");
			lookup_ht.Add('*', "*");
			lookup_ht.Add('?', "?");

			StringBuilder whereClausePieces_sb = new StringBuilder();
			
			int startIndex = -1;
			for (int i = 1; i < textLike.Length; i++) {
				if (('\\' != textLike[i - 1])  &&
					lookup_ht.ContainsKey(textLike[i]) ) {
					startIndex++;
					if (i > startIndex) {
						String tidbits = textLike.Substring(startIndex, i - startIndex);
						if (0 < tidbits.Length) {
							whereClausePieces_sb.Append("           AND SQL_TEXT LIKE '%");
							whereClausePieces_sb.Append(tidbits).Append("%' \n");
						}
					}

					startIndex = i;  // skip the wildcard character.
				}
			}


			/*  And add the last part now. */
			startIndex++;
			if (textLike.Length > startIndex) {
				String tidbits = textLike.Substring(startIndex);
				whereClausePieces_sb.Append("           AND SQL_TEXT LIKE '%").Append(tidbits).Append("%' \n");
			}


			StringBuilder  extendedFilter = new StringBuilder("\n");
			extendedFilter.Append("       OR ((1 = 1) \n").Append(whereClausePieces_sb.ToString() );
			extendedFilter.Append("          ) \n");

			return extendedFilter.ToString();
		}

        private String generateMatchingIDClause(String fieldName, String charsetPrefix, String fieldValue) {
            if (null == fieldName) 
                fieldName = "";

            if (null == fieldValue)
                fieldValue = "";

            String matchClause = fieldName + " = " + charsetPrefix + "'" + fieldValue + "'";
            bool hasWildcard = false;

            if (fieldValue.StartsWith("*") ) {
                hasWildcard = true;
                fieldValue = "%" + fieldValue.Substring(1);
            }

            if (fieldValue.EndsWith("*")) {
                hasWildcard = true;
                fieldValue = fieldValue.Substring(0, fieldValue.Length - 1) + "%";
            }


            if (hasWildcard  ||  fieldValue.StartsWith("%")  ||  fieldValue.EndsWith("%") ) {
                String likeClauseEndSuffix = "%";
                if (fieldValue.EndsWith("%") )
                    likeClauseEndSuffix = "";

                matchClause = fieldName + "  LIKE  " + charsetPrefix + "'" + fieldValue + likeClauseEndSuffix + "'";
            }


            return matchClause;
        }

		private String generateSQLFilter(bool generateTrafodionSQLSyntax) {
			return generateSQLFilter(generateTrafodionSQLSyntax, false);
		}

		private String generateSQLFilter(bool generateTrafodionSQLSyntax, bool ignoreTimeDimension) {
			TriageFilterInfo info = (TriageFilterInfo) this.filterPropertyGrid.SelectedObject;
            //NCCRepositoryVersion rv = this._theTriageHelper.RepositoryVersion;

			StringBuilder filterWhereClause = new StringBuilder();

			String charsetPrefix = "";

            String appDimensionName = "APPLICATION_NAME";
            if (generateTrafodionSQLSyntax && (null != _theTriageHelper) && _theTriageHelper.IsViewSingleRowPerQuery)
                appDimensionName = "APPLICATION_NAME";

            String appIDClause = generateWhereClause(appDimensionName, info.Applications,
													 generateTrafodionSQLSyntax);
			if (0 < appIDClause.Length)
				filterWhereClause.Append(" AND ").Append(appIDClause);


            String clientDimensionName = "CLIENT_ID";
            if (generateTrafodionSQLSyntax && (null != _theTriageHelper) && _theTriageHelper.IsViewSingleRowPerQuery)
                clientDimensionName = "CLIENT_NAME";

            String clientIDClause = generateWhereClause(clientDimensionName, info.Clients,
														generateTrafodionSQLSyntax);
			if (0 < clientIDClause.Length)
				filterWhereClause.Append(" AND ").Append(clientIDClause);


			String userNamesClause = generateWhereClause("USER_NAME", info.Users,
														 generateTrafodionSQLSyntax);
			if (0 < userNamesClause.Length)
				filterWhereClause.Append(" AND ").Append(userNamesClause);


			States state = info.State;

			// For Trafodion SQL queries we will filter the state on the client side.
			if (!generateTrafodionSQLSyntax) {
				if (state != States.Any) {
					String qryState = state.ToString();
					if (state == States.Killed_Or_Beyond12HourLimit)
						qryState = "Abnormally Terminated";

					filterWhereClause.Append("AND (STATE = '").Append(qryState).Append("') \n");
				}
			}

			Types type = info.Type;
			if (type != Types.Any) {
				if (!generateTrafodionSQLSyntax) {
					if (type == Types.Other) {
						filterWhereClause.Append("AND   (SQL_TYPE <> 'Select' AND SQL_TYPE <> 'Insert' AND SQL_TYPE <> 'Delete' \n");
						filterWhereClause.Append("          AND SQL_TYPE <> 'Update' AND SQL_TYPE <> 'Call' \n");
						filterWhereClause.Append("          AND SQL_TYPE <> 'Merge'  AND SQL_TYPE <> 'Control' \n");
						filterWhereClause.Append("          AND SQL_TYPE NOT LIKE '").Append(Types.DDL).Append("*' \n");
						filterWhereClause.Append("          AND SQL_TYPE NOT LIKE '").Append(Types.Management).Append("*' )\n");
					}
					else if ((Types.DDL == type) ||  (Types.Management == type) )
						filterWhereClause.Append("AND   (SQL_TYPE LIKE '").Append(type).Append("*') \n");
					else
						filterWhereClause.Append("AND   (SQL_TYPE = '").Append(type).Append("') \n");

				}

			}

			String qryText = info.Text;
			if ((null != qryText)  &&  (0 < qryText.Trim().Length) ) {
				String  sqlTextLike = qryText.Trim();
				String  extendedTextFilter = "";
				String  sqlTextComparer = "(SQL_TEXT";
				String  sqlTextPrefix = "";

				if (generateTrafodionSQLSyntax) {
					String endPrefix = "";

                    if ((null != _theTriageHelper) && !_theTriageHelper.IsViewSingleRowPerQuery)
                        endPrefix = _theTriageHelper.getCharsetPrefix("STATEMENT_STATUS");

					// Replace * and ? wildcards with % and _.
					sqlTextLike = Regex.Replace(sqlTextLike, @"([^\\])?\*", "$1%");
					sqlTextLike = Regex.Replace(sqlTextLike, @"([^\\])?\?", "$1_");

					sqlTextLike = sqlTextLike.ToUpper();

                    if ((null != _theTriageHelper) && _theTriageHelper.IsViewSingleRowPerQuery)
                        sqlTextComparer = "(UPPER(QSTATS.SQL_TEXT)";
                    else
                        sqlTextComparer = "TRIM(STATEMENT_STATUS) = " + endPrefix + "'END'  OR  (UPPER(QSTATS.SQL_TEXT)";

                    if (null != _theTriageHelper)
                        sqlTextPrefix = _theTriageHelper.getCharsetPrefix("SQL_TEXT");

				} else {
					// For client-side processing, remove the leading and trailing % signs.

					while ((1 < sqlTextLike.Length) &&
						   (sqlTextLike.StartsWith("%") || sqlTextLike.StartsWith("*")))
						   sqlTextLike = sqlTextLike.Substring(1);

					while ((1 < sqlTextLike.Length) &&
						   ((!sqlTextLike.EndsWith("\\%")  &&  sqlTextLike.EndsWith("%")) || 
						    (!sqlTextLike.EndsWith("\\*")  &&  sqlTextLike.EndsWith("*"))) )
						   sqlTextLike = sqlTextLike.Substring(0, sqlTextLike.Length - 1);


					extendedTextFilter = generateCSharpDataTableTextSearchWhereClause(sqlTextLike);

					sqlTextLike = Regex.Replace(sqlTextLike, @"([\[\]])", "[$1]");
					sqlTextLike = Regex.Replace(sqlTextLike, @"([\\])([\*\%_\?])", "[$2]");
					sqlTextLike = Regex.Replace(sqlTextLike, @"([^\[])([\*\%_\?])", "$1[$2]");
					
				}


				if (!sqlTextLike.StartsWith("%"))
					sqlTextLike = "%" + sqlTextLike;

				if (!sqlTextLike.EndsWith("%"))
					sqlTextLike = sqlTextLike + "%";

				filterWhereClause.Append("AND   (").Append(sqlTextComparer).Append(" LIKE ");
				filterWhereClause.Append(sqlTextPrefix).Append("'").Append(sqlTextLike).Append("' ");
				filterWhereClause.Append(extendedTextFilter).Append(")) \n");
			}


			String customFilter = info.Custom_Filter;
			if ((null != customFilter) && (0 < customFilter.Trim().Length)) {
				if (generateTrafodionSQLSyntax)
					filterWhereClause.Append("AND    (").Append(info.Custom_Filter).Append(") \n");
			}

			StringBuilder factsWhereClause = new StringBuilder();
			if (0 < info.Rows_Accessed)
				factsWhereClause.Append("AND    (ROWS_ACCESSED >= ").Append(info.Rows_Accessed).Append(") \n");

			if (0 < info.Row_Count)
				factsWhereClause.Append("AND    (ROWS_RETRIEVED >= ").Append(info.Row_Count).Append(") \n");

			if (0 < info.IUD_Count)
				factsWhereClause.Append("AND    (NUM_ROWS_IUD >= ").Append(info.IUD_Count).Append(") \n");

			if (0 < info.Estimated_Cost) {
				String fieldName = "ESTIMATED_COST";

				if (generateTrafodionSQLSyntax) {
					try {
                        if (TriageHelper.REPOSITORY_PROD_VERSION.R23 <= _theTriageHelper.Version)
                            fieldName = "EST_COST";

					} catch (Exception) {
					}

				}

				factsWhereClause.Append("AND    (").Append(fieldName).Append(" >= ");
				factsWhereClause.Append(info.Estimated_Cost).Append(") \n");
			}


			charsetPrefix = "";
            if (generateTrafodionSQLSyntax && (null != _theTriageHelper))
                charsetPrefix = _theTriageHelper.getCharsetPrefix("QUERY_ID");

			String qID = info.Query_ID;
			if ((null != qID) && (0 < qID.Trim().Length)) {
                String qidFieldName = "QUERY_ID";
                if (generateTrafodionSQLSyntax && (null != _theTriageHelper) && _theTriageHelper.IsViewSingleRowPerQuery)
                    qidFieldName = "QSTATS.QUERY_ID";

                String queryIDClause = generateMatchingIDClause(qidFieldName, charsetPrefix, qID.Trim() );
                filterWhereClause.Append("AND    (").Append(queryIDClause).Append(") \n");
			}


			charsetPrefix = "";
            if (generateTrafodionSQLSyntax && (null != _theTriageHelper))
                charsetPrefix = _theTriageHelper.getCharsetPrefix("SESSION_ID");

			String sessionID = info.Session_ID;
			if ((null != sessionID) && (0 < sessionID.Trim().Length)) {
                String sessionIDClause = generateMatchingIDClause("SESSION_ID", charsetPrefix, sessionID.Trim() );
                filterWhereClause.Append("AND    (").Append(sessionIDClause).Append(") \n");
			}


			Errors error = info.Error;
			if (error != Errors.Any) {
				// For Trafodion SQL queries we will filter the state on the client side.
				if (!generateTrafodionSQLSyntax) {
					if (error == Errors.Yes)
						filterWhereClause.Append("AND    (TRIM(ERROR_CODE) <> '' AND TRIM(ERROR_CODE) <> '0') \n");
					else
						filterWhereClause.Append("AND    (TRIM(ERROR_CODE) = ''  OR  TRIM(ERROR_CODE) = '0') \n");
				}
			}

			TimeSpan elapsedTime = info.Elapsed_Time;
			if (elapsedTime.ToString() != "00:00:00") {
				// For Trafodion SQL queries we will filter the state on the client side.
				if (!generateTrafodionSQLSyntax)
					filterWhereClause.Append("AND    (ELAPSED_TIME >= '").Append(elapsedTime.ToString()).Append("') \n");
			}


			//Default value return 1/1/0001 12:00:00 AM
			DateTime[] filterTimes = getFilterDateTimes(info);
			DateTime startTime = filterTimes[0];
			if (!ignoreTimeDimension  &&  (DateTime.MinValue < startTime) ) {
				if (generateTrafodionSQLSyntax) {
                    String trafodionSQLTime =  TriageHelper.dateToTrafodionSQLTimestamp(startTime);
					String fieldName = "QUERY_EVENT_DATETIME_UTC";
                    String convertTimeFunction = ""; 

					try {
                        if ((null != _theTriageHelper) && _theTriageHelper.IsViewSingleRowPerQuery)
                        {
                            fieldName = "QSTATS.EXEC_START_UTC_TS";

                        }
                        else if (TriageHelper.REPOSITORY_PROD_VERSION.R22 >= _theTriageHelper.Version)
                        {
                            fieldName = "ENTRY_ID";
                            convertTimeFunction = "JULIANTIMESTAMP";
                        }

					} catch (Exception) {
					}

					filterWhereClause.Append("AND    (");
					filterWhereClause.Append(fieldName);
					filterWhereClause.Append(" >= ").Append(convertTimeFunction).Append("(TIMESTAMP '");
					filterWhereClause.Append(trafodionSQLTime).Append("') ");
                    //if ((null != _theTriageHelper) && _theTriageHelper.IsViewSingleRowPerQuery)
                    //{
                    //    fieldName = "ENTRY_ID_LCT_TS";
                    //    filterWhereClause.Append("OR    ");
                    //    filterWhereClause.Append(fieldName);
                    //    filterWhereClause.Append(" >= ").Append(convertTimeFunction).Append("(TIMESTAMP '");
                    //    filterWhereClause.Append(trafodionSQLTime).Append("') ");
                    //}
                    filterWhereClause.Append(")\n");
				}
				else
					filterWhereClause.Append("AND    (START_TIME >= '").Append(startTime).Append("') \n");
			}

			DateTime endTime = 	filterTimes[1];
			if (DateTime.MinValue >= endTime)
				endTime = DateTime.MaxValue;

			if (!ignoreTimeDimension  &&  (DateTime.MaxValue > endTime) ) {
				if (generateTrafodionSQLSyntax) {
                    String trafodionSQLTime = TriageHelper.dateToTrafodionSQLTimestamp(endTime);
					String fieldName = "QUERY_EVENT_DATETIME_UTC";
                    String convertTimeFunc = "";

					try {
                        if ((null != _theTriageHelper) && _theTriageHelper.IsViewSingleRowPerQuery)
                        {
                            fieldName = "QSTATS.EXEC_START_UTC_TS";

                        }
                        else if (TriageHelper.REPOSITORY_PROD_VERSION.R22 >= _theTriageHelper.Version)
                        {
                            fieldName = "ENTRY_ID";
                            convertTimeFunc = "JULIANTIMESTAMP";
                        }

					} catch (Exception) {
					}

					filterWhereClause.Append("AND    (");
					filterWhereClause.Append(fieldName);
					filterWhereClause.Append(" <= ").Append(convertTimeFunc).Append("(TIMESTAMP '");
                    filterWhereClause.Append(trafodionSQLTime).Append("') ");
                    //if ((null != _theTriageHelper) && _theTriageHelper.IsViewSingleRowPerQuery)
                    //{
                    //    fieldName = "ENTRY_ID_LCT_TS";
                    //    filterWhereClause.Append("OR    ");
                    //    filterWhereClause.Append(fieldName);
                    //    filterWhereClause.Append(" <= ").Append(convertTimeFunc).Append("(TIMESTAMP '");
                    //    filterWhereClause.Append(trafodionSQLTime).Append("') ");
                    //}
                    filterWhereClause.Append(")\n");
                }
				else
					filterWhereClause.Append("AND    (END_TIME <= '").Append(endTime).Append("') \n");
			}

			int value = startTime.CompareTo(endTime);
			if (!ignoreTimeDimension  &&  (value > 0) && (endTime.Ticks > 0)) {
				MessageBox.Show("\nError: Invalid start time specified.\n\n" +
							    "Problem:  \t The Start time is greater than the End time.\n\n" +
								"Solution: \t Please specify a start time that is less than the end time.\n\n",
								"Invalid Start Time specified", MessageBoxButtons.OK, MessageBoxIcon.Error);
				throw  new Exception("Workload Filter input error. Invalid end time specified (before the start time).");
			}


			filterWhereClause.Append(factsWhereClause.ToString());

			return filterWhereClause.ToString();
		}

		private void fetchAndAddQueriesToTriageSpace() {
			//Fetch data from the repository
			String sqlFilter = "";
			String localFilter = "(1 = 1)";

			try {
				String theFilter = generateSQLFilter(true);
				if (0 < theFilter.Trim().Length) {
					sqlFilter = "(1 = 1)\n" + theFilter;
					try {
						localFilter = "(1 = 1)\n" + generateSQLFilter(false);
					} catch (Exception) {
						localFilter = "(1 = 1)\n";
					}

				} else {
					DialogResult answer =
							MessageBox.Show("\nWarning: No Platform specific filtering criteria was specified \n" +
											"\t\tand this will result in a lot of data being retrieved from the Platform. \n\n" +
											"Do you really wish to continue ?\n\n", "Warning: No Platform Filter specified",
											MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                    if (DialogResult.Yes != answer)
                    {
                        EnableButtons(true);
                        return;
                    }

					sqlFilter = "(1 = 1)\n";
					localFilter = "(1 = 1)\n";
				}

			} catch (Exception e) {
                MessageBox.Show("\nError: Error generating the workload filter. \n\n" +
									"Problem: \t An error occurred generating the workload filter.\n\n" +
									"Solution: \t Please see error details for recovery information.\n\n" +
									"Details: \t " + e.Message + "\n\n",
									"Workload Filter Generation Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				return;
			}

			this._lastFilter = localFilter;
            this._theTriageHelper.invokeTriageFromFilter(sqlFilter);
		}

		private void nccFilterPropertyGridFetchButton_Click(object sender, EventArgs e) {
            //this.nccWorkloadFilterPropertyGridFetchButton.Enabled = false;
            EnableButtons(false);
			fetchAndAddQueriesToTriageSpace();
            //this.nccWorkloadFilterPropertyGridFetchButton.Enabled = true;
		}

		private void nccFilterPropertyGridResetButton_Click(object sender, EventArgs e) {
            if (null != this._theTriageHelper)
            {
                this.nccWorkloadFilterPropertyGridResetButton.Enabled = false;
                this._theTriageHelper.clearTheFilters();
                this._lastFilter = "";
                m_filterInfo.resetFilters();

                this.nccWorkloadFilterPropertyGridResetButton.Enabled = true;

                //  Need to refresh the propertyGrid so that it clears the contents its showing.
                this.filterPropertyGrid.Refresh();
            }
		}

		private void nccFilterPropertyGridSaveButton_Click(object sender, EventArgs e) {

			SaveFileDialog saveDialog = new SaveFileDialog();
			saveDialog.Title = "Save Filter As";
			saveDialog.Filter = NPA_FILTER_FILES;

			DialogResult result = saveDialog.ShowDialog();
			if (DialogResult.OK != result)
				return;



			Stream saveTo = null;

			try {
				saveTo = File.Create(saveDialog.FileName);
				XmlSerializer xmls = new XmlSerializer(typeof(TriageFilterInfo) );
				xmls.Serialize(saveTo, this.m_filterInfo);
			} catch (Exception exc) {
				MessageBox.Show("\nError: Error saving filter information to file \n" + 
								"\t\t " + saveDialog.FileName + "\n\n" +
								"Problem: \t Error saving filter information.\n\n" +
								"Solution: \t Please see the error details for recovery information.\n\n" +
								"Error Details : " + exc.Message + "\n\n", 
								"Save Filter Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
	
			} finally {
				if (null != saveTo)
					saveTo.Close();

			}
		}

		private void nccFilterPropertyGridLoadButton_Click(object sender, EventArgs e) {
			OpenFileDialog loadDialog = new  OpenFileDialog();
			loadDialog.Title = "Load Filter";
			loadDialog.Filter = NPA_FILTER_FILES;

			DialogResult result = loadDialog.ShowDialog();
			if (DialogResult.OK != result)
				return;

			try {
				loadFilterFromFile(loadDialog.FileName, null);
			} catch (Exception exc) {
				MessageBox.Show("\nError: Error loading filter information from file \n" +
								"\t\t " + loadDialog.FileName + "\n\n" +
								"Problem: \t Error loading filter information.\n\n" +
								"Solution: \t Please see the error details for recovery information.\n\n" +
								"Error Details : " + exc.Message + "\n\n",
								"Load Filter Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}

		}

		private void NCCWorkloadFilterPropertyGrid_DragDrop(object sender, DragEventArgs e) {
			try {
				String dndData = (String) e.Data.GetData(System.Windows.Forms.DataFormats.Text).ToString();

				String[] dataValues = dndData.Split(new String[] { "^_^" }, StringSplitOptions.None);
				if (2 <= dataValues.Length)
					m_filterInfo.Query_ID = dataValues[1];

				if (3 <= dataValues.Length)
					m_filterInfo.Session_ID = dataValues[2];

                if (4 <= dataValues.Length)
                    m_filterInfo.Custom_Filter = dataValues[3];

				//  Need to reset the time period if we drag and drop a Query ID and Session ID.
				m_filterInfo.TimePeriod = FilterTimePeriod.Any;
				this.filterPropertyGrid.Refresh();

			} catch (Exception) {
			}

		}

		private void NCCWorkloadFilterPropertyGrid_DragEnter(object sender, DragEventArgs e) {
			if (e.Data.GetDataPresent(DataFormats.Text))
				e.Effect = DragDropEffects.Copy;
			else
				e.Effect = DragDropEffects.None;
        }

        private void filterPropertyGrid_PropertyValueChanged(object s, PropertyValueChangedEventArgs e)
        {
            //if (m_filterInfo.EnablePrompts() &&
            if (e.ChangedItem.Label.Equals("TimePeriod") &&
                e.ChangedItem.Value.ToString() == FilterTimePeriod.Custom.ToString())
            {
                m_filterInfo.GetStartAndEndDateTimes();
            }
        }

        #endregion
    }
}
