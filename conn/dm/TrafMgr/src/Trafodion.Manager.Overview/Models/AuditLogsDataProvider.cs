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
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.UniversalWidget;
using System.Text;

namespace Trafodion.Manager.OverviewArea.Models
{
    public class AuditLogsDataProvider : DatabaseDataProvider
    {
        #region Fields
        AuditLogFilterModel _theAuditLogFilterModel;
        int rowCount = 1000;
        private ArrayList _Columns = new ArrayList();
        public const string AUDIT_LOG_MESSAGE = "MESSAGE";

        public const string AUDIT_TYPE_VIEW_PRIVILEGES = "MANAGEABILITY.SECURITY_AUDIT_REPOSITORY.SECURITY_AUDIT_PRIVILEGES_1";
        public const string AUDIT_TYPE_VIEW_CONFIGCHANGES = "MANAGEABILITY.SECURITY_AUDIT_REPOSITORY.SECURITY_AUDIT_CONFIGCHANGES_1";
        public const string AUDIT_TYPE_VIEW_OBJECTCHANGES = "MANAGEABILITY.SECURITY_AUDIT_REPOSITORY.SECURITY_AUDIT_OBJECTCHANGES_2";
        public const string AUDIT_TYPE_VIEW_USERMGMT = "MANAGEABILITY.SECURITY_AUDIT_REPOSITORY.SECURITY_AUDIT_USERMANAGEMENT_2";
        public const string AUDIT_TYPE_VIEW_AUTHENTICATION = "MANAGEABILITY.SECURITY_AUDIT_REPOSITORY.SECURITY_AUDIT_AUTHENTICATION_2";
        public const string AUDIT_TYPE_VIEW_SECURITYVIOLATIONS = "MANAGEABILITY.SECURITY_AUDIT_REPOSITORY.SECURITY_AUDIT_ACCESSVIOLATIONS_1";
        
        #endregion Fields

        #region Properties
        private string TheSQLText
        {
            get
            {
                StringBuilder sbSQL = new StringBuilder();


                //Construct a union query against the individual views
                int iFiltersCount = 0;
                sbSQL.Append("SELECT {0} * FROM (");
                foreach (NameValuePair nvp in AuditLogFilterModel.AuditTypeFilters)
                {
                        
                    #region Concatenate Sub-SQL statement
                        
                    sbSQL.Append("SELECT ");
                    sbSQL.Append(" GEN_TS_LCT,");
                    sbSQL.Append("'");
                    sbSQL.Append(nvp.Name);
                    sbSQL.Append("' AS  AUDIT_TYPE,");
                    sbSQL.Append(" EXTERNAL_USER_NAME,");
                    sbSQL.Append(" USER_ID,");
                    sbSQL.Append(" INTERNAL_USER_NAME,");
                    if (nvp.Name.Equals(AuditLogDetails.AUDIT_TYPE_AUTHENTICATION_NAME))
                    {
                        sbSQL.Append("'NA' AS SESSION_ID,");
                    }
                    else
                    {
                        sbSQL.Append(" SESSION_ID,");
                    }
                    sbSQL.Append(" TRANSACTION_ID,");
                    sbSQL.Append(" OUTCOME,");
                    sbSQL.Append(" SQL_CODE,");
                    sbSQL.Append(" MESSAGE ");
                    sbSQL.Append(" FROM ");
                    switch (nvp.Name)
                    {
                        case AuditLogDetails.AUDIT_TYPE_AUTHENTICATION_NAME:
                            {
                                sbSQL.Append(AUDIT_TYPE_VIEW_AUTHENTICATION);
                                break;
                            }
                        case AuditLogDetails.AUDIT_TYPE_CONFIGCHANGES_NAME:
                            {
                                sbSQL.Append(AUDIT_TYPE_VIEW_CONFIGCHANGES);
                                break;
                            }
                        case AuditLogDetails.AUDIT_TYPE_OBJECTCHANGES_NAME:
                            {
                                sbSQL.Append(AUDIT_TYPE_VIEW_OBJECTCHANGES);
                                break;
                            }
                        case AuditLogDetails.AUDIT_TYPE_PRIVILEGES_NAME:
                            {
                                sbSQL.Append(AUDIT_TYPE_VIEW_PRIVILEGES);
                                break;
                            }
                        case AuditLogDetails.AUDIT_TYPE_SECURITYVIOLATIONS_NAME:
                            {
                                sbSQL.Append(AUDIT_TYPE_VIEW_SECURITYVIOLATIONS);
                                break;
                            }
                        case AuditLogDetails.AUDIT_TYPE_USERMGMT_VALUE:
                            {
                                sbSQL.Append(AUDIT_TYPE_VIEW_USERMGMT);
                                break;
                            }
                    }
                    if (iFiltersCount < AuditLogFilterModel.AuditTypeFilters.Count-1)
                    {
                        sbSQL.Append(" UNION ALL ");
                    }
                    iFiltersCount++;
                    #endregion
                }
                sbSQL.Append(") AS LOGS_TEMP");
                sbSQL.Append(" {1} ");
                sbSQL.Append(" ORDER BY GEN_TS_LCT DESC for read uncommitted access;");



                return sbSQL.ToString();
            }

        }
        public AuditLogFilterModel AuditLogFilterModel
        {
            get { return _theAuditLogFilterModel; }
            set 
            {
                _theAuditLogFilterModel = value;
                Persistence.Put(AuditLogFilterModel.AuditLogFilterPersistenceKey, _theAuditLogFilterModel);
            }
        }

        public ArrayList ColumnsToFilterOn
        {
            get { return _Columns; }
            set { _Columns = value; }
        }

        #endregion Properties

        #region Constructors

        public AuditLogsDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
        }

        #endregion Constructors

        //Overrides the default DoPrefetchSetup and sets up the query based on the number of rows requested and the 
        //filter criteria.
        public override void DoPrefetchSetup(Hashtable predefinedParametersHash)
        {
            AuditLogDataProviderConfig dbConfig = DataProviderConfig as AuditLogDataProviderConfig;
            rowCount = dbConfig.MaxRowCount;
            string rowCountString = (rowCount > 0) ? string.Format("[First {0}]", rowCount) : "";
            string filterStr = (_theAuditLogFilterModel != null) ? _theAuditLogFilterModel.GetFilterSQL() : "";
            string whereClause = ((filterStr != null) && (filterStr.Trim().Length > 0)) ? ("WHERE " + filterStr) : "";
            dbConfig.SQLText = string.Format(TheSQLText, rowCountString, whereClause);
            base.DoPrefetchSetup(predefinedParametersHash);
        }  
    }

    [Serializable]
    public class AuditLogDataProviderConfig : DatabaseDataProviderConfig
    {
        int _CacheSize = 1000;
        //[XmlIgnore]
        [NonSerialized]
        protected AuditLogsDataProvider _theDataProvider = null;
        //[XmlIgnore]
        public override DataProviderTypes DataProviderType
        {
            get { return DataProviderTypes.DB; }
        }

        /// <summary>
        /// Default constructor
        /// </summary>
        public AuditLogDataProviderConfig()
        {
            base.TimerContinuesOnError = true;
        }

        /// <summary>
        /// Returns a new DatabaseDataProvider using this config
        /// </summary>
        /// <returns></returns>
        public override DataProvider GetDataProvider()
        {
            if (_theDataProvider == null)
            {
                _theDataProvider = new AuditLogsDataProvider(this);
            }
            return _theDataProvider;
        }


        public int CacheSize
        {
            get { return _CacheSize; }
            set { _CacheSize = value; }
        }
    }
}
