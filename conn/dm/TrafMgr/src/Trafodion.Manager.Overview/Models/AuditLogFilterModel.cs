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
using System.Collections.Generic;
using System.Text;
using System.Collections;
using System.Data;
using System.Data.Odbc;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.Framework.Queries;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Object to save filter values
    /// </summary>
    [Serializable]
    [XmlRoot("AuditLogFilter")]
    [XmlInclude(typeof(NameValuePair))]
    [XmlInclude(typeof(TimeRangeHandler))]
    public class AuditLogFilterModel
    {
        // Define audit log display column id
        public const int AUDITLOG_GENERATION_TIME_TS_COL_ID = 0;
        public const int AUDITLOG_AUDITTYPE_COL_ID = 1;
        public const int AUDITLOG_INTERNALUSER_NAME_COL_ID = 4;
        public const int AUDITLOG_USER_ID_COL_ID = 2;
        public const int AUDITLOG_EXTERNALUSER_NAME_COL_ID = 3;
        public const int AUDITLOG_SESSION_COL_ID = 5; 
        public const int AUDITLOG_TRANSACTION_ID_COL_ID = 6;
        public const int AUDITLOG_OUTCOME_COL_ID = 7;
        public const int AUDITLOG_SQLCODE_COL_ID = 8;
        public const int AUDITLOG_MESSAGE_COL_ID = 9;

        // Define data grid column names
        public const string AUDITLOG_TIME = "Audit Logging Time (server local time)";
        public const string AUDITTYPE = "Audit Type";
        public const string INTERNALUSERNAME = "Database User Name";
        public const string USER_ID = "User ID";
        public const string EXTERNALUSERNAME = "Directory-Service User Name";
        public const string SESSION_ID = "Session ID";
        public const string TRANSACTION_ID = "Transaction ID";
        public const string OUTCOME = "Outcome";
        public const string SQLCODE = "SQL Code";
        public const string MESSAGE = "Message";
        
             

        // Define audit log filter persistence key
        public const string AuditLogFilterPersistenceKey = "AuditLogFilterPersistenceKey";


        #region Member variables
        List<NameValuePair> _theAuditTypeFilters = new List<NameValuePair>();
        string _theUserIds = "";
        string _theExternalUserNames = "";
        string _theInternalUserNames = "";
        string _theSessionIds = "";
        string _theTransactionIds = "";
        string _theSQLCodeIds = "";
        List<NameValuePair> _theOutcomesFilters = new List<NameValuePair>();
        TimeRangeHandler.Range _theTimeRange;
        bool _theCurrentTime = true;
        DateTime _theCustomEndTime;
        DateTime _theCustomStartTime;
        string _theMessaegeFilter;
        bool _theCaseSensitive;
        NameValuePair _theMessageFilterCondition;
        
        [NonSerialized]
        private bool _theAuditTypeAllChecked = false;

        [NonSerialized]
        private bool _theOutcomeAllChecked = false;

        #endregion

        #region Constructors


        [XmlArray("AuditTypeFilters")]
        [XmlArrayItem("AuditTypeFilter")]
        public List<NameValuePair> AuditTypeFilters
        {
            get { return _theAuditTypeFilters; }
            set { _theAuditTypeFilters = value; }
        }
             

        [XmlElement("UserIds")]
        public string UserIds
        {
            get 
            {
                return getFormattedListString(GetIntIds(_theUserIds));
            }
            set { _theUserIds = getFormattedListString(GetIntIds(value)); }
        }
        [XmlElement("ExternalUserNames")]
        public string ExternalUserNames
        {
            get { return getFormattedListString(GetIds(_theExternalUserNames)); }
            set { _theExternalUserNames = getFormattedListString(GetIds(value)); }
        }
        [XmlElement("InternalUserNames")]
        public string InternalUserNames
        {
            get { return getFormattedListString(GetIds(_theInternalUserNames)); }
            set { _theInternalUserNames = getFormattedListString(GetIds(value)); }
        }
       
        [XmlElement("SQLCodeIds")]
        public string SQLCodeIds
        {
            get 
            {
                return getFormattedListString(GetIntIds(_theSQLCodeIds));
            }
            set { _theSQLCodeIds = getFormattedListString(GetIntIds(value)); }
        }

        [XmlArray("Outcomes")]
        [XmlArrayItem("Outcome")]
        public List<NameValuePair> OutcomesFilters
        {
            get { return _theOutcomesFilters; }
            set { _theOutcomesFilters = value; }
        }

         [XmlElement("SessionIds")]
         public string SessionIds
         {
             get
             {
                 return getFormattedListString(GetIds(_theSessionIds));
             }
             set { _theSessionIds = getFormattedListString(GetIds(value)); }
         }

        [XmlElement("TransactionIds")]
        public string TransactionIds
        {
            get
            {
                return getFormattedListString(GetIntIds(_theTransactionIds));
            }
            set { _theTransactionIds = getFormattedListString(GetIntIds(value)); }
        }

        [XmlElement("TimeRange")]
        public TimeRangeHandler.Range TimeRange
        {
            get { return _theTimeRange; }
            set { _theTimeRange = value; }
        }
        [XmlElement("CurrentTime")]
        public bool CurrentTime
        {
            get { return _theCurrentTime; }
            set { _theCurrentTime = value; }
        }
        [XmlElement("TheEndTime")]
        public DateTime TheEndTime
        {
            get { return _theCustomEndTime; }
            set { _theCustomEndTime = value; }
        }
        [XmlElement("TheStartTime")]
        public DateTime TheStartTime
        {
            get { return _theCustomStartTime; }
            set { _theCustomStartTime = value; }
        }
        [XmlElement("MessaegeFilter")]
        public string MessaegeFilter
        {
            get { return _theMessaegeFilter; }
            set { _theMessaegeFilter = value; }
        }
        [XmlElement("CaseSensitive")]
        public bool CaseSensitive
        {
            get { return _theCaseSensitive; }
            set { _theCaseSensitive = value; }
        }
        [XmlElement("MessageFilterCondition")]
        public NameValuePair MessageFilterCondition
        {
            get { return _theMessageFilterCondition; }
            set { _theMessageFilterCondition = value; }
        }

        [XmlIgnore()]
        public bool AuditTypeAllChecked
        {
            get { return _theAuditTypeAllChecked; }
            set { _theAuditTypeAllChecked = value; }
        }


        #endregion

        #region Public Methods

        //Compares the passed AuditLogFilterModel to the current AuditLogFilterModel
        public override bool Equals(Object obj)
        {
            AuditLogFilterModel newModel = obj as AuditLogFilterModel;
            if (newModel != null)
            {
                bool ret = true;

                //has _theAuditTypes changed
                ret = ret && AreAuditTypeFiltersSame(newModel);
         
                //has _theUserIds changed
                ret = ret && AreUserIdsSame(newModel);

                //has _theExternalNames changed
                ret = ret && AreExternalUserNamesSame(newModel);

                //has _theInternalNames changed
                ret = ret && AreInternalUserNamesSame(newModel);

                //has _theSessionIds changed
                ret = ret && AreSessionIdsSame(newModel);

                //has _theTransactionIds changed
                ret = ret && AreTransactionIdsSame(newModel);

                //has _theSQLCodeIds changed
                ret = ret && AreSQLCodeIdsSame(newModel);

                //has _theOutComes changed
                ret = ret && AreOutcomeFiltersSame(newModel);

                //has _theTimeRange changed
                ret = ret && _theTimeRange.Equals(newModel.TimeRange);

                //has _theCurrentTime flag changed
                ret = ret && (_theCurrentTime == newModel.CurrentTime);

                //has _theStartTime  changed
                ret = ret && _theCustomEndTime.Equals(newModel.TheEndTime);

                //has _theEndTime  changed
                ret = ret && _theCustomStartTime.Equals(newModel.TheStartTime);

                //has _theMessaegeFilter  changed
                ret = ret && _theMessaegeFilter.Equals(newModel.MessaegeFilter);
                
                //has _theCaseSensitive flag changed
                ret = ret && (_theCaseSensitive == newModel.CaseSensitive);

                //has _theMessageFilterCondition changed
                ret = ret && (_theMessageFilterCondition.Equals(newModel.MessageFilterCondition));

                return ret;
            }
            return false;
        }



        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        public static List<string> GetIntIds(string idsStr)
        {
            List<string> logIds = new List<string>();
            if (idsStr != null)
            {
                string[] ids = idsStr.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
                foreach (string id in ids)
                {
                    try
                    {
                        Int64.Parse(id.Trim());
                        logIds.Add(id.Trim());
                    }
                    catch (Exception)
                    {
                    }
                }
            }
            return logIds;
        }

        public static List<string> GetIds(string idsStr)
        {
            List<string> logIds = new List<string>();
            if (idsStr != null)
            {
                string[] ids = idsStr.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
                foreach (string id in ids)
                {

                    logIds.Add(id.Trim());
                }
            }
            return logIds;
        }

        /// <summary>
        /// Get filter string for regular Audit Log viewer. This string is to be used as the predicates for the SQL text
        /// fetching the Audit Log table in the repository. 
        /// </summary>
        /// <returns></returns>
        public string GetFilterSQL()
        {
            string ret = "";

            //get user id filter string
            List<string> userIds = GetIds(_theUserIds);
            if (userIds.Count > 0)
            {
                String EventIdFilter = string.Format("{0} IN (", "user_id");
                for (int i = 0; i < userIds.Count; i++)
                {
                    EventIdFilter = EventIdFilter + userIds[i] + ((i < (userIds.Count - 1)) ? ", " : "");
                }
                EventIdFilter = EventIdFilter + ")";
                ret = addToCondition(ret, EventIdFilter);
            }

            //get external user name filter string
            List<string> externalUserNames = GetIds(_theExternalUserNames);
            if (externalUserNames.Count > 0)
            {
                String ExternalUserNameFilter = string.Format("{0} IN (", "EXTERNAL_USER_NAME");
                for (int i = 0; i < externalUserNames.Count; i++)
                {
                    string processName = externalUserNames[i].Trim().Replace("'", "''");
                    ExternalUserNameFilter = ExternalUserNameFilter + string.Format("'{0}'", processName) + ((i < (externalUserNames.Count - 1)) ? ", " : "");
                }
                ExternalUserNameFilter = ExternalUserNameFilter + ")";
                ret = addToCondition(ret, ExternalUserNameFilter);
            }

            //get internal user name filter string
            List<string> internalUserNames = GetIds(_theInternalUserNames);
            if (internalUserNames.Count > 0)
            {
                String InternalUserNameFilter = string.Format("{0} IN (", "INTERNAL_USER_NAME");
                for (int i = 0; i < internalUserNames.Count; i++)
                {
                    string processName = internalUserNames[i].Trim().Replace("'", "''");
                    InternalUserNameFilter = InternalUserNameFilter + string.Format("'{0}'", processName) + ((i < (internalUserNames.Count - 1)) ? ", " : "");
                }
                InternalUserNameFilter = InternalUserNameFilter + ")";
                ret = addToCondition(ret, InternalUserNameFilter);
            }


            //get session id filter string
            List<string> sessionIds = GetIds(_theSessionIds);
            if (sessionIds.Count > 0)
            {
                String SessionIDFilter = string.Format("{0} IN (", "SESSION_ID");
                for (int i = 0; i < sessionIds.Count; i++)
                {
                    SessionIDFilter = SessionIDFilter + string.Format("'{0}'", sessionIds[i].Trim()) + ((i < (sessionIds.Count - 1)) ? ", " : "");
                }
                SessionIDFilter = SessionIDFilter + ")";
                ret = addToCondition(ret, SessionIDFilter);
            }
 
            //get transaction id filter string
            List<string> transactionIds = GetIds(_theTransactionIds);
            if (transactionIds.Count > 0)
            {
                String TransactionIdFilter = string.Format("{0} IN (", "TRANSACTION_ID");
                for (int i = 0; i < transactionIds.Count; i++)
                {
                    TransactionIdFilter = TransactionIdFilter + transactionIds[i] + ((i < (transactionIds.Count - 1)) ? ", " : "");
                }
                TransactionIdFilter = TransactionIdFilter + ")";
                ret = addToCondition(ret, TransactionIdFilter);
            }

            //get SQL Code id filter string
            List<string> SQLCodeIds = GetIds(_theSQLCodeIds);
            if (SQLCodeIds.Count > 0)
            {
                String SQLCodeIdFilter = string.Format("{0} IN (", "SQL_CODE");
                for (int i = 0; i < SQLCodeIds.Count; i++)
                {
                    SQLCodeIdFilter = SQLCodeIdFilter + SQLCodeIds[i] + ((i < (SQLCodeIds.Count - 1)) ? ", " : "");
                }
                SQLCodeIdFilter = SQLCodeIdFilter + ")";
                ret = addToCondition(ret, SQLCodeIdFilter);
            }

            //get out come filter
            if (_theOutcomesFilters.Count > 0)
            {
                String OutcomeFilter = string.Format("{0} IN (", "OUTCOME");
                for (int i = 0; i < _theOutcomesFilters.Count; i++)
                {
                    OutcomeFilter = OutcomeFilter + string.Format("'{0}'",_theOutcomesFilters[i].Value) + ((i < (_theOutcomesFilters.Count - 1)) ? ", " : "");
                }
                OutcomeFilter = OutcomeFilter + ")";
                ret = addToCondition(ret, OutcomeFilter);
            }  

            //get time range filter
            if (_theTimeRange != TimeRangeHandler.Range.AllTimes)
            {
                String TimeFilter = "GEN_TS_LCT";
                if (_theTimeRange == TimeRangeHandler.Range.CustomRange)
                {
                    if (_theCurrentTime)
                    {
                        TimeFilter = string.Format("{0} >= {1}", TimeFilter, getDateForQuery(_theCustomStartTime));
                    }
                    else
                    {
                        TimeFilter = string.Format("({0} >= {1}) AND ({2} <= {3}) ", TimeFilter, getDateForQuery(_theCustomStartTime), TimeFilter, getDateForQuery(_theCustomEndTime));
                    }
                }
                else
                {
                    TimeFilter = string.Format("{0} {1}", TimeFilter, TimeRangeInput.ComputeSQLTimeRange(new TimeRangeHandler(_theTimeRange).GetTimeRangeString(_theTimeRange)));
                }
                ret = addToCondition(ret, TimeFilter);
            }

            //get message filter
            if (_theMessaegeFilter.Trim().Length > 0)
            {
                String MessageFilter = (!_theCaseSensitive) ? "UPSHIFT(MESSAGE)" : "MESSAGE";
                String filterMessage = (!_theCaseSensitive) ? _theMessaegeFilter.Trim().ToUpper() : _theMessaegeFilter.Trim();
                filterMessage = filterMessage.Replace("'", "''");
                if (_theMessageFilterCondition.Value.Equals("Contains"))
                {
                    MessageFilter = string.Format("{0} LIKE '%{1}%'", MessageFilter, filterMessage);
                }
                else if (_theMessageFilterCondition.Value.Equals("Equals"))
                {
                    MessageFilter = string.Format("{0} = '{1}'", MessageFilter, filterMessage);
                }
                else if (_theMessageFilterCondition.Value.Equals("Does Not Contain"))
                {
                    MessageFilter = string.Format("{0} NOT LIKE '%{1}%'", MessageFilter, filterMessage);
                }
                ret = addToCondition(ret, MessageFilter);
            }


            return ret;

        }


        public string GetFormattedFilterString(AuditLogsDataHandler anAuditLogsDataHandler)
        {
            string ret = "";

            if (anAuditLogsDataHandler == null)
                return "Filter string is ...";

            //get auditType filter string            
            if (_theAuditTypeFilters.Count > 0 && _theAuditTypeFilters.Count !=6)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("AUDIT_TYPE");
                if (cd != null)
                {
                    String AuditTypeFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < _theAuditTypeFilters.Count; i++)
                    {
                        AuditTypeFilter = AuditTypeFilter + string.Format("'{0}'",_theAuditTypeFilters[i].Name) + ((i < (_theAuditTypeFilters.Count - 1)) ? ", " : "");
                    }
                    AuditTypeFilter = AuditTypeFilter + ")";
                    ret = addToCondition(ret, AuditTypeFilter);
                }
            }

            //get user id filter string
            List<string> userIds = GetIds(_theUserIds);
            if (userIds.Count > 0)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("USER_ID");
                if (cd != null)
                {
                    String UserIdFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < userIds.Count; i++)
                    {
                        UserIdFilter = UserIdFilter + userIds[i] + ((i < (userIds.Count - 1)) ? ", " : "");
                    }
                    UserIdFilter = UserIdFilter + ")";
                    ret = addToCondition(ret, UserIdFilter);
                }
            }

            //get external user name filter string
            List<string> externalUserNames = GetIds(_theExternalUserNames);
            if (externalUserNames.Count > 0)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("EXTERNAL_USER_NAME");
                if (cd != null)
                {

                    String ExternalNameFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < externalUserNames.Count; i++)
                    {
                        ExternalNameFilter = ExternalNameFilter + string.Format("'{0}'", externalUserNames[i].Trim()) + ((i < (externalUserNames.Count - 1)) ? ", " : "");
                    }
                    ExternalNameFilter = ExternalNameFilter + ")";
                    ret = addToCondition(ret, ExternalNameFilter);
                }
            }

            //get internal user name filter string
            List<string> internalUserNames = GetIds(_theInternalUserNames);
            if (internalUserNames.Count > 0)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("INTERNAL_USER_NAME");
                if (cd != null)
                {

                    String InternalNameFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < internalUserNames.Count; i++)
                    {
                        InternalNameFilter = InternalNameFilter + string.Format("'{0}'", internalUserNames[i].Trim()) + ((i < (internalUserNames.Count - 1)) ? ", " : "");
                    }
                    InternalNameFilter = InternalNameFilter + ")";
                    ret = addToCondition(ret, InternalNameFilter);
                }
            }

            //get transaction id filter string
            List<string> transactionId = GetIds(_theTransactionIds);
            if (transactionId.Count > 0)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("TRANSACTION_ID");
                if (cd != null)
                {
                    String TransactionIdFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < transactionId.Count; i++)
                    {
                        TransactionIdFilter = TransactionIdFilter + transactionId[i] + ((i < (transactionId.Count - 1)) ? ", " : "");
                    }
                    TransactionIdFilter = TransactionIdFilter + ")";
                    ret = addToCondition(ret, TransactionIdFilter);
                }
            }

            //get session id filter string
            List<string> sessionId = GetIds(_theSessionIds);
            if (sessionId.Count > 0)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("SESSION_ID");
                if (cd != null)
                {
                    String SessionIdFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < sessionId.Count; i++)
                    {
                        SessionIdFilter = SessionIdFilter + string.Format("'{0}'",sessionId[i].Trim()) + ((i < (sessionId.Count - 1)) ? ", " : "");
                    }
                    SessionIdFilter = SessionIdFilter + ")";
                    ret = addToCondition(ret, SessionIdFilter);
                }
            }

            //get sqlcode id filter string
            List<string> sqlcodeId = GetIds(_theSQLCodeIds);
            if (sqlcodeId.Count > 0)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("SQL_CODE");
                if (cd != null)
                {
                    String sqlcodeFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < sqlcodeId.Count; i++)
                    {
                        sqlcodeFilter = sqlcodeFilter + sqlcodeId[i] + ((i < (sqlcodeId.Count - 1)) ? ", " : "");
                    }
                    sqlcodeFilter = sqlcodeFilter + ")";
                    ret = addToCondition(ret, sqlcodeFilter);
                }
            }

            //get outcome filter string            
            if (_theOutcomesFilters.Count > 0)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("OUTCOME");
                if (cd != null)
                {
                    String OutcomeFilter = string.Format("{0} IN (", cd.DisplayName);
                    for (int i = 0; i < _theOutcomesFilters.Count; i++)
                    {
                        OutcomeFilter = OutcomeFilter + string.Format("'{0}'",_theOutcomesFilters[i].Value) + ((i < (_theOutcomesFilters.Count - 1)) ? ", " : "");
                    }
                    OutcomeFilter = OutcomeFilter + ")";
                    ret = addToCondition(ret, OutcomeFilter);
                }
            }


            //get time range filter
            if (_theTimeRange != TimeRangeHandler.Range.AllTimes)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("GEN_TS_LCT");
                if (cd != null)
                {
                    String TimeFilter = cd.DisplayName;
                    if (_theTimeRange == TimeRangeHandler.Range.CustomRange)
                    {
                        if (_theCurrentTime)
                        {
                            TimeFilter = string.Format("{0} >= {1}", TimeFilter, _theCustomStartTime);
                        }
                        else
                        {
                            TimeFilter = string.Format("({0} >= {1}) AND ({2} <= {3}) ", TimeFilter, _theCustomStartTime, TimeFilter, _theCustomEndTime);
                        }
                    }
                    else
                    {
                        TimeFilter = string.Format("{0} IN {1}", TimeFilter, new TimeRangeHandler(_theTimeRange).ToString());
                    }
                    ret = addToCondition(ret, TimeFilter);
                }
            }
            //get message filter
            if (_theMessaegeFilter.Trim().Length > 0)
            {
                ColumnDetails cd = anAuditLogsDataHandler.getColumnDetailsForName("MESSAGE");
                if (cd != null)
                {
                    String MessageFilter = (!_theCaseSensitive) ? string.Format("UPSHIFT({0})", cd.DisplayName) : cd.DisplayName;
                    String filterMessage = (!_theCaseSensitive) ? _theMessaegeFilter.Trim().ToUpper() : _theMessaegeFilter.Trim();
                    if (_theMessageFilterCondition.Value.Equals("Contains"))
                    {
                        MessageFilter = string.Format("{0} LIKE '%{1}%'", MessageFilter, filterMessage);
                    }
                    else if (_theMessageFilterCondition.Value.Equals("Equals"))
                    {
                        MessageFilter = string.Format("{0} = '{1}'", MessageFilter, filterMessage);
                    }
                    else if (_theMessageFilterCondition.Value.Equals("Does Not Contain"))
                    {
                        MessageFilter = string.Format("{0} NOT LIKE '%{1}%'", MessageFilter, filterMessage);
                    }
                    ret = addToCondition(ret, MessageFilter);
                }
            }


            return ret;
        }

        public string AddUserId(string userId)
        {
            if (userId != null)
            {
                userId = userId.Trim();
                List<string> userIds = GetIds(_theUserIds);
                if (!userIds.Contains(userId))
                {
                    userIds.Add(userId);
                }
                _theUserIds = getFormattedListString(userIds);
            }
            return _theUserIds;
        }
        
        public string AddExternalUserNames(string externalUserName)
        {
            if (externalUserName != null)
            {
                externalUserName = externalUserName.Trim();
                List<string> externalUserNameList= GetIds(_theExternalUserNames);
                if (!externalUserNameList.Contains(externalUserName))
                {
                    externalUserNameList.Add(externalUserName);
                }
                _theExternalUserNames = getFormattedListString(externalUserNameList);
            }
            return _theExternalUserNames;
        }

        public string AddInternalUserNames(string internalUserName)
        {
            if (internalUserName != null)
            {
                internalUserName = internalUserName.Trim();
                List<string> internalUserNameList = GetIds(_theInternalUserNames);
                if (!internalUserNameList.Contains(internalUserName))
                {
                    internalUserNameList.Add(internalUserName);
                }
                _theInternalUserNames = getFormattedListString(internalUserNameList);
            }
            return _theInternalUserNames;
        }

        public string AddSessionIds(string sessionId)
        {
            if (sessionId != null)
            {
                sessionId = sessionId.Trim();
                List<string> sessionIdList = GetIds(_theSessionIds);
                if (!sessionIdList.Contains(sessionId))
                {
                    sessionIdList.Add(sessionId);
                }
                _theSessionIds = getFormattedListString(sessionIdList);
            }
            return _theSessionIds;
        }

        public string AddTransactionIds(string transactionId)
        {
            if (transactionId != null)
            {
                transactionId = transactionId.Trim();
                List<string> transactionIdList = GetIds(_theTransactionIds);
                if (!transactionIdList.Contains(transactionId))
                {
                    transactionIdList.Add(transactionId);
                }
                _theTransactionIds = getFormattedListString(transactionIdList);
            }
            return _theTransactionIds;
        }


        public string AddSQLCodes(string sqlCode)
        {
            if (sqlCode != null)
            {
                sqlCode = sqlCode.Trim();
                List<string> sqlCodeList = GetIds(_theSQLCodeIds);
                if (!sqlCodeList.Contains(sqlCode))
                {
                    sqlCodeList.Add(sqlCode);
                }
                _theSQLCodeIds = getFormattedListString(sqlCodeList);
            }
            return _theSQLCodeIds;
        }


        public ArrayList IsValid()
        {
            ArrayList ret = new ArrayList();
            if (_theAuditTypeFilters.Count == 0)
            {
                ret.Add(Properties.Resources.AuditLogsTypeFilterMissingMsg);
            }

            if (_theTimeRange == TimeRangeHandler.Range.CustomRange)
            {
                //we only need to check Start time must be less than or equal to the End time if the 
                //current time flag has not been set.
                if ((! CurrentTime) && (TheEndTime.CompareTo(TheStartTime) < 0))
                {
                    ret.Add("The Start time must be less than or equal to the End time");
                }
            }
            return ret;
        }
        #endregion

        #region Private Methods



        private string getFormattedListString(List<string> list)
        {
            string ret = "";
            int count = list.Count;
            for (int i = 0; i < count; i++)
            {
                ret += (i < (count - 1)) ? list[i].Trim() + ", " : list[i].Trim();
            }
            return ret;
        }

        //Helper to create the date string that is needed in the SQL statement
        public string getDateForQuery(DateTime aDateTime)
        {
            string timestamp = aDateTime.ToString("yyyy-MM-dd HH:mm:ss");
            return string.Format("TIMESTAMP '{0}'", timestamp);
        }

        //Helper method to add the filter conditions
        public string addToCondition(String existingFilter, string newFilter)
        {
            if ((newFilter != null) && (newFilter.Trim().Length > 0))
            {
                if (existingFilter.Trim().Length == 0)
                {
                    existingFilter = string.Format("({0})", newFilter);
                }
                else
                {
                    existingFilter = string.Format("{0} AND ({1})", existingFilter, newFilter.Trim());
                }
            }
            return existingFilter;
        }



        private bool AreAuditTypeFiltersSame(AuditLogFilterModel newModel)
        {
            if (_theAuditTypeFilters.Count == newModel.AuditTypeFilters.Count)
            {
                foreach (NameValuePair nvp in _theAuditTypeFilters)
                {
                    if (!(newModel.AuditTypeFilters.IndexOf(nvp) >= 0))
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }

     
        private bool AreUserIdsSame(AuditLogFilterModel newModel)
        {
            return AreStringListsSame(_theUserIds, newModel.UserIds);
        }

        private bool AreExternalUserNamesSame(AuditLogFilterModel newModel)
        {
            return AreStringListsSame(_theExternalUserNames, newModel.ExternalUserNames);
        }

        private bool AreInternalUserNamesSame(AuditLogFilterModel newModel)
        {
            return AreStringListsSame(_theInternalUserNames, newModel.InternalUserNames);
        }
    

        private bool AreSessionIdsSame(AuditLogFilterModel newModel)
        {
            return AreStringListsSame(_theSessionIds, newModel.SessionIds);
        }
    
        private bool AreTransactionIdsSame(AuditLogFilterModel newModel)
        {
            return AreStringListsSame(_theTransactionIds, newModel.TransactionIds);
        }

        private bool AreSQLCodeIdsSame(AuditLogFilterModel newModel)
        {
            return AreStringListsSame(_theSQLCodeIds, newModel.SQLCodeIds);
        }

        private bool AreOutcomeFiltersSame(AuditLogFilterModel newModel)
        {
            if (_theOutcomesFilters.Count == newModel.OutcomesFilters.Count)
            {
                foreach (NameValuePair nvp in _theOutcomesFilters)
                {
                    if (!(newModel.OutcomesFilters.IndexOf(nvp) >= 0))
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }

        private bool AreStringListsSame(string oldList, string newList)
        {
            List<string> oldIds = GetIds(oldList);
            List<string> newIds = GetIds(newList);

            if (oldIds.Count == newIds.Count)
            {
                foreach (string id in oldIds)
                {
                    if (!(newIds.IndexOf(id) >= 0))
                    {
                        return false;
                    }
                }
                return true;
            }
            else
            {
                return false;
            }
        }       
        

        #endregion
    }

  
    /// <summary>
    /// Helper class to manage the various default filters
    /// </summary>
    public class AuditLogDetails
    {
        #region Member Variables
        private ConnectionDefinition _theConnectionDefinition;
        private Connection _theCurrentConnection = null;

        private ArrayList _theAuditTypes=new ArrayList();
        private ArrayList _theOutcomes = new ArrayList();
        static private string[] _theMessageFilters = { "Contains", "Does Not Contain" };
        static private TimeRangeHandler[] _theTimeRanges = { new TimeRangeHandler()
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.CustomRange) 
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last10Minutes) 
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last20Minutes)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last30Minutes)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last1Hour)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Today)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last24Hours)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last7Days)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last14Days)
                                                           , new TimeRangeHandler(TimeRangeHandler.Range.Last30Days)};

        private const string TRACE_SUB_AREA_NAME = "Audit Log Filter";
        
        public const string AUDIT_TYPE_PRIVILEGES_NAME = "Privilege Changes";
        public const string AUDIT_TYPE_CONFIGCHANGES_NAME = "Configuration Changes";
        public const string AUDIT_TYPE_OBJECTCHANGES_NAME = "Object Changes";
        public const string AUDIT_TYPE_USERMGMT_NAME = "User Management";
        public const string AUDIT_TYPE_AUTHENTICATION_NAME = "Authentication";
        public const string AUDIT_TYPE_SECURITYVIOLATIONS_NAME = "Authorization violations";

        public const string AUDIT_TYPE_PRIVILEGES_VALUE = "Privilege Changes";
        public const string AUDIT_TYPE_CONFIGCHANGES_VALUE = "Configuration Changes";
        public const string AUDIT_TYPE_OBJECTCHANGES_VALUE = "Object Changes";
        public const string AUDIT_TYPE_USERMGMT_VALUE = "User Management";
        public const string AUDIT_TYPE_AUTHENTICATION_VALUE = "Authentication";
        public const string AUDIT_TYPE_SECURITYVIOLATIONS_VALUE = "Security Violations";
       
        #endregion

        #region Constructor
        public AuditLogDetails(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;
        }
        #endregion

        #region Properties
        public ArrayList AuditTypes
        {
            get
            {               

                if (_theAuditTypes != null && _theAuditTypes.Count>0)
                    return _theAuditTypes;
                _theAuditTypes.Add(new NameValuePair(AUDIT_TYPE_PRIVILEGES_NAME, AUDIT_TYPE_PRIVILEGES_VALUE));
                _theAuditTypes.Add(new NameValuePair(AUDIT_TYPE_CONFIGCHANGES_NAME, AUDIT_TYPE_CONFIGCHANGES_VALUE));
                _theAuditTypes.Add(new NameValuePair(AUDIT_TYPE_USERMGMT_NAME, AUDIT_TYPE_USERMGMT_VALUE));
                _theAuditTypes.Add(new NameValuePair(AUDIT_TYPE_OBJECTCHANGES_NAME, AUDIT_TYPE_OBJECTCHANGES_VALUE));
                _theAuditTypes.Add(new NameValuePair(AUDIT_TYPE_AUTHENTICATION_NAME, AUDIT_TYPE_AUTHENTICATION_VALUE));
                _theAuditTypes.Add(new NameValuePair(AUDIT_TYPE_SECURITYVIOLATIONS_NAME, AUDIT_TYPE_SECURITYVIOLATIONS_VALUE));
                return _theAuditTypes;
            }
            set
            {
                _theAuditTypes = value;
            }
        }
        public ArrayList Outcomes
        {
            get
            {
                if (_theOutcomes != null && _theOutcomes.Count > 0)
                    return _theOutcomes;
                _theOutcomes.Add(new NameValuePair("Successful", "S"));
                _theOutcomes.Add(new NameValuePair("Failed", "F"));
                _theOutcomes.Add(new NameValuePair("Failed and rolled back", "FR"));
                return _theOutcomes;
            }
            set
            {
                _theOutcomes = value;
            }
        }
        public string[] MessageFilters
        {
            get { return AuditLogDetails._theMessageFilters; }
            set { AuditLogDetails._theMessageFilters = value; }
        }

        public TimeRangeHandler[] TimeRanges
        {
            get { return _theTimeRanges; }
            set { _theTimeRanges = value; }
        }


      
        #endregion

        #region Public Methods

 

     


     

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
        #endregion
    }

 

   

 
}
